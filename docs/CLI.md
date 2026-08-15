# CLI Reference / 命令行参考

## Overview / 总览

```bash
md-archive --help
md-archive --version
md-archive --config <path> --workspace <path> <command>
md-archive init [--force]
md-archive config show
md-archive config path
md-archive add <file.md> [--force]
md-archive scan [--force]
md-archive list [tag]
md-archive docs
md-archive remove <file.md>
md-archive rebuild
```

## Global Options / 全局参数

`--config <path>` uses an explicit config file.

`--config <path>` 使用指定配置文件。

`--workspace <path>` overrides the workspace only for the current command.

`--workspace <path>` 仅在本次命令中覆盖 workspace。

## Commands / 命令

`init [--force]`: create `config.ini` in the current directory. Existing files are not overwritten unless `--force` is provided.

`init [--force]`：在当前目录创建 `config.ini`。默认不覆盖已有文件，除非传入 `--force`。

`config show`: print the effective config file path, workspace, `tags_dir`, and `archive_dir`.

`config show`：显示最终生效的配置文件路径、workspace、`tags_dir` 和 `archive_dir`。

`config path`: print the config file path, or explain that built-in defaults are being used.

`config path`：显示配置文件路径；若使用内置默认值，则说明未找到配置文件。

`add <file.md> [--force]`: parse frontmatter, store a SHA-256-addressed backup object, update the source/hash table, and create tag links to the source file.

`add <file.md> [--force]`：解析 frontmatter，保存 SHA-256 内容对象、更新源路径哈希表，并创建指向源文件的标签链接。

`scan [--force]`: archive Markdown files under workspace. It skips `.archive/` and `.tags/`.

`scan [--force]`：归档 workspace 下的 Markdown 文件，并跳过 `.archive/` 和 `.tags/`。

`list [tag]`: list all tags or documents under one tag.

`list [tag]`：列出所有标签，或列出某个标签下的文档。

`docs`: list archived documents.

`docs`：列出已归档文档。

`remove <file.md>`: remove its tag links and source mapping; delete the content object only when no other source path references it.

`remove <file.md>`：移除对应标签链接和源路径映射；仅在没有其他源路径引用时删除内容对象。

`rebuild`: reconstruct directory-based document entries, normalize
cross-platform link representations, and migrate legacy archive links. It uses
the source when present and the durable hash object when the source was deleted.

`rebuild`：重建目录式文档入口、整理跨平台链接表示，并迁移旧版归档链接。
源文件存在时使用源文件；源文件已删除时使用持久哈希对象。

### Rebuild guarantees / Rebuild 保证

- `.archive/index.tsv` and `.archive/objects/` are the durable source of truth.
- `.tags/<tag>/<title>.md` is the only tag view. Root overview files are not generated.
- Deleting a source file directly does not delete its archive record. `rebuild`
  recovers the directory entry from the indexed object and links it to that object.
- Only `remove` deletes a source mapping and may prune an unreferenced object.
- `rebuild` does not re-hash sources, change mappings, or delete archive objects.
- `list` and `docs` display the first indexed source path for a hash, never the
  internal `.archive/objects/<hash>.md` path.

- `.archive/index.tsv` 和 `.archive/objects/` 是持久事实来源。
- `.tags/<标签>/<标题>.md` 是唯一标签视图；不生成根级概览文件。
- 直接删除源文件不会删除归档记录；`rebuild` 会从索引对象恢复目录入口并链接到该对象。
- 只有 `remove` 会删除源路径映射，并可能清理不再被引用的对象。
- `rebuild` 不会重新计算源文件 hash、改变映射或删除归档对象。
- `list` 和 `docs` 对同一 hash 显示索引中的第一条源路径，绝不显示内部
  `.archive/objects/<hash>.md` 路径。

`整理标签` means a link representation was repaired. It
does not mean that archived Markdown content was forcibly overwritten.

`整理标签` 表示链接形式得到修复，不表示归档 Markdown 正文被强制覆盖。

### Same-filename limitation / 同名文件限制

One `.tags/<tag>/<title>.md` filename cannot simultaneously point to different
hashes. A collision is skipped by default; `--force` replaces the single visible
entry but does not merge or delete the independent archive mappings. Use
`remove <source-path>` to remove one precise mapping; if another mapping can
occupy that filename, it is restored immediately.

同一 `.tags/<标签>/<标题>.md` 文件名不支持同时指向不同 hash。默认跳过冲突；
`--force` 只替换唯一的可见入口，不会合并或删除各自独立的归档映射。使用
`remove <源路径>` 精确删除一条映射；若还有映射可以占用该文件名，会立即恢复。

## Frontmatter / Frontmatter 要求

Each archived file must start with closed YAML frontmatter and include both `tags` and `title`:

每个归档文件必须以闭合的 YAML frontmatter 开头，并包含 `tags` 和 `title`：

```markdown
---
tags: [算法, 图论]
title: Dijkstra 最短路径
---
```

## Configuration Discovery / 配置查找

Search order / 查找顺序：

1. `--config <path>`
2. `./config.ini`
3. parent directories / 父目录逐级向上
4. `~/.config/md-archive/config.ini` or `%APPDATA%/md-archive/config.ini`
5. built-in defaults: current directory as workspace, `.tags`, `.archive` / 内置默认值

`tags_dir` and `archive_dir` must be relative paths inside workspace. Absolute paths and `..` are rejected.

`tags_dir` 和 `archive_dir` 必须是 workspace 内的相对路径。绝对路径和 `..` 会被拒绝。

## Archive Semantics / 归档语义

`.archive/objects/<prefix>/<sha256>.md` stores one durable object per unique
file content. `.archive/index.tsv` is the source-path-to-hash table. Multiple
source paths may share one hash and one object. Every observed path remains an
independent mapping even after it disappears; equal content is never used to
guess that a file moved. Only `remove` deletes a mapping. `.tags/<tag>/` entries prefer
the original source and fall back to the durable object if that source is gone.

`.archive/objects/<前缀>/<sha256>.md` 按内容保存唯一持久对象，
`.archive/index.tsv` 是源路径到哈希的映射表。多个源路径可以共享同一哈希和对象；
即使路径已经消失也会保留其独立映射，不会根据内容相同猜测文件发生了移动。
只有 `remove` 会删除映射。`.tags/<tag>/` 优先指向原始源文件，
源文件消失后回退到持久对象。

When Windows cannot create symbolic links, tag entries use hard links instead.
Commit `.archive/index.tsv` and `.archive/objects/`. Keep the entire `.tags/`
tree ignored as platform-local derived state. After a
cross-platform clone, every operational command silently
recreates missing tag links from the portable hash index before doing its work.
Legacy checked-out link representations are also normalized. `rebuild`
performs the same pass explicitly, removes obsolete root overview files, and does not
delete archive mappings or objects.

Windows 无法创建符号链接时，标签条目会自动使用硬链接。跨平台 clone 后，
应提交 `.archive/index.tsv` 和 `.archive/objects/`；整个 `.tags/` 都是平台本地派生状态并应忽略。
每个业务命令都会先根据可移植哈希索引静默重建缺失链接，同时兼容旧版跨端链接
表示；`rebuild` 可显式执行同一过程并移除遗留根级概览页，但不会删除归档映射或对象。

## Exit Codes / 退出码

```text
0  success / 成功
1  general error / 通用错误
2  invalid arguments / 参数错误
3  config error / 配置错误
4  filesystem error / 文件系统错误
5  parse error / 解析错误（预留）
```
