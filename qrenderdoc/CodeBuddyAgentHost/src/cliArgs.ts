export type AgentHostArgs = {
  model?: string;
  timeoutMs: number;
};

const DEFAULT_TIMEOUT_MS = 5 * 60 * 1000;

export function parseArgs(argv: string[]): AgentHostArgs {
  const out: AgentHostArgs = { timeoutMs: DEFAULT_TIMEOUT_MS };

  for (let i = 0; i < argv.length; i++) {
    const a = argv[i];
    if (a === "--model" && i + 1 < argv.length) {
      out.model = argv[i + 1];
      i++;
      continue;
    }

    if (a === "--timeout-ms" && i + 1 < argv.length) {
      const v = Number(argv[i + 1]);
      if (Number.isFinite(v) && v > 0) out.timeoutMs = v;
      i++;
      continue;
    }
  }

  return out;
}

