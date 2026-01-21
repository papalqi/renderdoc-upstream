#include "AIBridge.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QMetaType>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QUuid>

#include "Code/Interface/QRDInterface.h"
#include "Code/QRDUtils.h"

static qint64 NowMs()
{
  return QDateTime::currentMSecsSinceEpoch();
}

static QByteArray HttpReasonPhrase(int code)
{
  switch(code)
  {
    case 200: return "OK";
    case 400: return "Bad Request";
    case 401: return "Unauthorized";
    case 404: return "Not Found";
    default: return "OK";
  }
}

static QJsonObject MakeJsonRpcError(const QJsonValue &id, int code, const QString &message,
                                   const QJsonValue &data = QJsonValue())
{
  QJsonObject err;
  err[lit("code")] = code;
  err[lit("message")] = message;
  if(!data.isUndefined())
    err[lit("data")] = data;

  QJsonObject resp;
  resp[lit("jsonrpc")] = lit("2.0");
  resp[lit("id")] = id;
  resp[lit("error")] = err;
  return resp;
}

AIBridgeWorker::AIBridgeWorker(ICaptureContext *ctx, const QString &token, QObject *parent)
    : QObject(parent), m_Ctx(ctx), m_token(token)
{
}

void AIBridgeWorker::start()
{
  if(m_Server)
    return;

  m_Server = new QTcpServer(this);

  if(!m_Server->listen(QHostAddress::LocalHost, 0))
  {
    emit error(lit("Failed to start localhost listener for AI bridge"));
    m_Server->deleteLater();
    m_Server = NULL;
    return;
  }

  m_port = (uint16_t)m_Server->serverPort();
  QObject::connect(m_Server, &QTcpServer::newConnection, this,
                   &AIBridgeWorker::acceptPendingConnections);

  qInfo() << "RDAI_BRIDGE started port=" << m_port;
  emit started(m_port);
}

void AIBridgeWorker::stop()
{
  if(!m_Server)
  {
    emit stopped();
    return;
  }

  for(QTcpSocket *sock : m_Buffers.keys())
  {
    QObject::disconnect(sock, NULL, this, NULL);
    sock->disconnectFromHost();
    sock->deleteLater();
  }

  m_Buffers.clear();

  m_Server->close();
  m_Server->deleteLater();
  m_Server = NULL;
  m_port = 0;

  qInfo() << "RDAI_BRIDGE stopped";
  emit stopped();
}

void AIBridgeWorker::acceptPendingConnections()
{
  if(!m_Server)
    return;

  while(m_Server->hasPendingConnections())
  {
    QTcpSocket *sock = m_Server->nextPendingConnection();
    if(!sock)
      continue;

    sock->setParent(this);

    m_Buffers.insert(sock, QByteArray());

    QObject::connect(sock, &QTcpSocket::readyRead, this,
                     [this, sock]() { onSocketReadyRead(sock); });
    QObject::connect(sock, &QTcpSocket::disconnected, this, [this, sock]() {
      m_Buffers.remove(sock);
      sock->deleteLater();
    });

    // If the client sent data immediately after connecting, readyRead may not be emitted after we
    // connect the signal, so process any already-buffered bytes.
    if(sock->bytesAvailable() > 0)
      onSocketReadyRead(sock);
  }
}

void AIBridgeWorker::sendResponse(QTcpSocket *socket, int httpCode, const QByteArray &contentType,
                                 const QByteArray &body)
{
  if(!socket)
    return;

  QByteArray resp;
  resp.reserve(256 + body.size());

  resp += "HTTP/1.1 ";
  resp += QByteArray::number(httpCode);
  resp += " ";
  resp += HttpReasonPhrase(httpCode);
  resp += "\r\n";

  if(!contentType.isEmpty())
  {
    resp += "Content-Type: ";
    resp += contentType;
    resp += "\r\n";
  }

  resp += "Content-Length: ";
  resp += QByteArray::number(body.size());
  resp += "\r\n";
  resp += "Connection: close\r\n";
  resp += "\r\n";
  resp += body;

  socket->write(resp);
  socket->disconnectFromHost();
}

