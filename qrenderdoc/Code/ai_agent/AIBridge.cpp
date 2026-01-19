#include "AIBridge.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QStandardPaths>
#include <QTcpServer>
#include <QThread>
#include <QUuid>

#include "Code/Interface/QRDInterface.h"
#include "Code/pyrenderdoc/PythonContext.h"

static const char kBridgeScriptName[] = "rdai_bridge.py";

static QString DefaultBridgeScript()
{
  return QString::fromUtf8(R"PY(
import json
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from socketserver import ThreadingMixIn

_rdai_token = None
_rdai_server = None
_rdai_thread = None

def _rdai_now_ms():
  return int(time.time() * 1000)

def _rdai_api_string(api_props):
  try:
    s = str(api_props.pipelineType)
    if "." in s:
      return s.split(".")[-1]
    return s
  except Exception:
    return None

def _rdai_get_context():
  capture_path = None
  api = None
  event_id = 0
  event_name = None

  try:
    ctx = globals().get("pyrenderdoc", None)
    if ctx is None:
      return {
        "capture_path": None,
        "api": None,
        "event_id": 0,
        "event_name": None,
      }

    if ctx.IsCaptureLoaded():
      capture_path = ctx.GetCaptureFilename()

      api_props = ctx.APIProps()
      api = _rdai_api_string(api_props)

      event_id = int(ctx.CurEvent())

      act = ctx.CurAction()
      if act is not None:
        n = None
        try:
          if hasattr(act, "customName") and act.customName:
            n = act.customName
        except Exception:
          n = None
        if not n:
          try:
            if hasattr(act, "name") and act.name:
              n = act.name
          except Exception:
            n = None
        event_name = n
  except Exception:
    pass

  return {
    "capture_path": capture_path,
    "api": api,
    "event_id": event_id,
    "event_name": event_name,
  }

class _ThreadingHTTPServer(ThreadingMixIn, HTTPServer):
  daemon_threads = True

class _Handler(BaseHTTPRequestHandler):
  protocol_version = "HTTP/1.1"

  def log_message(self, fmt, *args):
    return

  def _send_json(self, http_code, obj):
    data = json.dumps(obj, separators=(",", ":")).encode("utf-8")
    self.send_response(http_code)
    self.send_header("Content-Type", "application/json")
    self.send_header("Content-Length", str(len(data)))
    self.end_headers()
    self.wfile.write(data)

  def _send_error(self, req_id, code, message, data=None, http_code=200):
    err = {"code": code, "message": message}
    if data is not None:
      err["data"] = data
    self._send_json(http_code, {"jsonrpc": "2.0", "id": req_id, "error": err})

  def do_POST(self):
    start_ms = _rdai_now_ms()
    req_id = None
    method = None
    ok = False
    http_code = 200

    try:
      if self.path != "/rpc":
        self.send_response(404)
        self.end_headers()
        return

      auth = self.headers.get("Authorization", "")
      if not auth.startswith("Bearer "):
        http_code = 401
        self._send_error(None, -32001, "unauthorized", http_code=http_code)
        return

      token = auth[len("Bearer "):]
      if token != _rdai_token:
        http_code = 401
        self._send_error(None, -32001, "unauthorized", http_code=http_code)
        return

      try:
        length = int(self.headers.get("Content-Length", "0"))
      except Exception:
        length = 0

      raw = self.rfile.read(length).decode("utf-8") if length > 0 else ""

      try:
        req = json.loads(raw)
      except Exception as e:
        self._send_error(None, -32700, "parse error", data=str(e))
        return

      req_id = req.get("id", None)

      if req.get("jsonrpc") != "2.0" or "method" not in req:
        self._send_error(req_id, -32600, "invalid request")
        return

      method = req.get("method", None)

      if method == "renderdoc.get_context":
        result = _rdai_get_context()
        ok = True
        self._send_json(200, {"jsonrpc": "2.0", "id": req_id, "result": result})
        return

      self._send_error(req_id, -32601, "method not found")
    except Exception as e:
      self._send_error(req_id, -32000, "server error", data=str(e))
    finally:
      elapsed_ms = _rdai_now_ms() - start_ms
      status = "ok" if ok else "error"
      rid = req_id if req_id is not None else ""
      m = method if method is not None else ""
      # Logs are intentionally low-detail and never include tokens or request bodies.
      print("RDAI_BRIDGE request_id=%s method=%s status=%s elapsed_ms=%d" % (rid, m, status, elapsed_ms))

def rdai_bridge_start(port, token):
  global _rdai_token, _rdai_server, _rdai_thread

  if _rdai_server is not None:
    return

  _rdai_token = token
  _rdai_server = _ThreadingHTTPServer(("127.0.0.1", int(port)), _Handler)
  _rdai_thread = threading.Thread(target=_rdai_server.serve_forever, name="rdai_bridge", daemon=True)
  _rdai_thread.start()
  print("RDAI_BRIDGE started port=%d" % _rdai_server.server_port)

def rdai_bridge_stop():
  global _rdai_server, _rdai_thread

  if _rdai_server is None:
    return

  try:
    _rdai_server.shutdown()
  except Exception:
    pass

  try:
    _rdai_server.server_close()
  except Exception:
    pass

  _rdai_server = None
  _rdai_thread = None
  print("RDAI_BRIDGE stopped")
)PY");
}

static QString GetBridgeScriptPath()
{
  QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if(base.isEmpty())
    return QString();

  QDir dir(base);
  dir.mkpath(lit("ai"));
  dir.cd(lit("ai"));
  return dir.absoluteFilePath(QString::fromUtf8(kBridgeScriptName));
}

