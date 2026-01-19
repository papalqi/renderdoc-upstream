/******************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 2026 Baldur Karlsson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ******************************************************************************/

#include "CodeBuddyAssistant.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QScrollBar>
#include <QStandardPaths>
#include <QVBoxLayout>

#include "Code/Interface/QRDInterface.h"
#include "Code/QRDUtils.h"
#include "Code/ai_agent/AIBridge.h"

CodeBuddyAssistant::CodeBuddyAssistant(ICaptureContext &ctx, QWidget *parent)
    : QFrame(parent), m_Ctx(ctx)
{
  setWindowTitle(tr("AI Assistant (CodeBuddy)"));
  setObjectName(lit("CodeBuddyAssistant"));

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setSpacing(6);
  layout->setContentsMargins(6, 6, 6, 6);

  m_Output = new QPlainTextEdit(this);
  m_Output->setReadOnly(true);
  m_Output->setUndoRedoEnabled(false);
  m_Output->setLineWrapMode(QPlainTextEdit::WidgetWidth);
  layout->addWidget(m_Output, 1);

  QHBoxLayout *options = new QHBoxLayout();

  m_IncludeContext = new QCheckBox(tr("Include RenderDoc context"), this);
  m_IncludeContext->setChecked(true);
  options->addWidget(m_IncludeContext);

  options->addSpacing(12);
  options->addWidget(new QLabel(tr("Model:"), this));

  m_Model = new QLineEdit(this);
  m_Model->setPlaceholderText(tr("Optional, e.g. gemini-1.5-pro"));
  options->addWidget(m_Model, 1);

  layout->addLayout(options);

  m_Input = new QPlainTextEdit(this);
  m_Input->setPlaceholderText(tr("Ask a question..."));
  m_Input->setMaximumBlockCount(1000);
  m_Input->setFixedHeight(90);
  layout->addWidget(m_Input);

  QHBoxLayout *buttons = new QHBoxLayout();
  buttons->addStretch(1);

  m_Send = new QPushButton(tr("Send"), this);
  m_Cancel = new QPushButton(tr("Cancel"), this);
  m_Cancel->setEnabled(false);

  buttons->addWidget(m_Send);
  buttons->addWidget(m_Cancel);

  layout->addLayout(buttons);

  QObject::connect(m_Send, &QPushButton::clicked, this, &CodeBuddyAssistant::onSend);
  QObject::connect(m_Cancel, &QPushButton::clicked, this, &CodeBuddyAssistant::onCancel);

  m_Bridge = new AIBridge(m_Ctx, this);
  QObject::connect(m_Bridge, &AIBridge::BridgeStarted, this,
                   [this]() { appendSystem(tr("Tool bridge started: %1").arg(m_Bridge->RpcUrl())); });
  QObject::connect(m_Bridge, &AIBridge::BridgeError, this,
                   [this](const QString &msg) { appendError(tr("Tool bridge error: %1").arg(msg)); });
  m_Bridge->Start();

  appendSystem(tr("Backend: Node Agent Host (CodeBuddy Agent SDK)."));
}

CodeBuddyAssistant::~CodeBuddyAssistant()
{
  if(m_Process)
  {
    if(m_Process->state() != QProcess::NotRunning)
      m_Process->kill();
    m_Process->detach();
    m_Process->deleteLater();
    m_Process = NULL;
  }
}

void CodeBuddyAssistant::appendLine(const QString &line)
{
  if(!m_Output)
    return;

  m_Output->appendPlainText(line);
  QScrollBar *scroll = m_Output->verticalScrollBar();
  if(scroll)
    scroll->setValue(scroll->maximum());
}

void CodeBuddyAssistant::appendSystem(const QString &line)
{
  appendLine(lit("[system] ") + line);
}

void CodeBuddyAssistant::appendError(const QString &line)
{
  appendLine(lit("[error] ") + line);
}

QString CodeBuddyAssistant::defaultWorkDir() const
{
  QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  if(base.isEmpty())
    base = QDir::tempPath();

  return QDir(base).filePath(lit("codebuddy-agent-host"));
}

QString CodeBuddyAssistant::findNodeExecutable() const
{
  const QByteArray overridePath = qgetenv("RENDERDOC_AI_NODE_PATH");
  if(!overridePath.isEmpty())
  {
    const QString p = QString::fromUtf8(overridePath);
    if(QFileInfo::exists(p))
      return p;
  }

  const QString appDir = QCoreApplication::applicationDirPath();

#if defined(Q_OS_WIN32)
  const QString bundled = QDir(appDir).filePath(lit("ai/node/node.exe"));
  if(QFileInfo::exists(bundled))
    return bundled;
#endif

  QString exe = QStandardPaths::findExecutable(lit("node"));
  if(!exe.isEmpty())
    return exe;

  return QString();
}

