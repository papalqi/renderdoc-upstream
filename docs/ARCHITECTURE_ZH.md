# RenderDoc 架构与数据流（中文）

> 本文基于当前仓库代码（`renderdoc/`、`qrenderdoc/`、`renderdoccmd/` 等）做“架构+数据流”总结，目标是用尽量少的关键概念把 RenderDoc 的工作机理串起来。  
> 如果你准备深入某个 API（D3D12/Vulkan/GL…），建议先把本文的“控制面/数据面 + 抓取/回放 + 远程”心智模型建立起来，再沿着文末的“读代码路线”下钻。

---

## 1. 全景：模块分层与边界

### 1.1 仓库模块一览（职责边界）

- `renderdoc/`：核心库（同一套代码覆盖**抓取（capture）**与**回放/分析（replay/analysis）**），对外导出 C API，并包含各图形 API 的驱动实现。
  - 中枢：`RenderDoc` 单例（`renderdoc/core/core.h`，`class RenderDoc`）。
  - Hook 框架：`renderdoc/hooks/*`（注册要 hook 的库/函数 + 平台落地）。
  - 各图形 API 驱动：`renderdoc/driver/*`（D3D11、D3D12、Vulkan、GL…）。
  - 回放/分析核心：`renderdoc/replay/*`（驱动接口、ReplayController、打开 `.rdc`、回放入口等）。
  - 远程：`renderdoc/core/remote_server.*`、`renderdoc/core/replay_proxy.*`。
- `qrenderdoc/`：Qt GUI 前端。
  - 程序入口：`qrenderdoc/Code/qrenderdoc.cpp`。
  - GUI 上下文：`qrenderdoc/Code/CaptureContext.*`（更偏“业务/状态/窗口协调”）。
  - 回放线程与异步队列：`qrenderdoc/Code/ReplayManager.*`（更偏“执行/串行化/跨线程调用”）。
- `renderdoccmd/`：命令行工具（含 `remoteserver` 子命令）。
- `renderdocshim/`：Windows 全局注入的极简 shim DLL（把 renderdoc 注入到符合条件的进程；尽量不依赖 CRT，降低对目标进程的影响）。

### 1.2 总体层次图（“控制面 / 数据面”）

```text
                         ┌────────────────────────────────────┐
                         │            qrenderdoc (GUI)         │
                         │  CaptureContext + ReplayManager     │
                         └───────────────┬────────────────────┘
                                         │  ① 控制面：启动/注入/连接/打开
                                         v
┌──────────────────────────────────────────────────────────────────────────────┐
│                              renderdoc 核心库                                │
│                                                                              │
│  ┌──────────────┐   ┌───────────────┐    ┌────────────────────────────────┐  │
│  │  OS 抽象层     │   │ Hook 框架       │    │ 驱动层 driver/*               │  │
│  │  os/*          │<->│ hooks/*        │<-->| D3D11/D3D12/Vulkan/GL...      │  │
│  └──────────────┘   └───────────────┘    └────────────────────────────────┘  │
│           ^                     |                           |                 │
│           | ② 控制面：注入/进程  | ③ 数据面：拦截 API 调用    | ④ 数据面：序列化/回放 │
│           |                     v                           v                 │
│    Process::Launch...     Wrapper/Hooks                 Serialise/Replay       │
└──────────────────────────────────────────────────────────────────────────────┘
                                         │
                                         v
                           ┌────────────────────────┐
                           │   capture 文件（.rdc）   │
                           └────────────────────────┘
```

**你可以把 RenderDoc 理解为两条主链路：**
- **控制面（Control Plane）**：启动/注入/连接/远程调度/打开 capture。
- **数据面（Data Plane）**：hook 拦截 → wrapper 记录 → 序列化落盘 `.rdc` → 回放执行 + 提供分析查询。

---

## 2. 抓取（Capture）机理与数据流

抓取发生在“目标进程内”（renderdoc 被注入到目标进程后）。

### 2.1 控制面：启动与注入（本地）

RenderDoc 对外提供一组回放侧的导出 API（实现集中在 `renderdoc/replay/entry_points.cpp`），其中：
- 启动并注入：`RENDERDOC_ExecuteAndInject`（`renderdoc/replay/entry_points.cpp`）
- 注入已存在进程：`RENDERDOC_InjectIntoProcess`

