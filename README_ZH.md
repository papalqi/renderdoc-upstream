<p align="center"><img src="https://user-images.githubusercontent.com/661798/36482670-f81601c0-170b-11e8-8adb-2365b346ac27.png" /></p>

[![MIT 许可证](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE.md)
[![CI](https://github.com/baldurk/renderdoc/workflows/CI/badge.svg?branch=v1.x&event=push)](https://github.com/baldurk/renderdoc/actions)
[![贡献者公约](https://img.shields.io/badge/Contributor%20Covenant-v2.0%20adopted-ff69b4.svg)](docs/CODE_OF_CONDUCT.md) 

RenderDoc 是一个基于帧捕获的图形调试器，目前支持在 Windows、Linux、Android 和 Nintendo Switch&trade; 平台上进行 Vulkan、D3D11、D3D12、OpenGL 和 OpenGL ES 开发。它完全开源，采用 MIT 许可证。

RenderDoc 仅用于调试您自己的程序。在任何官方公开的 RenderDoc 场所（包括问题跟踪器、Discord 或电子邮件）中，不允许讨论捕获您未创建的程序。例如，这包括捕获您未创建的商业游戏，或捕获 Google Maps 或 Google Earth。注意：捕获您使用第三方引擎（如 Unreal 或 Unity）创建的项目，或开源和免费项目是完全可以的，并且受到支持。

如果您有任何问题、建议或问题，可以在 GitHub 上[创建问题](https://github.com/baldurk/renderdoc/issues/new/choose)、[直接发送电子邮件](mailto:baldurk@baldurk.org)或加入 [IRC](https://webchat.oftc.net/?channels=renderdoc) 或 [Discord](https://discord.gg/ahq6yRB) 进行讨论。

要在 Windows 上安装，请运行适合您操作系统的安装程序（[64位](https://renderdoc.org/stable/latest/RenderDoc_latest_64.msi) | [32位](https://renderdoc.org/stable/latest/RenderDoc_latest_32.msi)）或从[构建页面](https://renderdoc.org/builds)下载便携式 zip 文件。64位 Windows 构建完全支持从32位程序捕获。在 Linux 上仅支持64位 x86 - 有预编译的[二进制 tarball](https://renderdoc.org/stable/latest/renderdoc_latest.tar.gz) 可用，或者您的发行版可能已经打包了它。如果没有，您可以[从源代码构建](docs/CONTRIBUTING/Compiling.md)。

* **下载**: 稳定版和每日构建: https://renderdoc.org/builds ( [符号服务器](https://renderdoc.org/symbols) )
* **文档**: [在线 HTML](https://renderdoc.org/docs)、[构建中的 CHM](https://renderdoc.org/docs/renderdoc.chm)、[视频](https://www.youtube.com/user/baldurkarlsson)
* **联系方式**: [baldurk@baldurk.org](mailto:baldurk@baldurk.org)、[OFTC IRC 上的 #renderdoc](https://webchat.oftc.net/?channels=renderdoc)、[Discord 服务器](https://discord.gg/ahq6yRB)
* **行为准则**: [贡献者公约](docs/CODE_OF_CONDUCT.md)
* **贡献者信息**: [所有贡献信息](docs/CONTRIBUTING.md)、[编译说明](docs/CONTRIBUTING/Compiling.md)
* **社区扩展**: [扩展仓库](https://github.com/baldurk/renderdoc-contrib)

截图
--------------

|| [ ![纹理视图](https://renderdoc.org/fp/ts_screen1.jpg?2) ](https://renderdoc.org/fp/screen1.jpg) | [ ![像素历史和着色器调试](https://renderdoc.org/fp/ts_screen2.jpg?2) ](https://renderdoc.org/fp/screen2.png) |
|| --- | --- |
|| [ ![网格查看器](https://renderdoc.org/fp/ts_screen3.jpg?2) ](https://renderdoc.org/fp/screen3.png) | [ ![管线查看器和常量](https://renderdoc.org/fp/ts_screen4.jpg?2) ](https://renderdoc.org/fp/screen4.png) |

API 支持
--------------

|                          | Windows                  | Linux                    | Android                   |
| ------------------------ | ------------------------ | ------------------------ | ------------------------  |
| Vulkan                   | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:        |
| OpenGL ES 2.0 - 3.2      | :heavy_check_mark:       | :heavy_check_mark:       | :heavy_check_mark:        |
| OpenGL 3.2 - 4.6 Core    | :heavy_check_mark:       | :heavy_check_mark:       |  不适用                    |
| D3D11 & D3D12            | :heavy_check_mark:       |  不适用                   |  不适用                    |
| OpenGL 1.0 - 2.0 Compat  | :heavy_multiplication_x: | :heavy_multiplication_x: |  不适用                    |
| D3D9 & 10                | :heavy_multiplication_x: |  不适用                   |  不适用                    |
| Metal                    |  不适用                   |  不适用                   |  不适用                    |

* Nintendo Switch&trade; 支持作为 NintendoSDK 的一部分单独分发给授权开发者。更多信息请咨询 Nintendo 开发者门户。

下载
--------------

有[二进制发布版本](https://renderdoc.org/builds)可用，从发布目标构建。如果您只是想使用该程序并且来到了这里，这就是您想要的 :)。

建议新用户从稳定版本开始。如果您需要，每日构建版本每天都可以从[这里的 v1.x 分支](https://renderdoc.org/builds#nightly)获得，但相应地可能不太稳定。

文档
--------------

文本文档可在[最新稳定版本的在线版本](https://renderdoc.org/docs/)以及任何构建中的 [renderdoc.chm](https://renderdoc.org/docs/renderdoc.chm) 中获得。它是从[使用 sphinx 的重构文本](docs)构建的。

如上所述，有一些[YouTube 视频](https://www.youtube.com/user/baldurkarlsson)展示了一些基本功能的使用和介绍/概述。

还有一个由 [@Icetigris](https://twitter.com/Icetigris) 制作的精彩演示，详细介绍了如何在实际情况下使用 RenderDoc：[幻灯片在这里](https://docs.google.com/presentation/d/1LQUMIld4SGoQVthnhT1scoA3k4Sg0as14G4NeSiSgFU/edit#slide=id.p)。

许可证
--------------

RenderDoc 在 MIT 许可证下发布，完整文本以及第三方库致谢请参见 [LICENSE.md](LICENSE.md)。

编译
---------

在大多数平台上构建 RenderDoc 都相当简单。更多详细信息请参见 [Compiling.md](docs/CONTRIBUTING/Compiling.md)。

贡献与开发
--------------

我在 [Developing-Change.md](docs/CONTRIBUTING/Developing-Change.md) 中添加了一些关于如何贡献以及从哪里开始查看代码的说明。所有贡献信息都可以在 [CONTRIBUTING.md](docs/CONTRIBUTING.md) 下找到。