void AIBridgeWorker::onSocketReadyRead(QTcpSocket *socket)
{
  if(!socket)
    return;

  QByteArray &buf = m_Buffers[socket];
  buf.append(socket->readAll());

  // Only handle a single request per connection.
  const int headerEnd = buf.indexOf("\r\n\r\n");
  if(headerEnd < 0)
    return;

  const QByteArray headerBlock = buf.left(headerEnd);
  const QByteArray bodyBlock = buf.mid(headerEnd + 4);

  QList<QByteArray> headerLines = headerBlock.split('\n');
  if(headerLines.isEmpty())
  {
    sendResponse(socket, 400, "text/plain", QByteArray());
    return;
  }

  const QByteArray requestLine = headerLines.takeFirst().trimmed();
  const QList<QByteArray> requestParts = requestLine.split(' ');
  if(requestParts.size() < 2)
  {
    sendResponse(socket, 400, "text/plain", QByteArray());
    return;
  }

  const QByteArray httpMethod = requestParts[0];
  const QByteArray httpPath = requestParts[1];

  QHash<QByteArray, QByteArray> headers;
  for(QByteArray raw : headerLines)
  {
    raw.replace('\r', "");
    raw = raw.trimmed();
    if(raw.isEmpty())
      continue;

    const int colon = raw.indexOf(':');
    if(colon <= 0)
      continue;

    const QByteArray key = raw.left(colon).trimmed().toLower();
    const QByteArray val = raw.mid(colon + 1).trimmed();
    headers.insert(key, val);
  }

  bool okLen = false;
  int contentLength = headers.value("content-length").toInt(&okLen);
  if(!okLen || contentLength < 0)
    contentLength = 0;

  if(bodyBlock.size() < contentLength)
    return;

  const QByteArray body = bodyBlock.left(contentLength);

  // Prevent re-processing if more data arrives.
  m_Buffers[socket].clear();

  const qint64 startMs = NowMs();
  QString rpcMethod;
  QString requestId;

  auto finishLog = [&](const char *status) {
    const qint64 elapsed = NowMs() - startMs;
    qInfo() << "RDAI_BRIDGE request_id=" << requestId << "method=" << rpcMethod
            << "status=" << status << "elapsed_ms=" << elapsed;
  };

  if(httpMethod != "POST" || httpPath != "/rpc")
  {
    sendResponse(socket, 404, "text/plain", QByteArray());
    finishLog("error");
    return;
  }

  const QByteArray auth = headers.value("authorization");
  const QByteArray bearerPrefix = "Bearer ";
  const QByteArray presentedToken =
      auth.startsWith(bearerPrefix) ? auth.mid(bearerPrefix.size()).trimmed() : QByteArray();

  if(presentedToken.isEmpty() || presentedToken != m_token.toUtf8())
  {
    QJsonObject resp = MakeJsonRpcError(QJsonValue(QJsonValue::Null), -32001, lit("unauthorized"));
    sendResponse(socket, 401, "application/json",
                 QJsonDocument(resp).toJson(QJsonDocument::Compact));
    finishLog("error");
    return;
  }

  QJsonParseError parseErr = {};
  QJsonDocument doc = QJsonDocument::fromJson(body, &parseErr);
  if(parseErr.error != QJsonParseError::NoError || !doc.isObject())
  {
    QJsonObject resp = MakeJsonRpcError(QJsonValue(QJsonValue::Null), -32700, lit("parse error"),
                                        parseErr.errorString());
    sendResponse(socket, 200, "application/json",
                 QJsonDocument(resp).toJson(QJsonDocument::Compact));
    finishLog("error");
    return;
  }

  const QJsonObject req = doc.object();
  const QJsonValue idVal =
      req.contains(lit("id")) ? req.value(lit("id")) : QJsonValue(QJsonValue::Null);

  if(idVal.isString())
    requestId = idVal.toString();
  else if(idVal.isDouble())
    requestId = QString::number((qint64)idVal.toDouble());

  if(req.value(lit("jsonrpc")).toString() != lit("2.0"))
  {
    QJsonObject resp = MakeJsonRpcError(idVal, -32600, lit("invalid request"));
    sendResponse(socket, 200, "application/json",
                 QJsonDocument(resp).toJson(QJsonDocument::Compact));
    finishLog("error");
    return;
  }

  rpcMethod = req.value(lit("method")).toString();
  if(rpcMethod.isEmpty())
  {
    QJsonObject resp = MakeJsonRpcError(idVal, -32600, lit("invalid request"));
    sendResponse(socket, 200, "application/json",
                 QJsonDocument(resp).toJson(QJsonDocument::Compact));
    finishLog("error");
    return;
  }

  if(rpcMethod == lit("renderdoc.get_context"))
  {
    QJsonObject result;

    if(m_Ctx && m_Ctx->IsCaptureLoaded())
    {
      const QString capturePath = QString(m_Ctx->GetCaptureFilename());
      const QString api = ToQStr(m_Ctx->APIProps().pipelineType);
      const uint32_t eventId = m_Ctx->CurEvent();

      QString eventName;
      const ActionDescription *act = m_Ctx->CurAction();
      if(act)
      {
        if(!act->customName.empty())
          eventName = QString(act->customName);
        else
          eventName = QString(act->GetName(m_Ctx->GetStructuredFile()));
      }

      result[lit("capture_path")] =
          capturePath.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(capturePath);
      result[lit("api")] = api.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(api);
      result[lit("event_id")] = (int)eventId;
      result[lit("event_name")] =
          eventName.isEmpty() ? QJsonValue(QJsonValue::Null) : QJsonValue(eventName);
    }
    else
    {
      result[lit("capture_path")] = QJsonValue(QJsonValue::Null);
      result[lit("api")] = QJsonValue(QJsonValue::Null);
      result[lit("event_id")] = 0;
      result[lit("event_name")] = QJsonValue(QJsonValue::Null);
    }

    QJsonObject resp;
    resp[lit("jsonrpc")] = lit("2.0");
    resp[lit("id")] = idVal;
    resp[lit("result")] = result;

    sendResponse(socket, 200, "application/json",
                 QJsonDocument(resp).toJson(QJsonDocument::Compact));
    finishLog("ok");
    return;
  }

  QJsonObject resp = MakeJsonRpcError(idVal, -32601, lit("method not found"));
  sendResponse(socket, 200, "application/json", QJsonDocument(resp).toJson(QJsonDocument::Compact));
  finishLog("error");
}

