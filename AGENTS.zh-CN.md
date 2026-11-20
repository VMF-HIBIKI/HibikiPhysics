# 示例代码

[English](AGENTS.md) | **简体中文**

可运行示例和文档中的示例代码都使用语言内置类型、标准库类型以及所演示的公开 API 提供的类型，可以直接创建和使用这些类型的实例。用于验证扩展机制或失败路径的自定义类型定义放在 `tests/`。

`hibiki-example-types` 提交钩子检查暂存的示例。**Example types** 工作流检查 `examples/` 中纳入版本管理的 C++ 源文件、C++ 头文件和 Python 源文件，并报告示例里定义的 C++ `struct`、`class` 和 Python `class`。
