# md-archive

`md-archive` 是一个轻量级 C++ CLI 工具，用 YAML frontmatter 中的 `tags` 和 `title` 为 Markdown 文档建立持久归档和标签导航。

`md-archive` is a lightweight C++ CLI that builds a durable Markdown archive and tag navigation system from YAML frontmatter `tags` and `title`.

## Features / 功能特性

- Stores one SHA-256-addressed backup object per unique Markdown content.
- Records every source path in `.archive/index.tsv`, so copies and moves can be distinguished without duplicate backups.
- Creates `.tags/<tag>/` links that point to the original source files.
- Supports `add`, `scan`, `list`, `docs`, `remove`, `rebuild`, `init`, and `config` commands.
- Uses C++23 named modules, `import std;`, CMake, Ninja, and zero external runtime dependencies.

- 按 Markdown 完整内容的 SHA-256 哈希保存唯一备份对象。
- 在 `.archive/index.tsv` 中记录所有源路径，可辨别复制和移动且不产生重复备份。
- 在 `.tags/<tag>/` 下创建准确指向原始源文件的链接。
- 支持 `add`、`scan`、`list`、`docs`、`remove`、`rebuild`、`init` 和 `config`。
- 使用 C++23 命名模块、`import std;`、CMake、Ninja，无外部运行时依赖。

## How It Works / 工作原理

```text
你的 .md 文件（源文件）              .archive/ 目录（SHA-256 内容对象）
─────────────────────────────       ─────────────────────────────────────
dijkstra.md                         .archive/objects/ab/abcdef….md
  ---                              （源文件删除后也不丢失）
  tags: [算法, 图论]
  title: Dijkstra 最短路径           .archive/index.tsv ← 源路径到哈希的表
  ---                               ───────────────────────
segment-tree.md                     .tags/算法/
  ---                                └── Dijkstra 最短路径.md -> ../../dijkstra.md
  tags: [数据结构, 算法]             .tags/图论/
  title: 线段树完全指南                └── Dijkstra 最短路径.md -> ../../dijkstra.md
  ---
```

Each archive operation:

每次归档时：

1. Parse top-of-file YAML frontmatter with `tags` and `title`.
2. Hash the complete file with SHA-256 and store it once under `.archive/objects/`.
3. Update `.archive/index.tsv` with the workspace-relative source path and hash.
4. Create links under `.tags/<tag>/` pointing to the source file.

Identical files at multiple paths share one object. If an indexed path disappears
and identical content appears at a new path, it is treated as a move. Existing
paths remain aliases, representing copies. Pre-1.0 path-mirrored archives are
migrated automatically on the first command.

多个路径下内容完全相同的文件共享一个对象。旧路径消失、相同内容出现在新路径时，
会识别为移动；旧路径仍存在时则作为复制别名保留。1.0 以前按路径镜像的归档会在
首次运行命令时自动迁移。

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
| Same source path / 相同源路径 | Warn and skip / 警告并跳过 | Re-hash content, update source mapping, prune unreferenced object, and rebuild links / 重新计算哈希、更新源映射、清理无引用对象并重建链接 |
| Same title in a tag / 同标签内 title 冲突 | Skip that tag / 跳过该标签 | Replace the tag link / 覆盖标签链接 |

`.archive/` is durable storage, not a temporary cache. `scan` skips `.archive/` and `.tags/`.

`.archive/` 是持久内容副本，不是临时缓存。`scan` 会跳过 `.archive/` 和 `.tags/`。

Commit `.archive/index.tsv` and `.archive/objects/`; they are the portable state
of the archive. `.tags/` remains local and ignored because its link type is
platform-specific. After a cross-platform clone, the first `md-archive` command
uses the committed hash table to recreate tag links native to the current
system. On Windows, md-archive falls back to hard links when native symbolic
links are not permitted. `rebuild` remains available for an explicit pass.

应提交 `.archive/index.tsv` 和 `.archive/objects/`，它们是可跨平台的归档状态。
`.tags/` 因链接类型依赖平台而保持本地并被忽略。跨平台 clone 后，第一次运行任意
`md-archive` 命令会根据已提交的哈希表重建当前系统的标签链接。Windows 无权创建
原生符号链接时会自动改用硬链接；`rebuild` 可用于显式执行整理。

## Directory Layout / 目录结构

```text
workspace/
├── note1.md
├── note2.md
├── .archive/
│   ├── .gitignore
│   ├── index.tsv
│   └── objects/
│       ├── ab/abcdef….md
│       └── f0/f01234….md
├── .tags/
│   ├── .gitignore
│   ├── 算法/
│   │   └── 01背包问题.md -> ../../note1.md
│   └── 数据结构/
│       └── 线段树.md -> ../../note2.md
└── config.ini
```

## Development / 开发

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

## Version Backups / 版本备份

Starting with 0.2.0, every released code state is retained by an annotated Git
tag. `v0.2.0` is the preserved pre-hash-storage baseline; 1.0 introduces the
hash-addressed archive format and automatic legacy migration.

从 0.2.0 开始，每个发布版本都使用带说明的 Git 标签保留完整代码状态。
`v0.2.0` 是引入哈希存储前的基线；1.0 引入哈希寻址归档格式和旧版自动迁移。

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

Commit `.archive/index.tsv` and `.archive/objects/` to preserve portable archive
state. Do not commit `.tags/`, local `config.ini`, build directories, or binaries.

请提交 `.archive/index.tsv` 和 `.archive/objects/` 以保留跨平台归档状态；不要提交
`.tags/`、本地 `config.ini`、构建目录或二进制文件。

## License / 许可证

MIT. See [LICENSE](LICENSE).

MIT。见 [LICENSE](LICENSE)。
