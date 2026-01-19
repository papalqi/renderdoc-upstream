const http = require("node:http");
const assert = require("node:assert/strict");
const test = require("node:test");

const { jsonRpcCall, JsonRpcCallError } = require("../dist/jsonRpc");

function startServer(handler) {
  return new Promise((resolve) => {
    const server = http.createServer(handler);
    server.listen(0, "127.0.0.1", () => {
      const addr = server.address();
      resolve({ server, port: addr.port });
    });
  });
}

test("jsonRpcCall rejects unauthorized", async () => {
  const token = "good";
  const { server, port } = await startServer(async (req, res) => {
    if (req.method !== "POST" || req.url !== "/rpc") {
      res.statusCode = 404;
      res.end();
      return;
    }
    const auth = req.headers.authorization || "";
    if (auth !== `Bearer ${token}`) {
      res.statusCode = 401;
      res.setHeader("Content-Type", "application/json");
      res.end(JSON.stringify({ jsonrpc: "2.0", id: null, error: { code: -32001, message: "unauthorized" } }));
      return;
    }
    res.statusCode = 200;
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify({ jsonrpc: "2.0", id: "x", result: { ok: true } }));
  });

  try {
    await assert.rejects(
      () =>
        jsonRpcCall({
          rpcUrl: `http://127.0.0.1:${port}/rpc`,
          bearerToken: "bad",
          method: "renderdoc.get_context",
          timeoutMs: 2000,
        }),
      (e) => e instanceof JsonRpcCallError,
    );
  } finally {
    server.close();
  }
});

test("jsonRpcCall returns result", async () => {
  const token = "good";
  const { server, port } = await startServer(async (req, res) => {
    if (req.method !== "POST" || req.url !== "/rpc") {
      res.statusCode = 404;
      res.end();
      return;
    }
    const auth = req.headers.authorization || "";
    if (auth !== `Bearer ${token}`) {
      res.statusCode = 401;
      res.setHeader("Content-Type", "application/json");
      res.end(JSON.stringify({ jsonrpc: "2.0", id: null, error: { code: -32001, message: "unauthorized" } }));
      return;
    }

    let body = "";
    req.setEncoding("utf8");
    for await (const chunk of req) body += chunk;
    const parsed = JSON.parse(body);
    assert.equal(parsed.jsonrpc, "2.0");
    assert.equal(parsed.method, "renderdoc.get_context");

    res.statusCode = 200;
    res.setHeader("Content-Type", "application/json");
    res.end(JSON.stringify({ jsonrpc: "2.0", id: parsed.id, result: { capture_path: null, event_id: 1 } }));
  });

  try {
    const r = await jsonRpcCall({
      rpcUrl: `http://127.0.0.1:${port}/rpc`,
      bearerToken: token,
      method: "renderdoc.get_context",
      timeoutMs: 2000,
    });
    assert.equal(r.result.event_id, 1);
  } finally {
    server.close();
  }
});

