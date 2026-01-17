---
title: "CodeBuddy Agent SDK：为你的应用注入 AI Agent 能力 - CodeBuddy - KM平台"
source: "https://km.woa.com/articles/show/651033?kmref=search&from_page=1&no=1"
author:
  - "[[从 Claude Code 到 CodeBuddy Code：Skills 迁移指南]]"
  - "[[国产最佳 Claude Code 替代产品 CodeBuddy Code]]"
  - "[[CodeBuddy Code 自举：一句话搞定所有配置]]"
published:
created: 2026-01-17
description:
tags:
  - "clippings"
---
2026-01-16 14:42

570

8

39

分享

文章摘要

思维导图

文章朗读

在 AI Agent 快速发展的今天，如何将强大的 Agent 能力集成到现有软件系统中，成为开发者和企业面临的关键挑战。 **CodeBuddy Agent SDK** 正是为此而生——它提供了一套完整的编程接口，让你能够将 CodeBuddy Agent 的能力嵌入到任何 TypeScript/JavaScript 或 Python 应用中。

CodeBuddy Agent 不仅能编写和修改代码，还能读取文件、执行命令、搜索内容、与 MCP 服务交互——这是一个具备工具使用能力的通用 AI Agent。通过 SDK，你可以将这些能力程序化地集成到自己的产品中。

无论你是在构建 AI Infra 基础设施、开发需要被其他系统集成的 AI 服务，还是希望将传统软件系统升级改造为具备 AI 智能化能力的现代应用，CodeBuddy Agent SDK 都能为你提供灵活、可控、安全的解决方案。