它们会调用 OS 抽象层的进程注入逻辑：
- `Process::LaunchAndInjectIntoProcess` / `Process::InjectIntoProcess`（接口：`renderdoc/os/os_specific.h`；平台实现如 `renderdoc/os/win32/win32_process.cpp`、`renderdoc/os/posix/posix_process.cpp`）

**控制面流程图（本地）**

```text
qrenderdoc / renderdoccmd
   |
   |  RENDERDOC_ExecuteAndInject(app, ...)
   v
renderdoc/replay/entry_points.cpp
   |
   |  Process::LaunchAndInjectIntoProcess(...)
   v
renderdoc/os/*_process.cpp
   |
   |  创建进程 + 注入 renderdoc (或配置环境变量/注入方式)
   v
目标进程加载 renderdoc.dll / librenderdoc.so
```

> Windows 全局 hook 另一条常见路径是 `renderdocshim`：shim 被插入到更多进程里，它通过共享内存读取“匹配规则/rdoc 路径/选项”，匹配成功则 `LoadLibraryW` 载入 renderdoc（见 `renderdocshim/renderdocshim.cpp`）。

### 2.2 数据面：Hook → Wrapper → Serialise（命令流）

RenderDoc 不是“录屏”，而是“记录图形 API 调用序列 + 必要资源内容/状态”，形成可回放的命令流。核心机制在官方文档里有概述：`docs/behind_scenes/how_works.rst`。

**数据面关键点**
- 驱动层会 hook 关键入口（例如创建 device/instance），建立“应用对象”到“wrapper 对象”的替换关系。
- 之后应用所有 API 调用都会走 wrapper：
  - 一边调用真实 API（保证目标程序正常渲染）
  - 一边把调用参数/副作用按驱动规则序列化成 chunk 流（内存中）
- 当触发抓帧后，将该帧相关的 chunk 流和必要资源内容落盘为 `.rdc`。

**数据面流程图（目标进程内）**

```text
    应用线程 (Game/App)
           |
           |  API Call: vkCmdDraw / ID3D12CommandQueue::ExecuteCommandLists / glDraw* ...
           v
   Hooked Entrypoints + Wrapped Objects
           |
     ┌─────┴────────────────────────────────────────────────────┐
     |                                                          |
     | (A) 真实调用：转发到真实驱动/运行时，保证程序继续跑         |
     | (B) 记录调用：序列化参数、资源引用、状态变化到 chunk 流     |
     └─────┬────────────────────────────────────────────────────┘
           v
     in-memory chunk stream
           |
           |  EndFrameCapture -> 写入文件
           v
        capture.rdc
```

### 2.3 CaptureState：抓取/回放共享的状态机

核心库用同一套状态机描述“当前处于抓取还是回放”（见 `renderdoc/core/core.h` 的 `enum class CaptureState`），核心思想是：
- 注入到目标进程后，常态是 **BackgroundCapturing**（后台轻量记录/准备），触发抓帧后进入 **ActiveCapturing**（该帧全量序列化），落盘后返回后台。
- 打开 `.rdc` 后，先 **LoadingReplaying**（读取初始化段、构建 action 树/资源映射等），然后 **ActiveReplaying**（响应 UI 查询/局部回放）。
- 另有 **StructuredExport**：不初始化图形 API，仅用驱动序列化逻辑把 capture 解码成结构化数据用于导出。

**状态机图**

```text
Capture（目标进程内）
┌──────────────────────┐   触发抓帧    ┌──────────────────────┐
│ BackgroundCapturing   │─────────────>│ ActiveCapturing       │
│ (后台准备/轻量记录)    │              │ (该帧全量序列化)       │
└──────────┬───────────┘   帧结束落盘   └──────────┬───────────┘
           └────────────────────────────────────────┘

Replay（打开 .rdc）
┌──────────────────────┐   初始化加载   ┌──────────────────────┐
│ LoadingReplaying      │─────────────>│ ActiveReplaying       │
│ (建 action 树/资源表)  │              │ (响应 UI 查询/局部回放)│
└──────────────────────┘              └──────────────────────┘

Structured Export（不创建真实图形设备）
┌──────────────────────┐
│ StructuredExport      │
└──────────────────────┘
```

