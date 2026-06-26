# Development / 开发指南

## Recommended Environment / 推荐环境

- macOS or Linux / macOS 或 Linux
- Clang 18.1.2+ with `import std;` support / 支持 `import std;` 的 Clang 18.1.2+
- CMake 4.3+ / CMake 4.3+
- Ninja 1.11+ / Ninja 1.11+
- C++23 standard library module metadata / C++23 标准库模块 metadata

## CI Versions / CI 版本

GitHub Actions tool versions are centralized in `.github/workflows/ci.yml`:

GitHub Actions 的工具链版本集中写在 `.github/workflows/ci.yml`：

```yaml
env:
  LLVM_VERSION: "22"
  CMAKE_VERSION: "4.3.1"
  CMAKE_BUILD_TYPE: Release
```

Update these values when changing the CI compiler or CMake version. The Linux job installs matching LLVM/libc++ packages and passes `libc++.modules.json` to CMake. The macOS job uses Homebrew LLVM.

需要调整 CI 编译器或 CMake 版本时，改这里即可。Linux job 会安装匹配的 LLVM/libc++ 包，并把 `libc++.modules.json` 传给 CMake；macOS job 使用 Homebrew LLVM。

## Project Version / 项目版本

The CLI version printed by `md-archive --version` comes from `project(... VERSION ...)` in `CMakeLists.txt`. CMake injects it into `src/main.cpp` as `MD_ARCHIVE_VERSION`.

`md-archive --version` 打印的 CLI 版本来自 `CMakeLists.txt` 中的 `project(... VERSION ...)`。CMake 会把它作为 `MD_ARCHIVE_VERSION` 注入到 `src/main.cpp`。

## Configure and Build / 配置与构建

Debug:

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Release:

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The Makefile is a CMake wrapper:

```bash
make
make test
make run ARGS="--help"
```

On macOS, `make` prefers Homebrew LLVM at `/opt/homebrew/opt/llvm/bin/clang++` or `/usr/local/opt/llvm/bin/clang++` when `CXX` was not set explicitly. You can still override it with `make CXX=/path/to/clang++`.

在 macOS 上，如果没有显式设置 `CXX`，`make` 会优先使用 Homebrew LLVM 的 `clang++`。仍然可以用 `make CXX=/path/to/clang++` 覆盖。

## Running Tests / 运行测试

```bash
ctest --test-dir build --output-on-failure
```

Tests currently cover frontmatter parsing and a CLI archive smoke test. More tests should be added around title conflicts, remove, and rebuild.

当前测试覆盖 frontmatter 解析和 CLI 归档冒烟测试。后续应继续补充标题冲突、remove 和 rebuild 测试。

## API Documentation / API 文档

Public C++ APIs are documented in the `.cppm` module interface files with Doxygen comments. The comments are meant to serve both generated HTML docs and editor hover in clangd.

公共 C++ API 在 `.cppm` 模块接口文件中使用 Doxygen 注释。注释同时服务于生成的 HTML 文档和 clangd 的 hover 提示。

Generate API docs when Doxygen is installed:

安装 Doxygen 后可生成 API 文档：

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DMD_ARCHIVE_BUILD_API_DOCS=ON
cmake --build build --target api-docs
```

The output is written to `docs/api/html/` and is ignored by git.

输出会写入 `docs/api/html/`，该目录不会提交到 git。

## Local CLI / 本地运行 CLI

```bash
build/md-archive init
build/md-archive config show
build/md-archive add notes/example.md --force
```

## Install and Uninstall / 安装与卸载

Install writes the executable to `${CMAKE_INSTALL_PREFIX}/bin`, which defaults to `/usr/local/bin`:

安装会把可执行文件写入 `${CMAKE_INSTALL_PREFIX}/bin`，默认是 `/usr/local/bin`：

```bash
make install
```

Uninstall removes the files listed in `build/install_manifest.txt`:

卸载会删除 `build/install_manifest.txt` 中记录的文件：

```bash
make uninstall
```

## Configuration Discovery / 配置查找

`md-archive` resolves configuration in this order:

`md-archive` 按以下顺序解析配置：

1. `--config <path>`
2. `./config.ini`
3. parent directories up to filesystem root / 从当前目录向上查找
4. user config directory: `~/.config/md-archive/config.ini` or `%APPDATA%/md-archive/config.ini`
5. built-in defaults with current directory as workspace / 内置默认值，当前目录作为 workspace

`--workspace <path>` overrides the configured workspace for the current invocation.

`--workspace <path>` 会在本次调用中覆盖配置里的 workspace。

## C++ Modules / C++ Modules 说明

The project now builds the main code through real C++ named modules:

项目现在通过真实 C++ 命名模块构建主代码：

- `md_archive.config`
- `md_archive.frontmatter`
- `md_archive.tag_manager`
- `md_archive`

Each `.cppm` file contains the exported API, Doxygen comments, and implementation for one module. CMake uses `CXX_MODULES` file sets, `CXX_MODULE_STD`, and Ninja for BMI ordering.

每个 `.cppm` 文件包含一个模块的导出 API、Doxygen 注释和实现。CMake 使用 `CXX_MODULES` file set、`CXX_MODULE_STD` 和 Ninja 来调度 BMI 构建顺序。

On Homebrew LLVM, CMake may need `libc++.modules.json`. The top-level `CMakeLists.txt` tries to infer it from the selected `clang++`. If detection fails, pass it explicitly:

在 Homebrew LLVM 上，CMake 可能需要 `libc++.modules.json`。顶层 `CMakeLists.txt` 会尝试从所选 `clang++` 推断路径；如果失败，可显式传入：

```bash
cmake -S . -B build -G Ninja \
  -DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++ \
  -DCMAKE_CXX_STDLIB_MODULES_JSON=/opt/homebrew/opt/llvm/lib/c++/libc++.modules.json
```

TODO:

- Move `TagManager` archive behavior into `archive`.
- Move symlink and Markdown index generation into `index`.
- Add finer tests for `remove` and `rebuild`.

## Troubleshooting / 常见问题

If CMake cannot find `clang++`, pass an explicit compiler path:

如果 CMake 找不到 `clang++`，请显式指定路径：

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=/path/to/clang++
```

If clangd reports stale module errors, regenerate the build directory so `build/compile_commands.json` matches the current CMake graph:

如果 clangd 仍报告旧的 modules 错误，请重新生成构建目录，让 `build/compile_commands.json` 与当前 CMake 图一致：

```bash
cmake --fresh -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++
```

If `add` refuses a file, check that the file is under `workspace` and has closed frontmatter with both `tags` and `title`.

如果 `add` 拒绝文件，请确认文件位于 `workspace` 内，并且顶部 frontmatter 已闭合且包含 `tags` 与 `title`。
