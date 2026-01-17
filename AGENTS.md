# 仓库指南

## 核心原则

- 遇到问题必须按照第一性原理抽丝剥茧，定位本质后真正解决，而不是绕开。
- 不要我说一个做一个，我需要完整的功能，多发动主观能动性，而不是我说了一个你就只做这个，多思考。

## 语言与编码
- 沟通使用中文，但代码、文件名与标识符统一使用英文。
- 全项目采用 UTF-8 编码，禁止使用 GB2312 或其他不一致的编码格式。
## 交互与基本约束

- 在 Windows 环境执行命令时必须使用 `pwsh`（禁止 `bash`）；若发现 `powershell -NoLogo/-NoProfile/-Command <script>` 形式，重写为 `pwsh -c <script>` 并移除多余参数。


- 遇到多次复发的问题且已解决时，需要在本文件记录问题与解决方法，精炼描述即可。
- 文档归档在 `docs/`，评估价值后再记录，尽量复用现有文档文件而非频繁新建。


## 测试与类型检查

- 对复杂功能编写单元测试并放入 `test/` 目录，确保功能可验证。
- 每次开发完成后务必执行类型检查，保证无类型错误。


## 日志要求

- 项目完全由 AI 驱动，日志是定位问题的根基；实现功能时要输出足够信息，方便根据日志排查。

## 文件操作与搜索
- 读取文件、列目录优先使用函数工具（`read_file` / `list_dir`）；不得用 `cat`、`Get-Content`、`fd` 等命令替代。
- 查找普通文本使用 `grep_files`，禁止 `rg`/`grep`；查找符号（变量/函数等）优先 `sg (AST-grep)`。
- 需要拆分/提取文本时保持 ASCII，除非原文件已有其它字符集。



## 关于 Agents.md

不要把技术细节或者问题写入 agents.md 你如果要写就写抽线的内容，形而上的，而不是就事论事，写的是方法论。
## 项目结构与模块组织

- `renderdoc/`: 核心捕获/回放库、API 驱动程序（`driver/`）、钩子和共享工具。
- `qrenderdoc/`: Qt GUI 前端（`Code/`、`Resources/`、`Styles/`）。
- `renderdoccmd/`: 命令行工具和辅助程序。
- `renderdocshim/`: 用于全局钩取/注入的最小化 shim。
- `util/`: 工具集（包括 `clangformat/`）和 `util/test/` 中的 Python 测试框架。
- `docs/`: Sphinx 文档源文件。

## 构建、测试和开发命令

- Windows（主要平台）: 在 Visual Studio (2015+) 中打开 `renderdoc.sln`。日常调试使用 `Development`，性能测试使用 `Release`。
- CMake（Linux/macOS 或其他构建方式）:
  - 配置: `cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug`
  - 构建: `cmake --build build`
- 文档（可选）: `make -C docs html`（Windows 上使用 `docs\\make.bat html`）。

## 编码风格与命名约定

- 使用 **clang-format 15.0.7** 格式化（二进制文件位于 `util/clangformat/`）。CI 强制执行 `.clang-format`（2 空格缩进、100 列限制、不使用制表符）。
- 优先使用显式类型；仅在迭代器/lambda 中使用 `auto`。使用 `NULL`（而非 `nullptr`）。
- 最小化 STL 使用；优先使用项目容器（`rdcarray`、`rdcstr`）。字符串采用 UTF-8 编码（宽字符串仅用于 Windows 互操作）。
- 命名: 与周围代码保持一致；成员字段通常使用 `m_` 前缀。

## 测试指南

- 自动化覆盖率不完整；请在受影响的 API 和平台上测试更改。
- 构建测试使用的演示程序:
  - Windows: 打开 `util/test/demos.sln` 并构建。
  - 其他平台: `cmake -B build -S util/test/demos && cmake --build build`
- 运行 Python 测试:
  - 列出测试: `python util/test/run_tests.py --list`
  - 过滤测试: `python util/test/run_tests.py --test_include "D3D12_.*" --renderdoc <bin> --pyrenderdoc <pymodules>`

## 提交与 Pull Request 指南

- 提交主题: 使用祈使句、句首大写、不超过 72 个字符；常用子系统前缀（例如 `d3d12: ...`、`vulkan: ...`）。
- PR 基于 `v1.x` 分支；避免合并提交（使用 rebase），并将格式化/编译修复压缩到相关更改中。
- PR 应包括: 动机说明、测试备注（API/驱动/操作系统）、UI 更改的截图，以及关联的问题（适用时使用 `Closes #NNNN`）。