### 2.4 `.rdc` 的“chunk 命令流”视角（概念结构）

你不需要立刻理解 `.rdc` 的具体二进制布局，只要先把它当成“可回放的命令流容器”：

```text
capture.rdc（概念）
┌─────────────────────────────┐
│ Header / 元信息             │  (版本/驱动标识/缩略图/平台信息等)
├─────────────────────────────┤
│ 初始化段 chunks              │  (frame 开始前的资源创建/初始状态等)
├─────────────────────────────┤
│ 帧内 chunks（按顺序）        │  (每个 API 调用的序列化记录)
├─────────────────────────────┤
│ 资源内容 / 初始内容（按需）   │  (只保存被该帧引用的资源为主)
└─────────────────────────────┘
```

RenderDoc 回放时“从头执行初始化段”，再在帧内按需执行（全帧或局部），因此能够：
- 构建 Event Browser / API Inspector 那样的 action 树
- 在任意 eventId 位置做局部回放，恢复到“当时的管线状态+资源内容”，再叠加分析动作（overlay/pixel history/shader debug 等）

---

## 3. 回放（Replay/Analysis）机理与数据流（本地）

### 3.1 打开 capture：`ICaptureFile` → `IReplayController`

主链路是：
- 创建/打开 capture：`RENDERDOC_OpenCaptureFile`（声明在 `renderdoc/api/replay/renderdoc_replay.h`）
  - 实现类在 `renderdoc/replay/capture_file.cpp`（`class CaptureFile : public ICaptureFile`）
- 打开后 `OpenCapture()` 返回 `IReplayController*`
  - 实现是 `ReplayController`（`renderdoc/replay/replay_controller.h`）
- `ReplayController` 内部选择对应图形 API 的驱动实现 `IReplayDriver`
  - 接口在 `renderdoc/replay/replay_driver.h`（`class IRemoteDriver` / `class IReplayDriver`）

**回放数据流（本地）**

```text
qrenderdoc (ReplayManager 线程)
   |
   |  ICaptureFile::OpenFile(...) / OpenCapture(...)
   v
renderdoc/replay/capture_file.cpp (CaptureFile)
   |
   |  -> IReplayController*  (ReplayController)
   v
renderdoc/replay/replay_controller.h (ReplayController)
   |
   |  选择 IReplayDriver（按 capture 记录的 API 类型）
   |  ReadLogInitialisation(...)  (建 action 树/资源映射/统计依赖)
   |  ReplayLog(endEvent, type)   (按需局部回放)
   v
各种分析查询（pipeline state / resource / pixel history / shader debug / overlays...）
```

### 3.2 为什么 UI 能“跳到任意事件”：局部回放（Partial Replay）

RenderDoc 的常用分析都建立在“局部回放”上（官方示例见 `docs/behind_scenes/how_works.rst`），典型模式是：

```text
选择 eventId = N
  1) Replay up to (N-1)：把状态恢复到 action 前
  2)（可选）只 replay action N：分析该 draw/dispatch 的影响
  3)（可选）叠加 overlay：深度测试/过度绘制/输出高亮等
```

你可以把它理解为：RenderDoc 把“回放”当成一种通用的“查询手段”，UI 的每个工具都在回放基础上组合实现。

---

## 4. 远程 Capture/Replay（网络）

RenderDoc 把远程抽象成 **Replay Context**（见 `docs/how/how_network_capture_replay.rst`）：UI 在本地，但“运行/注入/回放”可在远端完成。

远程常用两类连接：
- **RemoteServer**：更像“远端代理服务”，负责远端文件浏览、执行并注入、拷贝 capture、打开 capture 等（实现：`renderdoc/core/remote_server.h` 的 `struct RemoteServer : public IRemoteServer`）。
- **TargetControl**：连接到“已注入的目标进程”，用于触发抓帧、查询目标状态等（API：`renderdoc/api/replay/renderdoc_replay.h` 的 `RENDERDOC_CreateTargetControl`）。

另外，为了让 UI “像本地一样调用回放接口”，RenderDoc 使用 **ReplayProxy** 在网络两端代理 `IReplayDriver` 的方法调用（`renderdoc/core/replay_proxy.h`）。

