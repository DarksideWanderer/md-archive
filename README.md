# md-archive

`md-archive` 是一个轻量级 C++ CLI 工具，用 YAML frontmatter 中的 `tags` 和 `title` 为 Markdown 文档建立持久归档和标签导航。

`md-archive` is a lightweight C++ CLI that builds a durable Markdown archive and tag navigation system from YAML frontmatter `tags` and `title`.

## Features / 功能特性

- Copies source Markdown files into `.archive/` so archived content survives source deletion.
- Creates `.tags/<tag>/` symlinks that point to `.archive/` copies.
- Generates `.tags/<tag>.md` index files that link back to source files for editing in VS Code.
- Supports `add`, `scan`, `list`, `docs`, `remove`, `rebuild`, `init`, and `config` commands.
- Uses C++23 named modules, `import std;`, CMake, Ninja, and zero external runtime dependencies.

- 将源 Markdown 复制到 `.archive/`，源文件删除后归档副本仍保留。
- 在 `.tags/<tag>/` 下创建指向 `.archive/` 副本的符号链接。
- 生成 `.tags/<tag>.md` 标签索引，索引链接指向源文件，方便在 VS Code 中编辑。
- 支持 `add`、`scan`、`list`、`docs`、`remove`、`rebuild`、`init` 和 `config`。
- 使用 C++23 命名模块、`import std;`、CMake、Ninja，无外部运行时依赖。

## How It Works / 工作原理

```text
你的 .md 文件（源文件）              .archive/ 目录（内容副本，安全存储）
─────────────────────────────       ─────────────────────────────────────
dijkstra.md                         .archive/dijkstra.md    ← 完整内容副本
  ---                              （源文件删除后也不丢失）
  tags: [算法, 图论]
  title: Dijkstra 最短路径           .tags/ 目录（自动生成）
  ---                               ───────────────────────
segment-tree.md                     .tags/算法/
  ---                                └── Dijkstra 最短路径.md -> ../../.archive/dijkstra.md
  tags: [数据结构, 算法]             .tags/图论/
  title: 线段树完全指南                └── Dijkstra 最短路径.md -> ../../.archive/dijkstra.md
  ---                               .tags/算法.md    ← 索引文件（链接指向源文件方便编辑）
                                    .tags/图论.md    ← 同上
```

Each archive operation:

每次归档时：

1. Parse top-of-file YAML frontmatter with `tags` and `title`.
2. Copy the full Markdown file into `.archive/`, mirroring the workspace-relative path.
3. Create symlinks under `.tags/<tag>/` pointing to the `.archive/` copy.
4. Update `.tags/<tag>.md` indexes with links to source files.

## Installation / 安装

Build from source:

从源码构建：

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The Makefile is a CMake wrapper:

Makefile 现在是 CMake 包装器：

```bash
make
make test
```

Optional install:

可选安装：

```bash
cmake --install build
```

Uninstall removes files recorded by CMake's `install_manifest.txt`:

卸载会删除 CMake `install_manifest.txt` 记录的已安装文件：

```bash
make uninstall
```

## Configuration / 配置

Create a local config:

创建本地配置：

```bash
md-archive init
md-archive config show
```

Example:

示例：

```ini
[archive]
workspace = /Users/you/your-notebook
tags_dir = .tags
archive_dir = .archive
```

Configuration lookup order:

配置查找顺序：

1. `--config <path>`
2. `./config.ini`
3. parent directories up to filesystem root / 从当前目录逐级向上查找
4. `~/.config/md-archive/config.ini` or `%APPDATA%/md-archive/config.ini`
5. built-in defaults: current directory as workspace, `.tags`, `.archive` / 内置默认值

`--workspace <path>` temporarily overrides the configured workspace. `tags_dir` and `archive_dir` must be relative paths inside workspace; absolute paths and `..` are rejected.

`--workspace <path>` 会临时覆盖配置中的 workspace。`tags_dir` 和 `archive_dir` 必须是 workspace 内部相对路径；绝对路径和 `..` 会被拒绝。

## CLI Usage / 命令行使用

```bash
md-archive --help
md-archive --version

md-archive init
md-archive config show
md-archive config path

md-archive add path/to/note.md
md-archive add path/to/note.md --force
md-archive scan
md-archive scan --force
md-archive list
md-archive list 算法
md-archive docs
md-archive remove path/to/note.md
md-archive rebuild
```

Markdown files must start with closed frontmatter:

Markdown 文件必须以闭合 frontmatter 开头：

```markdown
---
tags: [算法, 动态规划, 背包问题]
title: 01 背包问题详解
---

# 01 背包问题
```

See [docs/CLI.md](docs/CLI.md) for details.

更多细节见 [docs/CLI.md](docs/CLI.md)。

## Force Behavior / `--force` 行为

| Conflict / 冲突 | Without `--force` / 不带 `--force` | With `--force` / 带 `--force` |
| --- | --- | --- |
| Same source path / 相同源路径 | Warn and skip / 警告并跳过 | Replace archive copy and rebuild links / 覆盖归档副本并重建链接 |
| Same title in a tag / 同标签内 title 冲突 | Skip that tag / 跳过该标签 | Replace the tag link / 覆盖标签链接 |

`.archive/` is durable storage, not a temporary cache. `scan` skips `.archive/` and `.tags/`.

`.archive/` 是持久内容副本，不是临时缓存。`scan` 会跳过 `.archive/` 和 `.tags/`。

## Directory Layout / 目录结构

```text
workspace/
├── note1.md
├── note2.md
├── .archive/
│   ├── .gitignore
│   ├── note1.md
│   └── notes/
│       └── note2.md
├── .tags/
│   ├── .gitignore
│   ├── 算法.md
│   ├── 算法/
│   │   └── 01背包问题.md -> ../../.archive/note1.md
│   └── 数据结构/
│       └── 线段树.md -> ../../.archive/notes/note2.md
└── config.ini
```

## Development / 开发

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

The main code is built through `.cppm` module interfaces and `import std;`. See [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md) for toolchain notes.

主代码通过 `.cppm` 模块接口和 `import std;` 构建。工具链注意事项见 [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)。

Public APIs use Doxygen comments in module interface files. Optional generated API docs are available with `-DMD_ARCHIVE_BUILD_API_DOCS=ON`.

公共 API 在模块接口文件中使用 Doxygen 注释。可通过 `-DMD_ARCHIVE_BUILD_API_DOCS=ON` 可选生成 API 文档。

## Contributing / 贡献

Please read:

请阅读：

- [docs/CONTRIBUTING.md](docs/CONTRIBUTING.md)
- [docs/BRANCHING.md](docs/BRANCHING.md)
- [docs/CLI.md](docs/CLI.md)
- [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md)

Do not commit generated `.archive/`, `.tags/`, local `config.ini`, build directories, or binaries.

请不要提交生成的 `.archive/`、`.tags/`、本地 `config.ini`、构建目录或二进制文件。

## License / 许可证

MIT. See [LICENSE](LICENSE).

MIT。见 [LICENSE](LICENSE)。
