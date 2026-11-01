# Windows 工具链身份记录

[English](windows-identities.md) | **简体中文**

记录日期：2026-10-31。范围：ENV-001。

状态：ENV-001 已完成。22 个选定归档通过 SHA-256 核验，包括六个工具／编辑器发行包、六个 VSIX 包、六个直接 Python wheel 和四个 C++ 源码归档。记录包含 18 个现有 Windows 文件哈希和 21 份原始许可证文本。Schema 校验与跨文件一致性检查通过。错误输入检查能够检出缺失哈希、待核验记录标记为完成、重复 ID、发布方哈希冲突、Python 版本偏差和许可证内容变化。构建与运行验证按实施计划推进。

## 记录位置

[toolchain-lock.json](../../tools/toolchain-lock.json) 记录选定的工具归档、Python wheel、VSIX 包和 C++ 源码归档。各条目包含版本、平台、来源 URL、SHA-256 和核验状态，C++ 条目同时记录解析后的源码提交。[Dependencies.cmake](../../cmake/Dependencies.cmake) 读取这些源码身份并声明依赖选项，调用方提供已校验的本地归档目录。

[Windows 组件记录](../../tools/provenance/windows-system-components.json) 保存 Visual Studio 官方目录中的安装包身份和载荷哈希，以及选定编译器文件、STL 头文件、CRT 库和 SDK 文件的哈希。清单同时保留当前安装中的辅助 MSVC 目标和资源包。ENV-004 将据此确定并验证 Windows x64 所需的安装组件集合。

| 组件 | 记录身份 |
| --- | --- |
| MSVC 工具集目录 | 14.51.36231 |
| 编译器文件版本／产品版本 | 19.51.36248.0／14.51.36248.0 |
| CRT 头文件包 | 14.51.36244 |
| x64 桌面 CRT 包 | 14.51.36247 |
| SDK 头文件／库选择版本 | 10.0.26100.0 |
| SDK 安装 DisplayVersion | 10.1.26100.8249 |
| SDK Visual Studio 目录包 | Win11SDK_10.0.26100，10.0.26100.16 |

这些字段分别标识服务更新后安装内容的不同层级。机器专属绝对路径保存在本地。组件记录采用相对于选定 MSVC 和 SDK 根目录的路径。

## Python 与编辑器包

[.python-version](../../.python-version) 选择 CPython 3.12.16。[pyproject.toml](../../pyproject.toml) 在 `dev` 和 `warp` 依赖组中声明直接依赖的精确版本。解释器归档对应 [uv 0.13.6 下载元数据](https://github.com/astral-sh/uv/blob/0.13.6/crates/uv-python-managed/download-metadata.json) 中的常规 GIL Windows x64 条目：构建号 20261026，`install_only_stripped`。

首份 `uv.lock` 与传递依赖审查安排在 ENV-004。当前 wheel 记录描述直接依赖的发行文件。basedpyright 的 Node.js 依赖及 Warp 的传递依赖通过该锁文件确定解析身份。

VSIX 身份包含扩展版本和目标平台。Ruff 客户端选定 `charliermarsh.ruff` 2026.92.0 win32-x64，basedpyright 客户端选定 `detachhead.basedpyright` 1.40.2 universal。ENV-005 负责将这些客户端接入选定的项目服务端。

## 核验状态

| 状态 | 证据 |
| --- | --- |
| `publisher_metadata_only` | 官方元数据提供预期发行身份，本地归档核验待执行 |
| `publisher_hash_matched` | 下载内容与发布方 SHA-256 一致，同时核对已提供的发行大小 |
| `downloaded_and_hashed` | 下载内容已计算本地 SHA-256，记录保留来源 URL |
| `installed_files_hashed` | 现有安装中的选定文件已记录 SHA-256 |

[Schema](../../tools/toolchain-lock.schema.json) 允许 `identity_recording` 状态保留待补齐记录。`identity_complete` 状态要求归档已核验，且哈希和大小完整。发布方哈希相等、条目 ID 唯一、配套文件完整性和跨文件版本一致性通过额外检查验证。编译器执行、包安装、调试集成和干净重建属于 ENV-002 至 ENV-006 的验收工作。

本地证据缓存位于 `artifacts/env001/`，保存下载归档及上游 API 响应。版本化清单保留稳定的发布方 URL，临时签名重定向 URL 保存在本地下载报告中。后续准备过程按提交的哈希校验缓存内容。

`scripts/environment/check_identities.py` 检查跨文件版本、发布方哈希、必需条目 ID 和许可证记录。可选参数 `--archives artifacts/env001/downloads` 同时校验缓存归档。首次记录检查使用现有宿主 Python 和 PowerShell，项目工具执行安排在环境准备之后。JSON Schema 校验采用 Draft 2020-12 验证器，本次使用 PowerShell 7 的 `Test-Json`。

## 许可证记录

[第三方声明](../../third_party/licenses/README.zh-CN.md) 保存选定 C++ 归档及直接 Python wheel 中的原始许可证文件，并单独记录归档内路径和 SHA-256。上游许可证原文保留原始语言。环境准备过程保留工具发行包和扩展随附的许可证，分发打包阶段单独审查随附声明。