AIBridge::AIBridge(ICaptureContext *ctx, QObject *parent) : QObject(parent), m_Ctx(ctx)
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

  // Cross-thread queued connections require argument types to be registered with Qt's meta system.
  qRegisterMetaType<uint16_t>("uint16_t");

  m_token = generateToken();

  QThread *thread = new QThread(this);
  AIBridgeWorker *worker = new AIBridgeWorker(m_Ctx, m_token);

  worker->moveToThread(thread);
  if(worker->thread() != thread)
  {
    emit BridgeError(lit("Failed to move AI bridge worker to thread"));
    thread->deleteLater();
    worker->deleteLater();
    return;
  }

  // QThread::started is emitted from the worker thread. Use a direct connection so we don't depend
  // on the worker thread's event loop being up before start() runs.
  QObject::connect(thread, &QThread::started, worker, &AIBridgeWorker::start, Qt::DirectConnection);
  QObject::connect(worker, &AIBridgeWorker::started, this, &AIBridge::onWorkerStarted);
  QObject::connect(worker, &AIBridgeWorker::error, this, &AIBridge::onWorkerError);
  QObject::connect(thread, &QThread::finished, worker, &QObject::deleteLater);

  m_thread = thread;
  m_worker = worker;

  thread->start();

  // Extra safety: make sure start() is queued to the worker thread even if the started signal is
  // missed for any reason.
  QMetaObject::invokeMethod(worker, "start", Qt::QueuedConnection);
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

#if ENABLE_UNIT_TESTS

#include "3rdparty/catch/catch.hpp"

#include <QEventLoop>
#include <QTimer>

struct HttpResponse
{
  int statusCode = 0;
  QHash<QByteArray, QByteArray> headers;
  QByteArray body;
};

static HttpResponse ParseHttpResponse(const QByteArray &raw)
{
  HttpResponse ret;

  const int headerEnd = raw.indexOf("\r\n\r\n");
  REQUIRE(headerEnd >= 0);

  const QByteArray headerBlock = raw.left(headerEnd);
  ret.body = raw.mid(headerEnd + 4);

  QList<QByteArray> headerLines = headerBlock.split('\n');
  REQUIRE(!headerLines.isEmpty());

  const QByteArray statusLine = headerLines.takeFirst().trimmed();
  const QList<QByteArray> statusParts = statusLine.split(' ');
  REQUIRE(statusParts.size() >= 2);

  bool okStatus = false;
  ret.statusCode = statusParts[1].toInt(&okStatus);
  REQUIRE(okStatus);

  for(QByteArray rawLine : headerLines)
  {
    rawLine.replace('\r', "");
    rawLine = rawLine.trimmed();
    if(rawLine.isEmpty())
      continue;

    const int colon = rawLine.indexOf(':');
    if(colon <= 0)
      continue;

    const QByteArray key = rawLine.left(colon).trimmed().toLower();
    const QByteArray val = rawLine.mid(colon + 1).trimmed();
    ret.headers.insert(key, val);
  }

  bool okLen = false;
  const int contentLen = ret.headers.value("content-length").toInt(&okLen);
  if(okLen && contentLen >= 0 && ret.body.size() >= contentLen)
    ret.body = ret.body.left(contentLen);

  return ret;
}