QString CodeBuddyAssistant::findAgentHostEntry() const
{
  const QByteArray overridePath = qgetenv("RENDERDOC_AI_AGENT_HOST_JS");
  if(!overridePath.isEmpty())
  {
    const QString p = QString::fromUtf8(overridePath);
    if(QFileInfo::exists(p))
      return p;
  }

  const QString appDir = QCoreApplication::applicationDirPath();

  const QString deployed = QDir(appDir).filePath(lit("ai/agent-host/main.js"));
  if(QFileInfo::exists(deployed))
    return deployed;

  // Developer fallback: search upwards for the source-tree dist entry.
  QDir dir(appDir);
  for(int i = 0; i < 6; i++)
  {
    const QString candidate = dir.filePath(lit("qrenderdoc/CodeBuddyAgentHost/dist/main.js"));
    if(QFileInfo::exists(candidate))
      return candidate;
    if(!dir.cdUp())
      break;
  }

  return QString();
}

QString CodeBuddyAssistant::buildPrompt(const QString &userPrompt) const
{
  QString prompt = userPrompt;
  prompt = prompt.trimmed();

  if(!m_IncludeContext || !m_IncludeContext->isChecked())
    return prompt;

  uint32_t eid = m_Ctx.CurEvent();

  rdcstr captureRDC = m_Ctx.GetCaptureFilename();
  QString capture = QString::fromUtf8(captureRDC.c_str());
  QString api = ToQStr(m_Ctx.APIProps().pipelineType);
  QString eventName;
  if(m_Ctx.HasEventBrowser() && m_Ctx.GetEventBrowser())
    eventName = QString::fromUtf8(m_Ctx.GetEventBrowser()->GetEventName(eid).c_str());

  QString ctx;
  ctx += lit("RenderDoc context:\n");
  ctx += lit("- Capture: ") + (capture.isEmpty() ? lit("<none>") : capture) + lit("\n");
  ctx += lit("- API: ") + (api.isEmpty() ? lit("<unknown>") : api) + lit("\n");
  ctx += lit("- Current eventId: ") + QString::number(eid) + lit("\n");
  ctx += lit("- Current event name: ") + (eventName.isEmpty() ? lit("<unknown>") : eventName) + lit("\n\n");

  ctx += lit("User question:\n");
  ctx += prompt;
  ctx += lit("\n\n");
  ctx += lit("Answer with concrete RenderDoc steps and name the relevant panel(s). ");
  ctx += lit("Avoid suggesting external tools unless explicitly asked.");

  return ctx;
}

void CodeBuddyAssistant::startQuery(const QString &prompt)
{
  if(prompt.trimmed().isEmpty())
    return;

  if(m_Process)
  {
    if(m_Process->state() != QProcess::NotRunning)
    {
      appendSystem(tr("Canceling previous query..."));
      QObject::disconnect(m_Process, NULL, this, NULL);
      m_Process->kill();
    }

    m_Process->detach();
    m_Process->deleteLater();
    m_Process = NULL;
  }

  const QString nodeExe = findNodeExecutable();
  if(nodeExe.isEmpty())
  {
    appendError(tr("Could not find 'node' executable."));
    appendSystem(tr("Install Node.js or provide RENDERDOC_AI_NODE_PATH."));
    return;
  }

  const QString hostEntry = findAgentHostEntry();
  if(hostEntry.isEmpty())
  {
    appendError(tr("Could not find Agent Host entry (main.js)."));
    appendSystem(tr("Build qrenderdoc/CodeBuddyAgentHost and set RENDERDOC_AI_AGENT_HOST_JS."));
    return;
  }

  if(!m_Bridge || !m_Bridge->IsRunning() || m_Bridge->RpcUrl().isEmpty() || m_Bridge->Token().isEmpty())
  {
    appendError(tr("Tool bridge is not ready."));
    appendSystem(tr("Wait a moment and try again."));
    return;
  }

  const QString workDir = defaultWorkDir();
  QDir().mkpath(workDir);

  QStringList args;
  args << hostEntry << lit("--timeout-ms") << lit("300000");

  const QString model = m_Model ? m_Model->text().trimmed() : QString();
  if(!model.isEmpty())
    args << lit("--model") << model;

  appendSystem(tr("Starting Agent Host..."));
  qInfo() << "CodeBuddyAssistant starting:" << nodeExe << args << "cwd:" << workDir
          << "bridge:" << m_Bridge->RpcUrl();

  m_StdoutBuffer.clear();

  m_Process = new RDProcess(this);
  m_Process->setWorkingDirectory(workDir);
  m_Process->setProcessChannelMode(QProcess::SeparateChannels);

  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  env.insert(lit("RENDERDOC_AI_BRIDGE_URL"), m_Bridge->RpcUrl());
  env.insert(lit("RENDERDOC_AI_BRIDGE_TOKEN"), m_Bridge->Token());
  m_Process->setProcessEnvironment(env);

  QObject::connect(m_Process, &QProcess::readyReadStandardOutput, this, &CodeBuddyAssistant::onStdOutReady);
  QObject::connect(m_Process, &QProcess::readyReadStandardError, this, &CodeBuddyAssistant::onStdErrReady);
  QObject::connect(m_Process, OverloadedSlot<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
                   &CodeBuddyAssistant::onFinished);
  QObject::connect(m_Process, &QProcess::errorOccurred, this, &CodeBuddyAssistant::onErrorOccurred);

  m_Process->start(nodeExe, args);

  if(!m_Process->waitForStarted(5000))
  {
    appendError(tr("Failed to start Agent Host process."));
    appendSystem(m_Process->errorString());
    m_Process->deleteLater();
    m_Process = NULL;
    return;
  }

  m_Send->setEnabled(false);
  m_Cancel->setEnabled(true);

  QByteArray bytes = prompt.toUtf8();
  if(!bytes.endsWith('\n'))
    bytes.append('\n');

  m_Process->write(bytes);
  m_Process->closeWriteChannel();
}

