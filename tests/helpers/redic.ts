import { connect, type Redis, type RedisConnectOptions } from "@db/redis";

const REPO_ROOT = new URL("../../", import.meta.url);

export interface RunningRedic {
  port: number;
  // The server's working directory — it writes `data.aof` relative to this,
  // so a fresh dir is an empty AOF and two servers never share one.
  dir: string;
  proc: Deno.ChildProcess;
  /** Tail of the server's stderr — where PANIC and DEBUG_PRINTF both land. */
  stderr(): string;
  /** The exit status, once it has one. Undefined while still running. */
  exit(): Deno.CommandStatus | undefined;
  /** Resolves when stderr has been fully consumed. Awaited by stopRedic. */
  drained: Promise<void>;
}

/** FLUSH_TIMEOUT in src/potty/potty.c — how long a write can sit unflushed. */
export const FLUSH_TIMEOUT_MS = 1000;

let build: Promise<void> | undefined;

/**
 * Builds the server. Memoised so that N suites sharing a `deno test` process
 * only run make once.
 */
export function buildRedic(): Promise<void> {
  if (build === undefined) {
    build = runMake();
  }

  return build;
}

async function runMake(): Promise<void> {
  const { code, stderr } = await new Deno.Command("make", {
    args: ["debug"],
    cwd: REPO_ROOT,
    stdout: "null",
    stderr: "piped",
  }).output();

  if (code !== 0) {
    throw new Error(`make debug failed:\n${new TextDecoder().decode(stderr)}`);
  }
}

/** Asks the OS for an unused port rather than hardcoding one. */
function freePort(): number {
  const listener = Deno.listen({ port: 0 });
  const { port } = listener.addr as Deno.NetAddr;
  listener.close();

  return port;
}

/**
 * Starts a server and waits for it to accept connections.
 * Pass an existing `dir` to reuse a previous server's AOF — that's what makes
 * a restart a restart rather than a fresh instance.
 */
export async function startRedic(dir?: string): Promise<RunningRedic> {
  await buildRedic();

  const cwd = dir ?? await Deno.makeTempDir({ prefix: "redic_test_" });
  const port = freePort();

  const proc = new Deno.Command(new URL("Redic", REPO_ROOT), {
    args: ["--port", String(port)],
    cwd,
    stdout: "null",
    stderr: "piped",
  }).spawn();

  // A debug build logs every command, so keep the tail rather than the lot —
  // it's only ever read when something already went wrong, and a PANIC is the
  // last thing written.
  let tail = "";
  const drained = (async () => {
    for await (const chunk of proc.stderr.pipeThrough(new TextDecoderStream())) {
      tail = (tail + chunk).slice(-4096);
    }
  })();

  let exit: Deno.CommandStatus | undefined;
  proc.status.then((status) => {
    exit = status;
  });

  const server: RunningRedic = {
    port,
    dir: cwd,
    proc,
    stderr: () => tail,
    exit: () => exit,
    drained,
  };

  try {
    await waitUntilAccepting(server);
  } catch (error) {
    // Don't strand the child or the temp dir when startup fails
    await stopRedic(server);

    if (dir === undefined) {
      await Deno.remove(cwd, { recursive: true });
    }

    throw error;
  }

  return server;
}

async function waitUntilAccepting(server: RunningRedic): Promise<void> {
  const deadline = Date.now() + 5000;

  while (Date.now() < deadline) {
    try {
      // No retries: the client's own retry loop backs off for ~27s, which
      // swallows this deadline entirely and looks like a hang.
      const redis = await connectTo(server, { maxRetryCount: 0 });

      // A bare connect isn't proof of life: the socket listens before the
      // accept loop starts, so the backlog completes the handshake even for a
      // server that is busy dying during AOF replay. A round-trip isn't.
      await redis.ping();
      redis.close();

      return;
    } catch {
      // A server that died on startup is never coming back, so say why now
      // rather than burning the rest of the deadline on a corpse.
      const exit = server.exit();

      if (exit !== undefined) {
        throw new Error(
          `Redic exited with code ${exit.code} before accepting connections ` +
            `on port ${server.port}.\n--- stderr ---\n${server.stderr()}`,
        );
      }

      await delay(25);
    }
  }

  throw new Error(
    `Redic never accepted connections on port ${server.port}. ` +
      `Reproduce with: ./Redic --port ${server.port}\n` +
      `--- stderr ---\n${server.stderr()}`,
  );
}

