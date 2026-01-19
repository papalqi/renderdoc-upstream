import http from "node:http";
import https from "node:https";

export class JsonRpcCallError extends Error {
  public readonly requestId: string;
  public readonly code?: number;
  public readonly data?: unknown;
  public readonly httpStatus?: number;

  public constructor(params: {
    requestId: string;
    message: string;
    code?: number;
    data?: unknown;
    httpStatus?: number;
  }) {
    super(params.message);
    this.name = "JsonRpcCallError";
    this.requestId = params.requestId;
    this.code = params.code;
    this.data = params.data;
    this.httpStatus = params.httpStatus;
  }
}

type JsonRpcResponseOk<T> = { jsonrpc: "2.0"; id: string; result: T };
type JsonRpcResponseError = {
  jsonrpc: "2.0";
  id: string | null;
  error: { code: number; message: string; data?: unknown };
};

function makeRequestId(): string {
  return `rdai_${Date.now()}_${Math.random().toString(16).slice(2)}`;
}

function safeString(value: unknown): string {
  if (typeof value === "string") return value;
  try {
    return JSON.stringify(value);
  } catch {
    return String(value);
  }
}

export async function jsonRpcCall<T>(params: {
  rpcUrl: string;
  bearerToken: string;
  method: string;
  rpcParams?: unknown;
  timeoutMs: number;
}): Promise<{ requestId: string; result: T }> {
  const requestId = makeRequestId();
  const payload = JSON.stringify({
    jsonrpc: "2.0",
    id: requestId,
    method: params.method,
    params: params.rpcParams ?? {},
  });

  const url = new URL(params.rpcUrl);
  if (url.protocol !== "http:" && url.protocol !== "https:") {
    throw new JsonRpcCallError({
      requestId,
      message: `Unsupported RPC URL protocol: ${url.protocol}`,
    });
  }

  const client = url.protocol === "https:" ? https : http;
  const port = url.port ? Number(url.port) : url.protocol === "https:" ? 443 : 80;

  const headers: Record<string, string> = {
    "Content-Type": "application/json",
    "Content-Length": Buffer.byteLength(payload).toString(),
    Authorization: `Bearer ${params.bearerToken}`,
  };

  return await new Promise<{ requestId: string; result: T }>((resolve, reject) => {
    const req = client.request(
      {
        method: "POST",
        hostname: url.hostname,
        port,
        path: url.pathname,
        headers,
      },
      (res) => {
        const status = res.statusCode ?? 0;
        let data = "";
        res.setEncoding("utf8");
        res.on("data", (chunk) => (data += chunk));
        res.on("end", () => {
          let parsed: unknown;
          try {
            parsed = data ? JSON.parse(data) : null;
          } catch (e) {
            reject(
              new JsonRpcCallError({
                requestId,
                httpStatus: status,
                message: `Invalid JSON-RPC response (HTTP ${status}): ${String(e)}`,
              }),
            );
            return;
          }

          if (parsed && typeof parsed === "object" && "error" in (parsed as any)) {
            const err = (parsed as JsonRpcResponseError).error;
            reject(
              new JsonRpcCallError({
                requestId,
                httpStatus: status,
                code: err.code,
                data: err.data,
                message: `JSON-RPC error ${err.code}: ${err.message}`,
              }),
            );
            return;
          }

          if (parsed && typeof parsed === "object" && "result" in (parsed as any)) {
            resolve({ requestId, result: (parsed as JsonRpcResponseOk<T>).result });
            return;
          }

          reject(
            new JsonRpcCallError({
              requestId,
              httpStatus: status,
              message: `Invalid JSON-RPC response shape: ${safeString(parsed)}`,
            }),
          );
        });
      },
    );

    req.setTimeout(params.timeoutMs, () => {
      req.destroy(
        new JsonRpcCallError({
          requestId,
          message: `JSON-RPC request timed out after ${params.timeoutMs}ms`,
        }),
      );
    });

    req.on("error", (e) => {
      reject(
        e instanceof JsonRpcCallError
          ? e
          : new JsonRpcCallError({ requestId, message: `JSON-RPC transport error: ${String(e)}` }),
      );
    });

    req.write(payload);
    req.end();
  });
}

