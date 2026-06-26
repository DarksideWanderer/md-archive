# Contributing / 贡献指南

## Reporting Bugs / 报告 Bug

Please use the bug report template and include OS, compiler, CMake version, the exact command, and concise logs.

请使用 Bug 模板，并提供操作系统、编译器、CMake 版本、完整命令和精简日志。

## Requesting Features / 请求功能

Explain the workflow problem first, then propose CLI syntax or behavior. Keep the zero runtime dependency goal in mind.

请先说明工作流问题，再提出 CLI 语法或行为建议。请保持项目“零外部运行时依赖”的目标。

## Pull Requests / 提交 PR

Keep PRs focused. Include tests for behavior changes and update README or docs when CLI, config, or archive semantics change.

PR 应保持聚焦。涉及行为变化时请补测试；涉及 CLI、配置或归档语义时请更新 README 或 docs。

## Code Style / 代码风格

Use modern C++23 with standard library facilities. Format with `.clang-format`; prefer small functions with clear filesystem error handling.

使用现代 C++23 和标准库。请用 `.clang-format` 格式化；优先编写小函数，并提供清晰的文件系统错误处理。

Public module interfaces should use Doxygen comments for exported types and functions. Prefer concise `@brief`, `@param`, and `@return` notes, and use modern attributes such as `[[nodiscard]]` when ignoring a return value would hide a likely bug.

公共模块接口应为导出的类型和函数使用 Doxygen 注释。优先使用简洁的 `@brief`、`@param` 和 `@return`；当忽略返回值很可能隐藏错误时，使用 `[[nodiscard]]` 等现代属性。

## Testing / 测试要求

Run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

请在提交前运行上述命令。

## Documentation / 文档要求

Documentation should be bilingual in the same file. English and Chinese do not need to be line-by-line translations, but both audiences should understand the behavior.

文档应在同一文件中提供中英文说明。不要求逐句翻译，但应让中英文读者都能理解行为。

API comments may be shorter than user documentation, but exported APIs should still be understandable from generated Doxygen pages and clangd hover.

API 注释可以比用户文档更短，但导出 API 应能通过生成的 Doxygen 页面和 clangd hover 理解。

## Generated Files / 生成文件

Do not commit `.archive/`, `.tags/`, local `config.ini`, `build/`, or compiled binaries.

不要提交 `.archive/`、`.tags/`、本地 `config.ini`、`build/` 或编译产物。