static QByteArray MakeHttpPostRequest(const QByteArray &path, const QByteArray &authHeaderValue,
                                     const QByteArray &body)
{
  QByteArray req;
  req += "POST ";
  req += path;
  req += " HTTP/1.1\r\n";
  req += "Host: 127.0.0.1\r\n";
  req += "Content-Type: application/json\r\n";
  req += "Content-Length: ";
  req += QByteArray::number(body.size());
  req += "\r\n";

  if(!authHeaderValue.isEmpty())
  {
    req += "Authorization: ";
    req += authHeaderValue;
    req += "\r\n";
  }

  req += "Connection: close\r\n";
  req += "\r\n";
  req += body;
  return req;
}

static QByteArray SendRequest(uint16_t port, const QByteArray &request)
{
  QTcpSocket sock;
  QByteArray resp;
  QEventLoop loop;
  QTimer timer;
  timer.setSingleShot(true);

  QObject::connect(&sock, &QTcpSocket::readyRead, [&sock, &resp]() { resp += sock.readAll(); });
  QObject::connect(&sock, &QTcpSocket::disconnected, &loop, &QEventLoop::quit);
  QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

  sock.connectToHost(QHostAddress::LocalHost, port);

  REQUIRE(sock.waitForConnected(2000));

  const qint64 written = sock.write(request);
  REQUIRE(written == request.size());
  REQUIRE(sock.waitForBytesWritten(2000));

  timer.start(2000);
  loop.exec();

  if(sock.state() != QAbstractSocket::UnconnectedState)
    sock.disconnectFromHost();

  resp += sock.readAll();

  return resp;
}

static QJsonObject ParseJsonObject(const QByteArray &bytes)
{
  QJsonParseError err = {};
  QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
  REQUIRE(err.error == QJsonParseError::NoError);
  REQUIRE(doc.isObject());
  return doc.object();
}

TEST_CASE("AI bridge enforces auth and JSON-RPC responses", "[ai][bridge]")
{
  const QString token = lit("good_token");

  AIBridgeWorker worker(NULL, token);
  uint16_t port = 0;
  QObject::connect(&worker, &AIBridgeWorker::started, [&port](uint16_t p) { port = p; });

  worker.start();
  REQUIRE(port != 0);

  const QJsonObject req = {
      {lit("jsonrpc"), lit("2.0")},
      {lit("id"), lit("x")},
      {lit("method"), lit("renderdoc.get_context")},
  };

  const QByteArray json = QJsonDocument(req).toJson(QJsonDocument::Compact);

  SECTION("rejects missing bearer token")
  {
    const QByteArray httpReq = MakeHttpPostRequest("/rpc", QByteArray(), json);
    const HttpResponse resp = ParseHttpResponse(SendRequest(port, httpReq));
    CHECK(resp.statusCode == 401);

    const QJsonObject obj = ParseJsonObject(resp.body);
    CHECK(obj.value(lit("jsonrpc")).toString() == lit("2.0"));
    CHECK(obj.value(lit("id")).isNull());
    const QJsonObject err = obj.value(lit("error")).toObject();
    CHECK(err.value(lit("code")).toInt() == -32001);
    CHECK(err.value(lit("message")).toString() == lit("unauthorized"));
  }

  SECTION("returns context for authorized requests")
  {
    const QByteArray auth = QByteArray("Bearer ") + token.toUtf8();
    const QByteArray httpReq = MakeHttpPostRequest("/rpc", auth, json);
    const HttpResponse resp = ParseHttpResponse(SendRequest(port, httpReq));
    CHECK(resp.statusCode == 200);

    const QJsonObject obj = ParseJsonObject(resp.body);
    CHECK(obj.value(lit("jsonrpc")).toString() == lit("2.0"));
    CHECK(obj.value(lit("id")).toString() == lit("x"));

    const QJsonObject result = obj.value(lit("result")).toObject();
    CHECK(result.contains(lit("capture_path")));
    CHECK(result.contains(lit("api")));
    CHECK(result.contains(lit("event_id")));
    CHECK(result.contains(lit("event_name")));
    CHECK(result.value(lit("capture_path")).isNull());
    CHECK(result.value(lit("api")).isNull());
    CHECK(result.value(lit("event_id")).toInt() == 0);
    CHECK(result.value(lit("event_name")).isNull());
  }

  SECTION("returns parse error for invalid JSON")
  {
    const QByteArray auth = QByteArray("Bearer ") + token.toUtf8();
    const QByteArray httpReq = MakeHttpPostRequest("/rpc", auth, "{");
    const HttpResponse resp = ParseHttpResponse(SendRequest(port, httpReq));
    CHECK(resp.statusCode == 200);

    const QJsonObject obj = ParseJsonObject(resp.body);
    const QJsonObject err = obj.value(lit("error")).toObject();
    CHECK(err.value(lit("code")).toInt() == -32700);
    CHECK(err.value(lit("message")).toString() == lit("parse error"));
  }

  SECTION("returns invalid request for missing method")
  {
    const QByteArray auth = QByteArray("Bearer ") + token.toUtf8();
    const QJsonObject badReq = {
        {lit("jsonrpc"), lit("2.0")},
        {lit("id"), lit("x")},
    };
    const QByteArray httpReq =
        MakeHttpPostRequest("/rpc", auth, QJsonDocument(badReq).toJson(QJsonDocument::Compact));
    const HttpResponse resp = ParseHttpResponse(SendRequest(port, httpReq));
    CHECK(resp.statusCode == 200);

    const QJsonObject obj = ParseJsonObject(resp.body);
    const QJsonObject err = obj.value(lit("error")).toObject();
    CHECK(err.value(lit("code")).toInt() == -32600);
    CHECK(err.value(lit("message")).toString() == lit("invalid request"));
  }

  SECTION("returns method not found")
  {
    const QByteArray auth = QByteArray("Bearer ") + token.toUtf8();
    const QJsonObject badReq = {
        {lit("jsonrpc"), lit("2.0")},
        {lit("id"), lit("x")},
        {lit("method"), lit("renderdoc.nope")},
    };
    const QByteArray httpReq =
        MakeHttpPostRequest("/rpc", auth, QJsonDocument(badReq).toJson(QJsonDocument::Compact));
    const HttpResponse resp = ParseHttpResponse(SendRequest(port, httpReq));
    CHECK(resp.statusCode == 200);

    const QJsonObject obj = ParseJsonObject(resp.body);
    const QJsonObject err = obj.value(lit("error")).toObject();
    CHECK(err.value(lit("code")).toInt() == -32601);
    CHECK(err.value(lit("message")).toString() == lit("method not found"));
  }

  worker.stop();
}