**远程数据流图（Host=UI，Target=远端机器）**

```text
Host: qrenderdoc
  |
  | (1) CreateRemoteServerConnection(URL)
  v
Target: renderdoccmd remoteserver  (RemoteServer)
  |
  | (2) ExecuteAndInject(app, ...) -> 返回 ident
  v
Target: 目标程序 + 注入 renderdoc
  |
  | (3) CreateTargetControl(URL, ident)  (触发抓帧/控制目标)
  | (4) 生成 capture（默认先在远端临时保存）
  v
Target: capture.rdc
  |
  | (5a) CopyCaptureFromRemote(远端->本地)  或
  | (5b) OpenCapture(在远端回放) -> ReplayProxy 代理回传结果
  v
Host: UI 展示/分析（像本地一样操作）
```

> 直觉上：RemoteServer 更像“远程文件/进程/回放服务”，TargetControl 更像“目标进程的控制通道”，ReplayProxy 更像“把回放 API RPC 化”。

---

## 5. qrenderdoc 的线程/调用模型（理解 UI 为什么要 ReplayManager）

GUI 常见设计是：**UI 线程不直接做重回放**，而把回放相关调用串行化到一个工作线程，避免卡 UI，也避免驱动/回放接口被多线程乱序调用。

在 qrenderdoc 中：
- `CaptureContext`：偏“状态/业务/窗口协同/对外接口”（`qrenderdoc/Code/CaptureContext.h`）。
- `ReplayManager`：偏“回放线程 + 请求队列 + 本地/远程统一封装”（`qrenderdoc/Code/ReplayManager.h`）。

**线程模型图**

```text
UI线程 (Qt)
  |
  | 用户操作：切 event / 打开纹理 / shader debug / pixel history ...
  v
CaptureContext
  |
  | 组织一次回放请求（lambda/callback）
  v
ReplayManager（回放线程，串行执行）
  |
  | IReplayController / IReplayDriver 调用（可能触发 ReplayLog）
  v
结果回到 UI（更新视图/刷新窗口）
```

---

## 6. 读代码路线（推荐从“边界入口”往里走）

### 6.1 先建立总控视角（抓取/回放共用）
- `renderdoc/core/core.h`：`RenderDoc` 单例、`CaptureState`、驱动 provider 注册点等。
- `renderdoc/hooks/hooks.h`：hook 工作流总览（写得很详细）。
- `renderdoc/os/os_specific.h`：进程注入、线程、网络等 OS 抽象接口。

### 6.2 抓取入口（控制面）
- `renderdoc/replay/entry_points.cpp`：`RENDERDOC_ExecuteAndInject` / `RENDERDOC_InjectIntoProcess` / 远程枚举等导出函数。
- Windows 全局 shim：`renderdocshim/renderdocshim.cpp`

### 6.3 回放入口（数据面）
- `renderdoc/replay/capture_file.cpp`：打开 `.rdc`、`CaptureFile::OpenCapture`。
- `renderdoc/replay/replay_controller.h`：`ReplayController`（大量分析接口的上层汇总）。
- `renderdoc/replay/replay_driver.h`：`IReplayDriver/IRemoteDriver`（驱动需要实现的“能力清单”）。

### 6.4 选一个 API 驱动下钻（从 hooks 文件开始）
- D3D11：`renderdoc/driver/d3d11/d3d11_hooks.cpp`
- D3D12：`renderdoc/driver/d3d12/d3d12_hooks.cpp`
- Vulkan：`renderdoc/driver/vulkan/vk_layer.cpp` / `renderdoc/driver/vulkan/*`

---

## 7. 术语小抄（对照 UI/代码）

- **eventId / action**：帧内按顺序记录的事件/动作；UI 的 Event Browser 即基于 action 树。
- **background capture**：注入后常态，低开销记录必要信息、维护资源跟踪。
- **active capture**：触发抓帧后，该帧调用序列被完整序列化，帧结束写 `.rdc`。
- **partial replay**：为分析/查询而进行的局部回放（回放到某 eventId 前/只回放某 action）。
- **Replay Context**：远程工作模式下，“所有路径、执行、回放都相对某台机器”的上下文抽象。

