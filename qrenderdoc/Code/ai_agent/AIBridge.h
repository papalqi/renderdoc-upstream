#pragma once

#include <QObject>
#include <QString>

#include <stdint.h>

struct ICaptureContext;
struct PythonContextHandle;

class QThread;

class AIBridgeWorker : public QObject
{
  Q_OBJECT

public:
  AIBridgeWorker(ICaptureContext &ctx, const QString &token, QObject *parent = NULL);

public slots:
  void start();
  void stop();

signals:
  void started(uint16_t port);
  void stopped();
  void error(const QString &message);

private:
  ICaptureContext &m_Ctx;
  QString m_token;

  uint16_t m_port = 0;

  bool m_starting = false;
  QString m_startError;

  PythonContextHandle *m_py = NULL;
};

class AIBridge : public QObject
{
  Q_OBJECT

public:
  explicit AIBridge(ICaptureContext &ctx, QObject *parent = NULL);
  ~AIBridge();

  bool IsRunning() const { return m_running; }
  uint16_t Port() const { return m_port; }
  QString Token() const { return m_token; }
  QString RpcUrl() const;

  void Start();
  void Stop();

signals:
  void BridgeStarted();
  void BridgeStopped();
  void BridgeError(const QString &message);

private slots:
  void onWorkerStarted(uint16_t port);
  void onWorkerError(const QString &message);

private:
  static QString generateToken();

  ICaptureContext &m_Ctx;

  QThread *m_thread = NULL;
  AIBridgeWorker *m_worker = NULL;

  bool m_running = false;
  uint16_t m_port = 0;
  QString m_token;
};
