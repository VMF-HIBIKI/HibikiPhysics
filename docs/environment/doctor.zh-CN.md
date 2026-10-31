# Windows 环境诊断

[English](doctor.md) | **简体中文**

`scripts/dev.ps1 doctor` 检查选定的 Windows 开发环境，读取仓库声明、PE 版本资源与架构、文件哈希、Visual Studio 安装记录、SDK 注册信息、Python 发行元数据和 VS Code 扩展元数据。

## 使用方式

在仓库根目录运行：

```powershell
pwsh -NoProfile -File .\scripts\dev.ps1 doctor
pwsh -NoProfile -File .\scripts\dev.ps1 doctor -RequireWarp
pwsh -NoProfile -File .\scripts\dev.ps1 doctor -ReportPath artifacts/environment/doctor.json
```

默认命令将报告输出到终端。`-ReportPath` 显式将 UTF-8 JSON 文件写入 `artifacts/environment/`。输出路径采用普通目录，检测到已有重解析点时报告失败。退出码 `0` 表示必需检查匹配，`1` 表示必需诊断项失败，`2` 表示调用、输入读取或报告输出错误。

检查通过文件与注册表读取完成。工具、Python、扩展服务和安装程序保持未启动状态。网络获取、环境准备及编辑器集成属于后续实施阶段。

## 路径选择

命令行 `-Paths` 条目覆盖选定的本地配置。`-ConfigPath` 选择 JSON 文件，默认位置为 `.local/environment.json`。默认文件缺失时采用自动发现，显式指定的文件接受必需有效性检查。显式组件路径的目标缺失时保持原选择并报告结果。

路径按字面值处理，相对路径基于仓库根目录解析。PowerShell 会话可以直接传入覆盖：

```powershell
& .\scripts\dev.ps1 doctor -Paths @{
    'clang-cl' = 'C:\Development Tools\LLVM\bin\clang-cl.exe'
    visualStudio = 'C:\Program Files\Microsoft Visual Studio\18\Community'
}
```

本地配置格式如下：

```json
{
  "schema_version": 1,
  "paths": {
    "toolsRoot": "C:/Development Tools/HibikiPhysics/tools",
    "clang-cl": "C:/Development Tools/LLVM/bin/clang-cl.exe",
    "extensions": "C:/Development Tools/VSCode/extensions"
  }
}
```

该示例说明输入格式，本地配置生成由 ENV-004 负责。

| 路径键 | 含义 |
| --- | --- |
| `toolsRoot` | 按版本组织的工具根目录，默认 `%LOCALAPPDATA%/HibikiPhysics/tools` |
| `clang-cl`、`clangd`、`clang-format`、`clang-tidy`、`lld-link`、`cmake`、`ninja`、`uv`、`python`、`vscode` | 可执行文件完整路径 |
| `visualStudio` | Visual Studio 安装根目录 |
| `msvc` | 选定的 `VC/Tools/MSVC/<version>` 目录 |
| `sdk` | 包含 `Include`、`Lib` 和 `bin` 的 Windows Kits 根目录 |
| `extensions` | 包含已安装扩展文件夹的目录 |
| 选定扩展的 ID | 包含其 `package.json` 的具体安装文件夹 |

工具自动发现依次检查 `<toolsRoot>/<artifact>/<version>/bin` 和对应版本目录。LLVM 与 VS Code 同时检查常见安装位置，随后查找 PATH 中的可执行文件。Python 优先选择 `.venv/Scripts/python.exe`，项目解释器缺失时以 PATH 结果作为诊断候选。Python 包检查始终针对仓库的 `.venv`。

Visual Studio 发现优先匹配已记录的安装修订号，随后选择包含目标工具集目录的安装，再考虑其他本地实例记录。编译器、CRT、SDK 包修订及选定文件哈希分别检查。SDK 注册信息描述宿主安装，选定 SDK 的内容按解析路径检查。扩展发现优先选择固定版本，其他已安装版本作为偏差证据。报告记录每个选定路径。

## 报告含义

| 状态 | 含义 |
| --- | --- |
| `matched` | 当前检查与预期证据匹配 |
| `missing` | 所需文件、包或安装记录缺失 |
| `mismatch` | 实际版本、架构、哈希或声明存在偏差 |
| `pending` | 所需身份核验证据待补齐 |

每项检查包含 `required`、`path`、`expected`、`actual`、`detail` 和 `next_step`。必需项处于 `matched` 以外的状态时，`ready` 保持 false。Warp 包安装默认为可选检查，`-RequireWarp` 将其设为必需项。直接依赖声明始终属于仓库必需检查。Python 声明检查针对 `dev` 和 `warp` 组当前采用的精确版本格式。

报告结构由 [doctor-report.schema.json](../../tools/doctor-report.schema.json) 定义。

可执行文件检查记录实测 SHA-256，并与已验证归档文件记录比较。版本资源缺失时，通过固定归档的文件身份和 PE 架构补齐证据。已安装 VSIX 文件与其归档记录核对，侧载扩展的平台可以通过选定归档的核验结果确定。Python 锁文件身份和安装文件哈希与准备阶段的环境记录核对，仍缺少证据的项目保留 `pending` 状态。[环境准备](prepare.zh-CN.md)负责建立这些记录。

编译器执行、调试连接、LSP 工作、依赖导入和 GPU 检查属于验收项目及 ENV-006。报告描述检查时的环境身份。

## 验证

```powershell
pwsh -NoProfile -File .\tests\environment\doctor.tests.ps1
```

测试在 `artifacts/environment-tests/` 中创建独立样例，覆盖工具缺失、版本错误、架构读取、显式覆盖、含空格路径、声明冲突、系统文件变化、扩展元数据、配套清单完整性、退出码和显式 JSON 输出。文件哈希、修改时间和进程环境快照用于验证默认只读行为。测试样例保留供检查。

2026-10-31 的执行验证使用现有 PowerShell 7.6.5，28 项测试通过。Windows PowerShell 5.1 的语法检查通过；其脚本执行受到本机执行策略阻止，该宿主的运行验证保持待执行。执行策略保持现状。

本机诊断发现选定 MSVC／SDK 文件哈希匹配，PATH 中的 Python 候选版本与 CPython 3.12.16 存在偏差，部分工具和项目包等待准备。这些结果描述当前机器状态，后续工作由[实施计划](../plans/0001-windows-environment.zh-CN.md)跟踪。