static QString LoadBridgeScript(const QString &path)
{
  QFile f(path);
  if(!f.open(QIODevice::ReadOnly | QIODevice::Text))
    return QString();

  return QString::fromUtf8(f.readAll());
}

static bool EnsureDefaultBridgeScriptExists(const QString &path, QString &error)
{
  QFileInfo fi(path);
  QDir parent(fi.absolutePath());
  if(!parent.exists() && !parent.mkpath(lit(".")))
  {
    error = QFormatStr("Failed to create directory '%1'").arg(fi.absolutePath());
    return false;
  }

  if(fi.exists())
    return true;

  QFile f(path);
  if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
  {
    error = QFormatStr("Failed to write '%1'").arg(path);
    return false;
  }

  QByteArray bytes = DefaultBridgeScript().toUtf8();
  if(f.write(bytes) != bytes.size())
  {
    error = QFormatStr("Failed to write '%1'").arg(path);
    return false;
  }

  return true;
}

static uint16_t PickPort()
{
  QTcpServer server;
  if(!server.listen(QHostAddress::LocalHost, 0))
    return 0;

  uint16_t port = (uint16_t)server.serverPort();
  server.close();
  return port;
}

AIBridgeWorker::AIBridgeWorker(ICaptureContext &ctx, const QString &token, QObject *parent)
    : QObject(parent), m_Ctx(ctx), m_token(token)
{
}

void AIBridgeWorker::start()
{
  if(m_py)
    return;

  m_port = PickPort();
  if(m_port == 0)
  {
    emit error(lit("Failed to pick a localhost port for AI bridge"));
    return;
  }

  m_py = new PythonContextHandle();

  QObject::connect(&m_py->ctx(), &PythonContext::exception, this,
                   [this](const QString &type, const QString &value, int, QList<QString>) {
                     if(!m_starting)
                       return;
                     m_startError = QFormatStr("%1: %2").arg(type).arg(value);
                   });

  QObject::connect(&m_py->ctx(), &PythonContext::textOutput, this,
                   [](bool isStdError, const QString &output) {
                     QString line = output.trimmed();
                     if(!line.startsWith(lit("RDAI_BRIDGE")))
                       return;

                     if(isStdError)
                       qWarning() << line;
                     else
                       qInfo() << line;
                   });

  m_py->ctx().setGlobal("pyrenderdoc", (ICaptureContext *)&m_Ctx);

  QString scriptPath = GetBridgeScriptPath();
  QString scriptError;
  QString script;

  if(!scriptPath.isEmpty() && EnsureDefaultBridgeScriptExists(scriptPath, scriptError))
    script = LoadBridgeScript(scriptPath);

  if(script.isEmpty())
    script = DefaultBridgeScript();

  m_starting = true;
  m_startError.clear();

  if(!scriptPath.isEmpty())
    m_py->ctx().executeString(scriptPath, script);
  else
    m_py->ctx().executeString(script);

  QString cmd = QFormatStr("rdai_bridge_start(%1, \"%2\")").arg(m_port).arg(m_token);
  m_py->ctx().executeString(cmd);

  m_starting = false;

  if(!m_startError.isEmpty())
  {
    emit error(QFormatStr("AI bridge failed to start: %1").arg(m_startError));
    return;
  }

  emit started(m_port);
}

void AIBridgeWorker::stop()
{
  if(!m_py)
  {
    emit stopped();
    return;
  }

  m_py->ctx().executeString(lit("rdai_bridge_stop()"));
  delete m_py;
  m_py = NULL;
  emit stopped();
}

AIBridge::AIBridge(ICaptureContext &ctx, QObject *parent) : QObject(parent), m_Ctx(ctx)
{
  QObject::connect(qApp, &QCoreApplication::aboutToQuit, this, &AIBridge::Stop);
}

AIBridge::~AIBridge()
{
  Stop();
}

QString AIBridge::RpcUrl() const
{
  if(m_port == 0)
    return QString();

  return QFormatStr("http://127.0.0.1:%1/rpc").arg(m_port);
}

void AIBridge::Start()
{
  if(m_thread)
    return;

  m_token = generateToken();

  QThread *thread = new QThread(this);
  AIBridgeWorker *worker = new AIBridgeWorker(m_Ctx, m_token);

  worker->moveToThread(thread);

  QObject::connect(thread, &QThread::started, worker, &AIBridgeWorker::start);
  QObject::connect(worker, &AIBridgeWorker::started, this, &AIBridge::onWorkerStarted);
  QObject::connect(worker, &AIBridgeWorker::error, this, &AIBridge::onWorkerError);
  QObject::connect(thread, &QThread::finished, worker, &QObject::deleteLater);

  m_thread = thread;
  m_worker = worker;

  thread->start();
}

void AIBridge::Stop()
{
  if(!m_thread || !m_worker)
    return;

  QMetaObject::invokeMethod(m_worker, "stop", Qt::BlockingQueuedConnection);

  m_thread->quit();
  m_thread->wait(2000);

  m_thread->deleteLater();
  m_thread = NULL;
  m_worker = NULL;

  m_running = false;
  m_port = 0;
  m_token.clear();

  emit BridgeStopped();
}

void AIBridge::onWorkerStarted(uint16_t port)
{
  m_running = true;
  m_port = port;
  emit BridgeStarted();
}

void AIBridge::onWorkerError(const QString &message)
{
  emit BridgeError(message);

  // Ensure we don't leave a half-initialised bridge thread running.
  Stop();
}

QString AIBridge::generateToken()
{
  QByteArray bytes;
  while(bytes.size() < 32)
  {
    // QUuid is available in older Qt versions and uses platform RNG where possible.
    bytes.append(QUuid::createUuid().toRfc4122());
  }

  bytes.truncate(32);

  return QString::fromUtf8(bytes.toHex());
}