TEST_CASE("AI bridge starts and becomes ready", "[ai][bridge]")
{
  AIBridge bridge(NULL);

  bool started = false;
  QString startError;
  QEventLoop loop;
  QTimer timer;
  timer.setSingleShot(true);

  QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
  QObject::connect(&bridge, &AIBridge::BridgeStarted, &loop, &QEventLoop::quit);
  QObject::connect(&bridge, &AIBridge::BridgeStarted, [&started]() { started = true; });
  QObject::connect(&bridge, &AIBridge::BridgeError, [&startError, &loop](const QString &msg) {
    startError = msg;
    loop.quit();
  });

  timer.start(10000);
  bridge.Start();

  QThread *thread = bridge.findChild<QThread *>();
  INFO(QFormatStr("bridge_thread=%1 running=%2")
           .arg((quintptr)thread, 0, 16)
           .arg(thread ? (thread->isRunning() ? 1 : 0) : -1));

  loop.exec();

  INFO(startError.toUtf8().data());
  REQUIRE(started);
  REQUIRE(bridge.IsRunning());
  REQUIRE(bridge.Port() != 0);
  REQUIRE(!bridge.RpcUrl().isEmpty());
  REQUIRE(!bridge.Token().isEmpty());

  const QJsonObject req = {
      {lit("jsonrpc"), lit("2.0")},
      {lit("id"), lit("x")},
      {lit("method"), lit("renderdoc.get_context")},
  };

  const QByteArray auth = QByteArray("Bearer ") + bridge.Token().toUtf8();
  const QByteArray httpReq =
      MakeHttpPostRequest("/rpc", auth, QJsonDocument(req).toJson(QJsonDocument::Compact));
  const HttpResponse resp = ParseHttpResponse(SendRequest(bridge.Port(), httpReq));

  CHECK(resp.statusCode == 200);
  const QJsonObject obj = ParseJsonObject(resp.body);
  const QJsonObject result = obj.value(lit("result")).toObject();
  CHECK(result.value(lit("event_id")).toInt() == 0);

  bridge.Stop();
  CHECK(!bridge.IsRunning());
  CHECK(bridge.RpcUrl().isEmpty());
  CHECK(bridge.Token().isEmpty());
}

#endif    // ENABLE_UNIT_TESTS
