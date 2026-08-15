# md-archive

`md-archive` 是一个轻量级 C++ CLI 工具，用 YAML frontmatter 中的 `tags` 和 `title` 为 Markdown 文档建立持久归档和标签导航。

`md-archive` is a lightweight C++ CLI that builds a durable Markdown archive and tag navigation system from YAML frontmatter `tags` and `title`.

## Features / 功能特性

- Stores one SHA-256-addressed backup object per unique Markdown content.
- Records every source path in `.archive/index.tsv`, so copies and moves can be distinguished without duplicate backups.
- Creates `.tags/<tag>/` links to existing sources, with durable-object fallback after source deletion.
- Supports `add`, `scan`, `list`, `docs`, `remove`, `rebuild`, `init`, and `config` commands.
- Uses C++23 named modules, `import std;`, CMake, Ninja, and zero external runtime dependencies.

- 按 Markdown 完整内容的 SHA-256 哈希保存唯一备份对象。
- 在 `.archive/index.tsv` 中记录所有源路径，可辨别复制和移动且不产生重复备份。
- 在 `.tags/<tag>/` 下创建指向现有源文件的链接；源文件删除后回退到持久归档对象。
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
4. Create links under `.tags/<tag>/` pointing to the source file while it exists.

Identical files at multiple paths share one object, while every observed source
path remains a separate row in `index.tsv` even after that path disappears.
md-archive never guesses whether equal content represents a copy or a move;
only an explicit `remove` deletes a path mapping. Pre-1.0 path-mirrored archives
are migrated automatically on the first command.

多个路径下内容完全相同的文件共享一个对象，但每个出现过的源路径都会保留独立映射，
即使路径后来消失也不会根据相同内容猜测为移动；只有显式 `remove` 才会删除映射。
1.0 以前按路径镜像的归档会在
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

The Makefile keeps MSYS2's `/usr/bin/cmake`, installs to `/usr`
(`/usr/bin/md-archive.exe`), and uses only the CLANG64 compiler/runtime from
`/clang64/bin`. On macOS/Linux it uses `cmake` from `PATH` and installs to
`/usr/local`. Override with `make install INSTALL_PREFIX=/your/prefix`.

Makefile 会安装到平台对应的 Unix 前缀：MSYS2 使用 `/usr`
（即 `/usr/bin/md-archive.exe`），macOS/Linux 使用 `/usr/local`。可通过
`make install INSTALL_PREFIX=/your/prefix` 覆盖。

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

### Tag filename constraint / 标签文件名限制

Within one tag directory, the same normalized filename cannot simultaneously
represent different hashes. In other words, `.tags/<tag>/<title>.md` is a
single visible slot, not a multi-value index. Without `--force`, a different
hash claiming that filename is rejected. With `--force`, the new document
replaces the visible entry; both source-to-hash mappings remain archived, and
removing the visible source path immediately reveals a remaining mapping.

同一个标签目录中，规范化后的同一文件名不支持同时对应不同 hash。也就是说，
`.tags/<标签>/<标题>.md` 是单一可见入口，不是多值索引。不带 `--force` 时，使用该文件名
但 hash 不同的文档会被拒绝；带 `--force` 时，新文档只会替换当前可见入口。两条源路径到
hash 的归档映射仍分别保留；按源路径移除当前可见文档后，剩余映射会立即恢复为可见入口。

`.archive/` is durable storage, not a temporary cache. `scan` skips `.archive/` and `.tags/`.

`.archive/` 是持久内容副本，不是临时缓存。`scan` 会跳过 `.archive/` 和 `.tags/`。

Commit `.archive/index.tsv` and `.archive/objects/`. The entire `.tags/` tree is
platform-local derived state and remains ignored. After a cross-platform clone, the first
`md-archive` command recreates native document entries from the committed hash
table. On Windows, md-archive falls back to hard links when native symbolic
links are not permitted. `rebuild` remains available for an explicit pass.

应提交 `.archive/index.tsv` 和 `.archive/objects/`。整个 `.tags/` 都是平台本地派生状态并被忽略。
跨平台 clone 后，第一次运行任意
`md-archive` 命令会根据已提交的哈希表重建当前系统的标签链接。Windows 无权创建
原生符号链接时会自动改用硬链接；`rebuild` 可用于显式执行整理。

## What `rebuild` Does / `rebuild` 的预期行为

`rebuild` repairs the derived tag views. Its durable inputs are
`.archive/index.tsv` and `.archive/objects/`; it also inspects existing `.tags/`
entries to normalize symbolic links, Windows hard links, and Git's one-line
link files. For every indexed document it reads frontmatter from the source
when that source still exists, otherwise from the hash-addressed archive object.

`rebuild` 是派生标签视图的修复命令。它以 `.archive/index.tsv` 和
`.archive/objects/` 为持久数据源，同时检查已有 `.tags/` 条目，将符号链接、
Windows 硬链接和 Git 检出的单行链接文件整理为当前平台可用的形式。对每条索引记录，
源文件存在时读取源文件的 frontmatter；源文件已被用户删除时改读哈希归档对象。