void CodeBuddyAssistant::processStdOutLine(const QString &line)
{
  if(line.trimmed().isEmpty())
    return;

  QJsonParseError err = {};
  QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8(), &err);
  if(err.error != QJsonParseError::NoError || !doc.isObject())
  {
    appendLine(line);
    return;
  }

  QJsonObject obj = doc.object();
  const QString type = obj.value(lit("type")).toString();

  if(type == lit("init"))
  {
    const QString session = obj.value(lit("session_id")).toString();
    const QString model = obj.value(lit("model")).toString();
    appendSystem(tr("Session: %1  Model: %2").arg(session, model));
  }
  else if(type == lit("error"))
  {
    const QString message = obj.value(lit("message")).toString();
    appendError(message.trimmed());
  }
  else if(type == lit("message"))
  {
    const QString role = obj.value(lit("role")).toString();
    const QString content = obj.value(lit("content")).toVariant().toString();

    if(role == lit("assistant"))
      appendLine(lit("[assistant] ") + content.trimmed());
    else if(role == lit("user"))
      appendLine(lit("[user] ") + content.trimmed());
    else
      appendLine(lit("[") + role + lit("] ") + content.trimmed());
  }
  else if(type == lit("result"))
  {
    const QString status = obj.value(lit("status")).toString();
    appendSystem(tr("Result: %1").arg(status));
  }
  else if(type == lit("tool_call"))
  {
    const QString name = obj.value(lit("name")).toString();
    const QString callId = obj.value(lit("call_id")).toString();
    const QString args = QString::fromUtf8(
        QJsonDocument(obj.value(lit("arguments")).toObject()).toJson(QJsonDocument::Compact));
    appendSystem(tr("Tool call: %1 (%2) %3").arg(name, callId, args));
  }
  else if(type == lit("tool_result"))
  {
    const QString name = obj.value(lit("name")).toString();
    const QString callId = obj.value(lit("call_id")).toString();
    const bool ok = obj.value(lit("ok")).toBool();
    if(ok)
      appendSystem(tr("Tool result: %1 (%2) ok").arg(name, callId));
    else
      appendError(tr("Tool result: %1 (%2) error").arg(name, callId));
  }
  else
  {
    appendLine(QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
  }
}

void CodeBuddyAssistant::flushStdOutBuffer()
{
  while(true)
  {
    int newline = m_StdoutBuffer.indexOf('\n');
    if(newline < 0)
      break;

    QByteArray line = m_StdoutBuffer.left(newline);
    m_StdoutBuffer.remove(0, newline + 1);

    processStdOutLine(QString::fromUtf8(line).trimmed());
  }
}

void CodeBuddyAssistant::onSend()
{
  if(!m_Input)
    return;

  QString userPrompt = m_Input->toPlainText();
  userPrompt = userPrompt.trimmed();
  if(userPrompt.isEmpty())
    return;

  appendLine(lit("[you] ") + userPrompt);

  QString prompt = buildPrompt(userPrompt);

  m_Input->clear();

  startQuery(prompt);
}

void CodeBuddyAssistant::onCancel()
{
  if(!m_Process)
    return;

  appendSystem(tr("Cancel requested..."));
  m_Process->kill();
}

void CodeBuddyAssistant::onStdOutReady()
{
  if(!m_Process)
    return;

  m_StdoutBuffer.append(m_Process->readAllStandardOutput());
  flushStdOutBuffer();
}

void CodeBuddyAssistant::onStdErrReady()
{
  if(!m_Process)
    return;

  QString text = QString::fromUtf8(m_Process->readAllStandardError());
  QStringList lines = text.split(QLatin1Char('\n'));
  for(QString l : lines)
  {
    l = l.trimmed();
    if(!l.isEmpty())
      appendError(l);
  }
}

void CodeBuddyAssistant::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  Q_UNUSED(exitStatus);
  flushStdOutBuffer();

  if(!m_StdoutBuffer.isEmpty())
  {
    processStdOutLine(QString::fromUtf8(m_StdoutBuffer).trimmed());
    m_StdoutBuffer.clear();
  }

  appendSystem(tr("Process finished (exit code %1).").arg(exitCode));

  m_Send->setEnabled(true);
  m_Cancel->setEnabled(false);

  if(m_Process)
  {
    m_Process->detach();
    m_Process->deleteLater();
    m_Process = NULL;
  }
}

void CodeBuddyAssistant::onErrorOccurred(QProcess::ProcessError error)
{
  Q_UNUSED(error);
  if(!m_Process)
    return;

  appendError(m_Process->errorString());

  m_Send->setEnabled(true);
  m_Cancel->setEnabled(false);
}
