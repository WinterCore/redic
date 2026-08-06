import { connect, type Redis } from "@db/redis";

const REPO_ROOT = new URL("../../", import.meta.url);

export interface RunningRedic {
  port: number;
  // The server's working directory — it writes `data.aof` relative to this,
  // so a fresh dir is an empty AOF and two servers never share one.
  dir: string;
  proc: Deno.ChildProcess;
}

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

  // Long form on purpose: `-p` is currently ignored (cli.c matches the short
  // flag against def->name rather than def->shorthand) and falls back to 6969.
  const proc = new Deno.Command(new URL("Redic", REPO_ROOT), {
    args: ["--port", String(port)],
    cwd,
    stdout: "null",
    stderr: "null",
  }).spawn();

  const server = { port, dir: cwd, proc };

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
      const redis = await connectTo(server);
      redis.close();

      return;
    } catch {
      await delay(25);
    }
  }

  throw new Error(
    `Redic never accepted connections on port ${server.port}. ` +
      `Reproduce with: ./Redic --port ${server.port}`,
  );
}

export async function stopRedic(server: RunningRedic): Promise<void> {
  server.proc.kill("SIGKILL");

  // Awaited so Deno's op sanitizer doesn't see a dangling child
  await server.proc.status;
}

/** Stops the server and throws away its AOF. */
export async function stopRedicAndClean(server: RunningRedic): Promise<void> {
  await stopRedic(server);
  await Deno.remove(server.dir, { recursive: true });
}

export function connectTo(server: RunningRedic): Promise<Redis> {
  return connect({ hostname: "127.0.0.1", port: server.port });
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