It rebuilds `.tags/<tag>/<title>.md` document entries only; root-level
`.tags/<tag>.md` overview pages are not part of v1.1.0 and obsolete ones are
removed. A missing source is not treated as an archive deletion: the recovered
entry links directly to the durable object. Only the explicit
`remove` command removes a source mapping and prunes an unreferenced object.
User-facing `list` and `docs` output never exposes the hash-object path: when
several source paths share a hash, it displays the first indexed source path,
even if that historical path no longer exists.

它只重建 `.tags/<标签>/<标题>.md` 文档入口；v1.1.0 不再提供根级
`.tags/<标签>.md` 概览页，并会移除遗留概览页。源文件消失不等于删除归档：
恢复出的入口会直接链接到持久归档对象。只有显式执行 `remove` 才会移除源路径映射，
并在对象不再被引用时清理对象。
面向用户的 `list` 和 `docs` 不显示哈希对象路径；同一 hash 对应多个源路径时，显示
索引中的第一条原路径，即使该历史路径已经不存在。

Messages printed as `整理标签` describe link/index repair; they do not mean
Markdown content was overwritten. `rebuild` does not re-hash existing sources,
change `index.tsv` mappings, or prune archive objects.

输出中的 `整理标签` 表示修复链接和索引页，并不表示覆盖 Markdown 正文。
`rebuild` 不会重新计算现有源文件的哈希、修改 `index.tsv` 映射或清理归档对象。

## Data Safety and UTF-8 / 数据安全与 UTF-8

- `index.tsv` and hash objects are replaced transactionally through verified
  temporary files and recoverable `.bak` files. Startup restores an interrupted
  replacement automatically.
- A copied object is hashed again before it is installed. Existing objects are
  verified during forced updates and repaired from the source when corrupted.
- Markdown identity normalizes CRLF, LF, and CR to LF before hashing, so Git
  checkout line-ending policy does not create platform-specific hashes. Stored
  object bytes are still preserved exactly and protected by `.gitattributes`.
- A malformed `index.tsv` is never partially parsed and overwritten. Invalid
  absolute or `..` source paths are rejected and the index becomes read-only
  for that invocation.
- Legacy path-mirrored files are deleted only after the new index is safely
  committed, and only when a corresponding source proves they are legacy data.
- CLI paths use UTF-8 conversion through `std::filesystem`; Windows command-line
  arguments are read as UTF-16 and converted to UTF-8. UTF-8 BOM frontmatter,
  Chinese paths, spaces, titles, and tags are covered by tests.
- Tag names are single safe path components on every platform. Traversal,
  separators, control characters, Windows reserved characters, and device names
  are rejected. Reserved document titles are made safe consistently.

- `index.tsv` 和哈希对象均通过已校验的临时文件及可恢复 `.bak` 文件事务式替换；
  启动时会自动恢复被中断的替换。
- 对象副本安装前会再次计算 hash；强制更新时会校验已有对象，并用源文件修复损坏对象。
- Markdown 计算 hash 前会将 CRLF、LF 和 CR 统一视为 LF，避免 Git 换行策略产生平台相关
  hash；归档对象仍精确保留原始字节，并由 `.gitattributes` 禁止 checkout 转换。
- 畸形 `index.tsv` 不会被“解析一半后覆盖”。绝对路径和包含 `..` 的源路径会被拒绝，
  本次运行禁止改写该索引。
- 旧版路径镜像文件只在新索引安全落盘后删除，并且必须存在对应源文件才能判定为旧版数据。
- CLI 路径统一使用 UTF-8 与 `std::filesystem` 转换；Windows 命令行先按 UTF-16 读取再转
  UTF-8。测试覆盖 UTF-8 BOM、中文路径、空格、中文标题和标签。
- 标签名在所有平台都必须是安全的单一路径组件；拒绝路径逃逸、分隔符、控制字符、
  Windows 保留字符及设备名，并统一处理保留文档标题。

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
hash-addressed archive format and automatic legacy migration. `v1.0.1` fixes
automatic Clang discovery from an MSYS2 shell. `v1.0.2` installs into the
MSYS2 `/usr/bin` prefix instead of Windows Program Files. `v1.1.0` unifies
Windows and macOS rebuild behavior around directory-only tag entries, preserves
all source-path aliases, and fixes hash filenames leaking into `list` output.

从 0.2.0 开始，每个发布版本都使用带说明的 Git 标签保留完整代码状态。
`v0.2.0` 是引入哈希存储前的基线；1.0 引入哈希寻址归档格式和旧版自动迁移。
`v1.0.1` 修复从 MSYS2 shell 构建安装时的 Clang 自动发现。
`v1.0.2` 修复 MSYS2 安装前缀，使 `make install` 写入 `/usr/bin`。

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

请提交 `.archive/index.tsv` 和 `.archive/objects/`；不要提交 `.tags/`、
本地 `config.ini`、构建目录或二进制文件。

## License / 许可证

MIT. See [LICENSE](LICENSE).

MIT。见 [LICENSE](LICENSE)。
