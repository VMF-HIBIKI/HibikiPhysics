# Example code

**English** | [简体中文](AGENTS.zh-CN.md)

Runnable examples and code snippets in documentation use built-in types, standard library types and types provided by the public APIs they demonstrate. Example code can construct and use instances of those types. Custom type definitions used to test an extension or a failure path belong in `tests/`.

The `hibiki-example-types` prek hook checks staged examples. The **Example types** workflow checks the tracked C++ sources, C++ headers and Python sources under `examples/`. It flags C++ `struct` and `class` definitions and Python `class` definitions.
