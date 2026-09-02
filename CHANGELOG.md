# Changelog / 更新日志

All notable changes to `md-archive` are recorded here. The project follows
[Semantic Versioning](https://semver.org/).

这里记录 `md-archive` 的重要变化；版本号遵循语义化版本规范。

## [1.2.0] - 2026-09-02

### Added / 新增

- Added portable hierarchical tags such as `图论/树/基础`. They are stored as
  nested directories under `.tags/`, while CLI input and output consistently
  use the Unix-style `/` separator on Windows, Linux, and macOS.
- Added `search <word>` for matching document titles and source filenames.
- Added `search -all <word>` (and `--all`) for full Markdown content search,
  including a first-match preview.
- Added terminal-aware colored search output with `NO_COLOR` support.

- 新增 `图论/树/基础` 形式的跨平台层级标签；`.tags/` 内使用嵌套目录，
  Windows、Linux、macOS 的命令行输入输出统一使用 Unix 风格 `/`。
- 新增 `search <word>`，按文档标题和源文件名搜索。
- 新增 `search -all <word>`（也支持 `--all`），搜索 Markdown 全文并显示首个匹配行。
- 搜索结果在交互式终端中提供颜色区分，并支持 `NO_COLOR`。

### Changed / 变更

- `list`, `docs`, and `search` now enumerate `.archive/index.tsv` and durable
  objects directly instead of treating `.tags/` as the source of truth.
- Missing or damaged `.tags/` views no longer hide archived documents; the view
  is repaired automatically without requiring a manual `scan` or `rebuild`.
- Tag validation is performed component by component, rejecting traversal,
  empty components, backslashes, control characters, and Windows-reserved names.
- Version reported by the CLI is now `1.2.0`.

- `list`、`docs` 和 `search` 改为直接枚举 `.archive/index.tsv` 与持久归档对象，
  不再把 `.tags/` 当作事实来源。
- `.tags/` 缺失或损坏时不再漏掉已归档文档，并会自动修复，无需先执行 `scan` 或 `rebuild`。
- 标签改为逐层校验，拒绝路径穿越、空层级、反斜杠、控制字符和 Windows 保留名。
- CLI 报告的版本更新为 `1.2.0`。

### Tests / 测试

- Added end-to-end coverage for nested Unicode tags, exact `A/B/C` lookup,
  automatic view recovery, name-only search, full-text search, and unsafe tags.

- 新增端到端测试，覆盖 Unicode 层级标签、`A/B/C` 精确查询、自动恢复视图、
  名称搜索、全文搜索及危险标签输入。

## 1.1.0 - 2026-08-15

> Development milestone recorded in the repository history but not published
> as a Git tag. / 此版本是仓库历史中的开发里程碑，未发布同名 Git tag。

- Unified Windows and macOS rebuild behavior around directory-only tag entries.
- Preserved every source-path alias for shared content hashes.
- Prevented internal hash-object filenames from leaking into `list` output.
- Improved recovery of tag links after source deletion and across platforms.

- 统一 Windows 与 macOS 对目录式标签入口的重建行为。
- 保留同一内容哈希对应的所有源路径别名。
- 修复 `list` 输出泄露内部哈希对象文件名的问题。
- 改进源文件删除后及跨平台场景中的标签链接恢复。

## [1.0.1] - 2026-08-15

- Fixed MSYS2 Clang discovery during configure and install workflows.
- 修复配置和安装流程中的 MSYS2 Clang 自动发现。

## [1.0.0] - 2026-08-15

- Introduced SHA-256-addressed durable objects under `.archive/objects/`.
- Added `.archive/index.tsv` source-path mappings and automatic legacy migration.
- Added deduplication for identical Markdown content while retaining distinct source paths.

- 引入 `.archive/objects/` 下按 SHA-256 寻址的持久对象。
- 新增 `.archive/index.tsv` 源路径映射及旧版归档自动迁移。
- 相同 Markdown 内容共享对象，同时保留各自独立的源路径。

## [0.2.0] - 2026-08-15

- Preserved the pre-hash-storage implementation as a historical release.
- Improved UTF-8 and Windows path handling.

- 将引入哈希存储之前的实现保留为历史版本。
- 改进 UTF-8 与 Windows 路径处理。

[1.2.0]: https://github.com/DarksideWanderer/md-archive/compare/v1.0.1...windows-update
[1.0.1]: https://github.com/DarksideWanderer/md-archive/releases/tag/v1.0.1
[1.0.0]: https://github.com/DarksideWanderer/md-archive/releases/tag/v1.0.0
[0.2.0]: https://github.com/DarksideWanderer/md-archive/releases/tag/v0.2.0
