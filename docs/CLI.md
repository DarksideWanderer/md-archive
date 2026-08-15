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

`rebuild`: normalize cross-platform tag links and migrate legacy archive links.

`rebuild`：整理跨平台标签链接并迁移旧版归档链接。

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
existing paths with one hash are copies; a missing old path replaced by a new
path with the same hash is reconciled as a move. `.tags/<tag>/` links point to
the original source files.

`.archive/objects/<前缀>/<sha256>.md` 按内容保存唯一持久对象，
`.archive/index.tsv` 是源路径到哈希的映射表。同一哈希下仍存在的多个路径表示复制；
旧路径消失而相同哈希出现在新路径时会按移动处理。`.tags/<tag>/` 链接直接指向原始源文件。

When Windows cannot create symbolic links, tag entries use hard links instead.
Commit `.archive/index.tsv` and `.archive/objects/`, while keeping `.tags/`
ignored. After a cross-platform clone, every operational command silently
recreates missing tag links from the portable hash index before doing its work.
Legacy checked-out link representations are also normalized. `rebuild`
performs the same pass explicitly.

Windows 无法创建符号链接时，标签条目会自动使用硬链接。跨平台 clone 后，
应提交 `.archive/index.tsv` 和 `.archive/objects/`，并保持 `.tags/` 被忽略。
每个业务命令都会先根据可移植哈希索引静默重建缺失链接，同时兼容旧版跨端链接
表示；`rebuild` 可显式执行同一过程。

## Exit Codes / 退出码

```text
0  success / 成功
1  general error / 通用错误
2  invalid arguments / 参数错误
3  config error / 配置错误
4  filesystem error / 文件系统错误
5  parse error / 解析错误（预留）
```
