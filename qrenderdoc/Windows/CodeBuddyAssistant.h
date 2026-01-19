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

#pragma once

#include <QByteArray>
#include <QFrame>
#include <QProcess>

class QCheckBox;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class AIBridge;
class RDProcess;

struct ICaptureContext;

class CodeBuddyAssistant : public QFrame
{
  Q_OBJECT

public:
  explicit CodeBuddyAssistant(ICaptureContext &ctx, QWidget *parent = NULL);
  ~CodeBuddyAssistant();

  QWidget *Widget() { return this; }

private slots:
  void onSend();
  void onCancel();
  void onStdOutReady();
  void onStdErrReady();
  void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void onErrorOccurred(QProcess::ProcessError error);

private:
  void appendLine(const QString &line);
  void appendSystem(const QString &line);
  void appendError(const QString &line);
  QString buildPrompt(const QString &userPrompt) const;
  QString findNodeExecutable() const;
  QString findAgentHostEntry() const;
  QString defaultWorkDir() const;
  void startQuery(const QString &prompt);
  void processStdOutLine(const QString &line);
  void flushStdOutBuffer();

  ICaptureContext &m_Ctx;

  AIBridge *m_Bridge = NULL;

  RDProcess *m_Process = NULL;
  QByteArray m_StdoutBuffer;

  QPlainTextEdit *m_Output = NULL;
  QPlainTextEdit *m_Input = NULL;
  QLineEdit *m_Model = NULL;
  QCheckBox *m_IncludeContext = NULL;
  QPushButton *m_Send = NULL;
  QPushButton *m_Cancel = NULL;
};