![](https://km.woa.com/asset/000100022601003c3e0b986335463f01?height=1688&width=1706&imageMogr2/thumbnail/1540x%3E/ignore-error/1)

## 为什么需要 Agent SDK

当我们谈论 AI 助手时，大多数人首先想到的是聊天界面或命令行工具。但对于企业级应用和复杂系统来说，这远远不够。

**传统集成方式的局限** ：直接调用 LLM API 虽然简单，但你需要自己处理工具调用、上下文管理、权限控制、多轮对话等复杂逻辑。而 CodeBuddy Agent SDK 将这些能力封装为开箱即用的接口，让你专注于业务逻辑。

### 程序化控制：超越命令行

- **嵌入式集成** ：在你的应用程序中嵌入完整的 AI 编程助手，而非独立运行的工具
- **自定义交互** ：构建符合你产品需求的用户界面和交互方式
- **批量自动化** ：对多个文件、项目甚至代码仓库执行批量 AI 操作
- **系统集成** ：将 AI 能力无缝集成到 CI/CD 流水线、IDE 插件、内部开发平台等

### 企业级管控

- **权限管控** ：通过 `canUseTool` 回调实现企业级权限策略，精确控制 AI 能做什么、不能做什么
- **行为定制** ：使用 Hook 系统拦截和修改 Agent 行为，满足合规和审计需求
- **资源限制** ：控制 token 消耗、执行时间和费用预算，防止资源滥用
- **会话管理** ：持久化和恢复对话上下文，支持长时间运行的任务

### 可扩展架构

- **自定义 Agent** ：创建专门化的子 Agent 处理特定领域任务，如代码审查、安全扫描、文档生成
- **MCP 集成** ：通过 Model Context Protocol 接入数据库、API、内部服务等自定义工具
- **多模型支持** ：灵活切换和配置不同的 AI 模型，适应不同场景的性能和成本需求

## 典型应用场景

CodeBuddy Agent SDK 的设计目标是成为连接 AI 能力与现有软件系统的桥梁。无论你是个人开发者、创业团队还是大型企业，都能找到适合的应用方式。

### 场景一：个人开发者效率提升

即使你只是一个人，SDK 也能帮你打造专属的 AI 工作流：

- **个人自动化脚本** ：编写脚本让 AI 批量处理重复性编码任务，如格式化、重命名、添加注释
- **学习助手** ：构建交互式编程学习工具，让 AI 解释代码、回答问题、提供练习反馈
- **Side Project 加速器** ：快速搭建原型，让 AI 帮你生成样板代码、配置文件、测试用例
- **博客/教程生成** ：自动分析你的代码项目，生成技术博客、教程文档或 README

### 场景二：泛开发者与低代码场景

不仅仅是专业程序员，产品经理、数据分析师、运营人员也能借助 SDK 获得 AI 编程能力：

- **数据分析自动化** ：让非技术人员用自然语言描述需求，AI 自动生成 Python/SQL 分析脚本
- **Excel/报表处理** ：批量处理数据文件，生成可视化图表和分析报告
- **低代码平台增强** ：为低代码/无代码平台注入 AI 能力，让用户用自然语言描述逻辑
- **运营工具定制** ：快速生成爬虫脚本、数据清洗工具、批量处理程序
- **配置文件生成** ：通过对话生成 Docker、K8s、Nginx 等复杂配置文件

### 场景三：构建 AI Infra 基础设施

如果你正在构建面向开发者的 AI 基础设施，SDK 可以作为核心组件：

- **AI 编程平台** ：构建类似 Cursor、Windsurf 的 AI 编程产品，SDK 提供完整的 Agent 能力
- **多租户 SaaS 服务** ：为不同客户提供隔离的 AI 编程环境，通过权限控制确保安全
- **API 网关** ：将 Agent 能力封装为 RESTful 或 gRPC 服务，供内部或外部系统调用

### 场景四：传统系统智能化升级

将 AI 能力注入现有系统，实现智能化转型：

- **IDE 插件开发** ：为 VS Code、JetBrains、Vim/Neovim 等编辑器构建智能编程助手
- **DevOps 智能化** ：在 CI/CD 流水线中集成 AI 代码审查、自动修复、测试生成
- **运维自动化** ：让 AI 协助分析日志、诊断问题、生成修复脚本
- **遗留代码现代化** ：批量重构、迁移技术栈、补充文档和测试

### 场景五：企业内部工具

满足企业特定需求的定制化开发：

- **内部开发者门户** ：统一的 AI 编程入口，集成企业知识库和规范
- **代码合规检查** ：结合企业安全策略，自动检测和修复合规问题
- **新人培训系统** ：交互式编程学习，AI 实时指导和代码评审
- **技术债务治理** ：智能识别和清理技术债务，生成重构建议

### 场景六：垂直领域应用

针对特定技术栈或业务领域的深度定制：

- **数据库助手** ：结合 MCP 接入数据库，实现智能 SQL 生成和优化
- **API 文档生成** ：自动分析代码生成 OpenAPI 规范和使用示例
- **微服务治理** ：分析服务依赖、生成架构图、优化服务拆分
- **安全扫描集成** ：将 AI 分析与 SAST/DAST 工具结合，提供智能修复建议

## 核心能力一览

SDK 提供了完整的 Agent 编程接口：

| 能力 | 说明 |
| --- | --- |
| **消息流式传输** | 实时接收系统消息、助手响应和工具调用结果 |
| **多轮对话** | 支持跨多次推理调用的对话上下文保持 |
| **会话管理** | 通过会话 ID 继续或恢复现有对话 |
| **权限控制** | 细粒度的工具访问权限管理，支持自定义策略 |
| **Hook 系统** | 在工具执行前后插入自定义逻辑，满足审计和合规需求 |
| **自定义 Agent** | 定义专门化的子 Agent 处理特定任务 |
| **MCP 集成** | 支持配置自定义 MCP 服务器扩展功能 |
| **环境隔离** | SDK 默认不加载文件系统配置，确保行为可预测 |

- **Hook 系统** ：在工具执行前后插入自定义逻辑
- **自定义 Agent** ：定义专门化的子 Agent 处理特定任务
- **MCP 集成** ：支持配置自定义 MCP 服务器扩展功能

## 安装

```javascript
npm install @tencent-ai/agent-sdk

# 或

yarn add @tencent-ai/agent-sdk

# 或

pnpm add @tencent-ai/agent-sdk
```

```javascript
uv add codebuddy-agent-sdk

# 或

pip install codebuddy-agent-sdk
```

### 环境要求

| 语言 | 版本要求 |
| --- | --- |
| TypeScript/JavaScript | Node.js >= 18.20 |
| Python | Python >= 3.10 |

### 认证配置

#### 使用已有登录凭据

如果你已经在终端中通过 `codebuddy` 命令完成了交互式登录，SDK 会自动使用该认证信息，无需额外配置。

#### 使用 API Key

如果未登录或需要使用不同的凭据，可以通过 API Key 认证：

```bash
export CODEBUDDY_API_KEY="your-api-key"
```

也可以在代码中通过 `env` 选项传递：

```javascript
const q = query({

  prompt: '...',

  options: {

    env: {

      CODEBUDDY_API_KEY: process.env.MY_API_KEY

    }

  }

});
```

```javascript
options = CodeBuddyAgentOptions(

    env={

        "CODEBUDDY_API_KEY": os.environ.get("MY_API_KEY")

    }

)
```

#### 企业用户：OAuth Client Credentials

> 目前仅介绍 Client Credentials 授权方式，适用于服务端应用和 CI/CD 场景。

企业用户需要先通过 OAuth 2.0 Client Credentials 流程获取 access token，然后传入 SDK。

**第 1 步：创建应用获取凭据**

参考 [企业开发者快速入门](https://copilot.tencent.com/apiDocs/open-platform.html) 创建应用并获取 Client ID 和 Client Secret。

**第 2 步：获取 token 并调用 SDK**

```javascript
import httpx

from codebuddy_agent_sdk import query, CodeBuddyAgentOptions

async def get_oauth_token(client_id: str, client_secret: str) -> str:

    async with httpx.AsyncClient() as client:

        response = await client.post(

            "https://copilot.tencent.com/oauth2/token",

            data={

                "grant_type": "client_credentials",

                "client_id": client_id,

                "client_secret": client_secret,

            },

        )

        return response.json()["access_token"]

# 获取 token 并调用 SDK

token = await get_oauth_token("your-client-id", "your-client-secret")

options = CodeBuddyAgentOptions(

    env={"CODEBUDDY_AUTH_TOKEN": token}

)

async for msg in query(prompt="Hello", options=options):

    print(msg)
```

详细的认证配置说明请参阅 \[身份认证\](iam.md#认证方法）。

### 其他环境变量

| 变量名 | 说明 | 必需 |
| --- | --- | --- |
| `CODEBUDDY_CODE_PATH` | CodeBuddy CLI 可执行文件路径 | 可选 |

如果未设置，SDK 会自动尝试查找 CLI。

## 基础用法

### 简单查询

最基础的用法是发送一个提示词并处理响应：

```javascript
import { query } from '@tencent-ai/agent-sdk';

async function main() {

  const q = query({

    prompt: '请解释什么是递归函数',

    options: {

      permissionMode: 'bypassPermissions'

    }

  });

  for await (const message of q) {

    if (message.type === 'assistant') {

      for (const block of message.message.content) {

        if (block.type === 'text') {

          console.log(block.text);

        }

      }

    }

  }

}

main();
```

```javascript
import asyncio

from codebuddy_agent_sdk import query, CodeBuddyAgentOptions

from codebuddy_agent_sdk import AssistantMessage, TextBlock

async def main():

    options = CodeBuddyAgentOptions(

        permission_mode="bypassPermissions"

    )

    async for message in query(prompt="请解释什么是递归函数", options=options):

        if isinstance(message, AssistantMessage):

            for block in message.content:

                if isinstance(block, TextBlock):

                    print(block.text)

asyncio.run(main())
```

### 提取结果

查询完成后，会收到一个 `result` 消息，包含执行统计信息：

```javascript
for await (const message of q) {

  if (message.type === 'result') {

    if (message.subtype === 'success') {

      console.log('完成！耗时：', message.duration_ms, 'ms');

      console.log('费用：', message.total_cost_usd, 'USD');

    } else {

      console.log('执行出错');

    }

  }

}
```

```javascript
from codebuddy_agent_sdk import ResultMessage

async for message in query(prompt="...", options=options):

    if isinstance(message, ResultMessage):

        if message.subtype == "success":

            print(f"完成！耗时： {message.duration_ms} ms")

            print(f"费用： {message.total_cost_usd} USD")

        else:

            print("执行出错")
```

### 消息类型处理

SDK 返回多种类型的消息：

```javascript
for await (const message of q) {

  switch (message.type) {

    case 'system':

      // 会话初始化消息

      console.log('会话 ID:', message.session_id);

      console.log('可用工具：', message.tools);

      break;

    case 'assistant':

      // AI 助手响应

      for (const block of message.message.content) {

        if (block.type === 'text') {

          console.log('[文本]', block.text);

        } else if (block.type === 'tool_use') {

          console.log('[工具调用]', block.name, block.input);

        } else if (block.type === 'tool_result') {

          console.log('[工具结果]', block.content);

        }

      }

      break;

    case 'result':

      // 查询完成

      console.log('执行完成，耗时：', message.duration_ms, 'ms');

      break;

  }

}
```

```javascript
from codebuddy_agent_sdk import (

    SystemMessage, AssistantMessage, ResultMessage,

    TextBlock, ToolUseBlock, ToolResultBlock

)

async for message in query(prompt="...", options=options):

    if isinstance(message, SystemMessage):

        # 会话初始化消息

        print(f"会话 ID: {message.data.get('session_id')}")

        print(f"可用工具： {message.data.get('tools')}")

    elif isinstance(message, AssistantMessage):

        # AI 助手响应

        for block in message.content:

            if isinstance(block, TextBlock):

                print(f"[文本] {block.text}")

            elif isinstance(block, ToolUseBlock):

                print(f"[工具调用] {block.name}: {block.input}")

            elif isinstance(block, ToolResultBlock):

                print(f"[工具结果] {block.content}")

    elif isinstance(message, ResultMessage):

        # 查询完成

        print(f"执行完成，耗时： {message.duration_ms} ms")
```

## 配置选项

### 权限模式

通过 `permissionMode` 控制工具调用的权限行为：

| 模式 | 说明 |
| --- | --- |
| `default` | 默认模式，所有操作需确认 |
| `acceptEdits` | 自动批准文件编辑，Bash 仍需确认 |
| `plan` | 规划模式，仅允许读取操作 |
| `bypassPermissions` | 跳过所有权限检查（谨慎使用） |

```javascript
const q = query({

  prompt: '分析项目结构',

  options: {

    permissionMode: 'plan'  // 只读模式

  }

});
```

```javascript
options = CodeBuddyAgentOptions(

    permission_mode="plan"  # 只读模式

)

async for msg in query(prompt="分析项目结构", options=options):

    pass
```

### 工作目录

指定 Agent 的工作目录：

```javascript
const q = query({

  prompt: '读取 package.json',

  options: {

    cwd: '/path/to/project'

  }

});
```

```javascript
options = CodeBuddyAgentOptions(

    cwd="/path/to/project"

)
```

### 模型选择

指定使用的 AI 模型：

```javascript
const q = query({

  prompt: '...',

  options: {

    model: 'deepseek-v3.1',

    fallbackModel: 'deepseek-v3.1'

  }

});
```

```javascript
options = CodeBuddyAgentOptions(

    model="deepseek-v3.1",

    fallback_model="deepseek-v3.1"

)
```

### 资源限制

限制执行范围：

```javascript
const q = query({

  prompt: '...',

  options: {

    maxTurns：20         // 最大对话轮数

  }

});
```

```javascript
options = CodeBuddyAgentOptions(

    max_turns=20,        # 最大对话轮数

)
```

## 环境隔离（settingSources）

### 设计理念

SDK 默认 **不加载任何文件系统配置** ，提供完全干净的运行环境。这是与 CLI 直接使用的关键区别。

### 为什么这样设计？

1. **可预测性** ：SDK 应用的行为完全由代码控制，不受用户或项目配置文件影响
2. **隔离性** ：避免用户的个人偏好或项目设置干扰 SDK 应用的逻辑
3. **安全性** ：敏感配置（如 hooks、权限规则）不会意外泄露到 SDK 环境
4. **一致性** ：在不同机器上运行时，行为保持一致

### 默认行为对比

| 场景 | Settings | Memory | MCP |
| --- | --- | --- | --- |
| SDK 调用（默认） | ✗ 不加载 | ✗ 不加载 | ✗ 不加载 |
| CLI 直接运行 | ✓ 加载全部 | ✓ 加载全部 | ✓ 加载全部 |

### 显式加载配置

如需加载文件系统配置，使用 `settingSources` 显式指定：

```javascript
const q = query({

  prompt: '...',

  options: {

    // 加载项目配置（.codebuddy/settings.json, CODEBUDDY.md）

    settingSources: ['project'],

    // 或加载全部配置

    // settingSources: ['user', 'project', 'local']

  }

});
```

```javascript
options = CodeBuddyAgentOptions(

    # 加载项目配置

    setting_sources=["project"],

    # 或加载全部配置

    # setting_sources=["user", "project", "local"]

)
```

### 配置源说明

| 值 | 说明 | 位置 |
| --- | --- | --- |
| `'user'` | 全局用户设置 | `~/.codebuddy/settings.json`, `~/.codebuddy/CODEBUDDY.md` |
| `'project'` | 项目共享设置 | `.codebuddy/settings.json`, `CODEBUDDY.md` |
| `'local'` | 项目本地设置 | `.codebuddy/settings.local.json`, `CODEBUDDY.local.md` |

### 典型用例

**CI/CD 环境** ：

```javascript
// 只加载项目配置，忽略用户和本地配置

const q = query({

  prompt: '运行测试',

  options: {

    settingSources: ['project'],

    permissionMode: 'bypassPermissions'

  }

});
```

```javascript
# 只加载项目配置，忽略用户和本地配置

options = CodeBuddyAgentOptions(

    setting_sources=["project"],

    permission_mode="bypassPermissions"

)
```

**完全程序化控制** ：

```javascript
// 默认行为：不加载任何配置

// 所有行为通过 options 显式定义

const q = query({

  prompt: '...',

  options: {

    agents: { /* 自定义 agent */ },

    mcpServers: { /* 自定义 MCP */ },

    allowedTools: ['Read', 'Grep', 'Glob']

  }

});
```

```javascript
# 默认行为：不加载任何配置

# 所有行为通过 options 显式定义

options = CodeBuddyAgentOptions(

    agents={"reviewer": AgentDefinition(...)},

    mcp_servers={"db": {...}},

    allowed_tools=["Read", "Grep", "Glob"]

)
```

## 权限控制

### canUseTool 回调

通过 `canUseTool` 回调实现细粒度权限控制：

```javascript
import { query } from '@tencent-ai/agent-sdk';

const q = query({

  prompt: '分析项目结构',

  options: {

    canUseTool: async (toolName, input, options) => {

      // 只允许读取类工具

      const readOnlyTools = ['Read', 'Glob', 'Grep'];

      if (readOnlyTools.includes(toolName)) {

        return {

          behavior: 'allow',

          updatedInput: input

        };

      }

      // 拒绝其他工具

      return {

        behavior: 'deny',

        message: \`工具 ${toolName} 不允许使用\`

      };

    }

  }

});
```

```javascript
from codebuddy_agent_sdk import (

    query, CodeBuddyAgentOptions,

    CanUseToolOptions, PermissionResultAllow, PermissionResultDeny

)

async def can_use_tool(

    tool_name: str,

    input_data: dict,

    options: CanUseToolOptions

):

    # 只允许读取类工具

    read_only_tools = ["Read", "Glob", "Grep"]

    if tool_name in read_only_tools:

        return PermissionResultAllow(updated_input=input_data)

    # 拒绝其他工具

    return PermissionResultDeny(

        message=f"工具 {tool_name} 不允许使用"

    )

options = CodeBuddyAgentOptions(can_use_tool=can_use_tool)
```

### 拦截危险操作

结合权限回调拦截危险命令：

```javascript
const dangerousCommands = ['rm -rf', 'sudo', 'chmod 777'];

const q = query({

  prompt: '清理临时文件',

  options: {

    canUseTool: async (toolName, input) => {

      if (toolName === 'Bash') {

        const command = input.command as string;

        for (const dangerous of dangerousCommands) {

          if (command.includes(dangerous)) {

            return {

              behavior: 'deny',

              message: \`危险命令被拦截: ${dangerous}\`,

              interrupt: true  // 中断整个会话

            };

          }

        }

      }

      return { behavior: 'allow', updatedInput: input };

    }

  }

});
```

```javascript
dangerous_commands = ["rm -rf", "sudo", "chmod 777"]

async def can_use_tool(tool_name, input_data, options):

    if tool_name == "Bash":

        command = input_data.get("command", "")

        for dangerous in dangerous_commands:

            if dangerous in command:

                return PermissionResultDeny(

                    message=f"危险命令被拦截： {dangerous}",

                    interrupt=True  # 中断整个会话

                )

    return PermissionResultAllow(updated_input=input_data)
```

## 多轮对话

### 使用 Session/Client API

对于需要多轮交互的场景，使用 Session（TypeScript）或 Client（Python）API：

```javascript
import { unstable_v2_createSession } from '@tencent-ai/agent-sdk';

async function main() {

  const session = unstable_v2_createSession({

    model: 'deepseek-v3.1'

  });

  // 第一轮对话

  await session.send('分析这个项目的架构');

  for await (const message of session.stream()) {

    console.log(message);

  }

  // 第二轮对话（保持上下文）

  await session.send('请详细解释第三点');

  for await (const message of session.stream()) {

    console.log(message);

  }

  session.close();

}
```

```javascript
from codebuddy_agent_sdk import CodeBuddySDKClient, CodeBuddyAgentOptions

async def main():

    options = CodeBuddyAgentOptions(model="deepseek-v3.1")

    async with CodeBuddySDKClient(options=options) as client:

        # 第一轮对话

        await client.query("分析这个项目的架构")

        async for message in client.receive_response():

            print(message)

        # 第二轮对话（保持上下文）

        await client.query("请详细解释第三点")

        async for message in client.receive_response():

            print(message)

asyncio.run(main())
```

### 中断执行

在运行过程中中断执行：

```javascript
const q = query({ prompt: '执行长时间任务...' });

let count = 0;

for await (const message of q) {

  if (message.type === 'assistant') {

    for (const block of message.message.content) {

      if (block.type === 'tool_use') {

        count++;

        if (count >= 10) {

          await q.interrupt();  // 中断执行

          break;

        }

      }

    }

  }

}
```

```javascript
async with CodeBuddySDKClient(options=options) as client:

    await client.query("执行长时间任务...")

    count = 0

    async for message in client.receive_messages():

        if isinstance(message, AssistantMessage):

            for block in message.content:

                if isinstance(block, ToolUseBlock):

                    count += 1

                    if count >= 10:

                        await client.interrupt()  # 中断执行

                        break
```

## Hook 系统

Hook 允许在工具执行前后插入自定义逻辑。

### PreToolUse Hook

在工具执行前拦截和处理：

```javascript
const q = query({

  prompt: '清理临时文件',

  options: {

    hooks: {

      PreToolUse: [{

        matcher: 'Bash',  // 只匹配 Bash 工具

        hooks: [

          async (input, toolUseId) => {

            console.log('即将执行命令：', input.command);

            // 可以阻止执行

            if (input.command.includes('rm')) {

              return {

                decision: 'block',

                reason: '删除命令被阻止'

              };

            }

            return { continue: true };

          }

        ]

      }]

    }

  }

});
```

```javascript
from codebuddy_agent_sdk import HookMatcher, HookContext

async def pre_tool_hook(input_data, tool_use_id, context: HookContext):

    print(f"即将执行命令： {input_data.get('command')}")

    # 可以阻止执行

    if "rm" in input_data.get("command", ""):

        return {"continue_": False, "reason": "删除命令被阻止"}

    return {"continue_": True}

options = CodeBuddyAgentOptions(

    hooks={

        "PreToolUse": [

            HookMatcher(matcher="Bash", hooks=[pre_tool_hook])

        ]

    }

)
```

### Hook 事件类型

| 事件 | 触发时机 |
| --- | --- |
| `PreToolUse` | 工具执行前 |
| `PostToolUse` | 工具执行成功后 |
| `PostToolUseFailure` | 工具执行失败后 |
| `UserPromptSubmit` | 用户提交提示词 |
| `SessionStart` | 会话开始 |
| `SessionEnd` | 会话结束 |

## 扩展能力

### 自定义 Agent

定义专门化的子 Agent：

```javascript
const q = query({

  prompt: '使用 code-reviewer 审查代码',

  options: {

    agents: {

      'code-reviewer': {

        description: '专业代码审查助手',

        tools: ['Read', 'Glob', 'Grep'],  // 只允许读取

        disallowedTools: ['Bash', 'Write', 'Edit'],

        prompt: \`你是代码审查专家，请检查：

1. 代码规范

2. 潜在 bug

3. 性能问题

4. 安全漏洞\`,

        model: 'deepseek-v3.1'

      }

    }

  }

});
```

```javascript
from codebuddy_agent_sdk import AgentDefinition

options = CodeBuddyAgentOptions(

    agents={

        "code-reviewer": AgentDefinition(

            description="专业代码审查助手",

            tools=["Read", "Glob", "Grep"],  # 只允许读取

            disallowed_tools=["Bash", "Write", "Edit"],

            prompt="""你是代码审查专家，请检查：

1. 代码规范

2. 潜在 bug

3. 性能问题

4. 安全漏洞""",

            model="deepseek-v3.1"

        )

    }

)
```

### MCP 服务器配置

集成自定义 MCP 服务器：

```javascript
const q = query({

  prompt: '查询数据库',

  options: {

    mcpServers: {

      'database': {

        type: 'stdio',

        command: 'node',

        args: ['./mcp-servers/db-server.js'],

        env: {

          DB_HOST: 'localhost',

          DB_PORT: '5432'

        }

      }

    }

  }

});
```

```javascript
options = CodeBuddyAgentOptions(

    mcp_servers={

        "database": {

            "type": "stdio",

            "command": "node",

            "args": ["./mcp-servers/db-server.js"],

            "env": {

                "DB_HOST": "localhost",

                "DB_PORT": "5432"

            }

        }

    }

)
```

### 处理 AskUserQuestion

AI 可能会通过 `AskUserQuestion` 工具向用户提问，可以在权限回调中处理：

```javascript
const q = query({

  prompt: '配置数据库连接',

  options: {

    canUseTool: async (toolName, input) => {

      if (toolName === 'AskUserQuestion') {

        const questions = input.questions as any[];

        const answers: Record<string, string> = {};

        for (const q of questions) {

          console.log(\`问题: ${q.question}\`);

          // 这里可以接入实际的用户交互

          answers[q.question] = q.options[0].label;

        }

        return {

          behavior: 'allow',

          updatedInput: { ...input, answers }

        };

      }

      return { behavior: 'allow', updatedInput: input };

    }

  }

});
```

```javascript
async def can_use_tool(tool_name, input_data, options):

    if tool_name == "AskUserQuestion":

        questions = input_data.get("questions", [])

        answers = {}

        for q in questions:

            print(f"问题： {q['question']}")

            # 这里可以接入实际的用户交互

            answers[q["question"]] = q["options"][0]["label"]

        return PermissionResultAllow(

            updated_input={**input_data, "answers": answers}

        )

    return PermissionResultAllow(updated_input=input_data)
```

## 错误处理

```javascript
import { query, AbortError } from '@tencent-ai/agent-sdk';

try {

  const q = query({ prompt: '...' });

  for await (const message of q) {

    // ...

  }

} catch (error) {

  if (error instanceof AbortError) {

    console.log('操作被中止');

  } else {

    console.error('发生错误：', error);

  }

}
```

```javascript
from codebuddy_agent_sdk import (

    query, CodeBuddySDKError,

    CLIConnectionError, CLINotFoundError

)

try:

    async for message in query(prompt="..."):

        pass

except CLINotFoundError as e:

    print(f"CLI 未找到： {e}")

except CLIConnectionError as e:

    print(f"连接失败： {e}")

except CodeBuddySDKError as e:

    print(f"SDK 错误： {e}")
```

## 最佳实践

1. **权限控制** ：在生产环境中使用 `canUseTool` 实现细粒度权限,避免使用 `bypassPermissions`
2. **资源限制** ：使用 `maxTurns` 限制执行范围，防止意外的资源消耗
3. **错误处理** ：始终处理 `result` 消息中的错误状态
4. **Hook 超时** ：为 Hook 设置合理的超时时间

## 相关文档

- [TypeScript SDK 参考](https://copilot.tencent.com/docs/cli/sdk-typescript) - TypeScript API 详细参考
- [Python SDK 参考](https://copilot.tencent.com/docs/cli/sdk-python) - Python API 详细参考
- [Hook 参考指南](https://copilot.tencent.com/docs/cli/hooks) - 详细的 Hook 配置说明
- [MCP 集成](https://copilot.tencent.com/docs/cli/mcp) - MCP 服务器配置指南
- [子 Agent 系统](https://copilot.tencent.com/docs/cli/sub-agents) - 子 Agent 详细说明

*CodeBuddy Agent SDK - 让 AI 编程能力融入你的应用*

更新于：2026-01-16 14:54

标签：

微信扫一扫赞赏

转载

收录

反馈

[yarkli](https://km.woa.com/user/yarkli)

太好了。 这个看起来和 Claude Agent SDK 有点像，迁移成本怎么样？

昨天 22:51 · 回复

几乎是兼容的

昨天 23:02 · 回复

[felixpfchen](https://km.woa.com/user/felixpfchen)

昨天 14:45 · 回复

[shuaihuayan](https://km.woa.com/user/shuaihuayan)

subo大佬的文章先点赞再看

昨天 14:46 · 回复

[kikipang](https://km.woa.com/user/kikipang)

subo出品，必属精品。

昨天 15:15 · 回复

[skipzhang](https://km.woa.com/user/skipzhang)

subo出品，必属精品。

昨天 15:16 · 回复

[aaronkjiang](https://km.woa.com/user/aaronkjiang)

很强，先赞后试。

今天 00:08 · 回复

[dyuankong](https://km.woa.com/user/dyuankong)

今天 01:27 · 回复

已到底部

23

39

8

![](https://km.woa.com/img/download_openkm.png)

扫一扫安装手机KM