export async function stopRedic(server: RunningRedic): Promise<void> {
  try {
    server.proc.kill("SIGKILL");
  } catch {
    // Already gone — a startup PANIC gets here, and signalling a dead child
    // throws an error that would mask the one we're cleaning up after.
  }

  // Awaited so Deno's op sanitizer doesn't see a dangling child or a
  // half-read stderr stream
  await server.proc.status;
  await server.drained;
}

/** Stops the server and throws away its AOF. */
export async function stopRedicAndClean(server: RunningRedic): Promise<void> {
  await stopRedic(server);
  await Deno.remove(server.dir, { recursive: true });
}

/**
 * Waits for everything written since the AOF was `sizeBefore` bytes to reach
 * disk, then restarts the server on the same directory.
 *
 * `sizeBefore` is not optional for a reason: with no flush-on-exit, the only
 * evidence the tail of the log made it is the file growing past where it stood
 * before the writes. Waiting for "non-empty" instead is already satisfied by
 * whatever an earlier case left in there, and the restart silently loses the
 * last second of writes.
 */
export async function restartRedic(
  server: RunningRedic,
  sizeBefore: number,
): Promise<RunningRedic> {
  await waitForAofQuiesce(server.dir, sizeBefore);
  await stopRedic(server);

  return await startRedic(server.dir);
}

/**
 * Runs `write` against a fresh server, restarts it, then runs `check` against
 * the replayed one. Every call gets its own AOF, so cases can't contaminate
 * each other.
 */
export async function acrossRestart(
  write: (redis: Redis) => Promise<void>,
  check: (redis: Redis) => Promise<void>,
): Promise<void> {
  let server = await startRedic();

  try {
    const first = await connectTo(server);

    try {
      await write(first);
    } finally {
      first.close();
    }

    // Fresh dir, so the log starts at zero bytes
    server = await restartRedic(server, 0);

    const second = await connectTo(server);

    try {
      await check(second);
    } finally {
      second.close();
    }
  } finally {
    await stopRedicAndClean(server);
  }
}

export function connectTo(
  server: RunningRedic,
  options?: Partial<RedisConnectOptions>,
): Promise<Redis> {
  return connect({ hostname: "127.0.0.1", port: server.port, ...options });
}

/**
 * Waits for the flusher to put something on disk and returns the AOF size.
 *
 * The potty flushes on a timer (FLUSH_TIMEOUT in src/potty/potty.c), so there
 * is no way to ask for a flush — you wait one out. Polling rather than sleeping
 * the full interval keeps the test as fast as the flusher allows, and gives a
 * useful message instead of a mystery assertion when nothing ever lands.
 */
export async function waitForAofFlush(
  dir: string,
  minSize = 1,
): Promise<number> {
  const aof = `${dir}/data.aof`;
  const deadline = Date.now() + 10_000;

  while (Date.now() < deadline) {
    const size = await aofSize(aof);

    if (size >= minSize) {
      // Let the rest of the batch land before anyone kills the process
      await delay(250);

      return await aofSize(aof);
    }

    await delay(100);
  }

  throw new Error(
    `${aof} never reached ${minSize} bytes. Is the flusher running?`,
  );
}

/**
 * Waits for the log to stop growing, and returns its final size.
 *
 * `waitForAofFlush` only proves *something* landed. A write burst bigger than
 * FLUSH_ITEMS_THRESHOLD is split across several jobs, so the first one lands
 * immediately and the rest wait out the timer — stopping there would silently
 * cut the tail off. The file going quiet for a full flush window is the only
 * signal that there is nothing left coming.
 */
export async function waitForAofQuiesce(
  dir: string,
  sizeBefore = 0,
): Promise<number> {
  const aof = `${dir}/data.aof`;

  await waitForAofFlush(dir, sizeBefore + 1);

  const deadline = Date.now() + 30_000;
  let last = await aofSize(aof);

  while (Date.now() < deadline) {
    await delay(FLUSH_TIMEOUT_MS + 300);

    const size = await aofSize(aof);

    if (size === last) {
      return size;
    }

    last = size;
  }

  throw new Error(`${aof} never stopped growing`);
}

export async function aofSize(path: string): Promise<number> {
  try {
    return (await Deno.stat(path)).size;
  } catch {
    return 0;
  }
}

export function delay(ms: number): Promise<void> {
  return new Promise((resolve) => setTimeout(resolve, ms));
}
