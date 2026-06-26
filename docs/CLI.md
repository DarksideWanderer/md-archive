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

`add <file.md> [--force]`: parse frontmatter, copy the source into `.archive/`, create tag symlinks, and update tag indexes.

`add <file.md> [--force]`：解析 frontmatter，复制源文件到 `.archive/`，创建标签符号链接，并更新标签索引。

`scan [--force]`: archive Markdown files under workspace. It skips `.archive/` and `.tags/`.

`scan [--force]`：归档 workspace 下的 Markdown 文件，并跳过 `.archive/` 和 `.tags/`。

`list [tag]`: list all tags or documents under one tag.

`list [tag]`：列出所有标签，或列出某个标签下的文档。

`docs`: list archived documents.

`docs`：列出已归档文档。

`remove <file.md>`: remove tag links and the archive copy for a source file.

`remove <file.md>`：移除某源文件对应的标签链接和归档副本。

`rebuild`: rebuild tag index Markdown files from tag directories.

`rebuild`：根据标签目录重建标签索引 Markdown 文件。

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

`.archive/` stores durable content copies. `.tags/<tag>/` contains symlinks pointing to `.archive/` copies. `.tags/<tag>.md` index files link back to source files for editing.

`.archive/` 保存持久内容副本。`.tags/<tag>/` 中的符号链接指向 `.archive/` 副本。`.tags/<tag>.md` 索引文件链接回源文件，方便编辑。

## Exit Codes / 退出码

```text
0  success / 成功
1  general error / 通用错误
2  invalid arguments / 参数错误
3  config error / 配置错误
4  filesystem error / 文件系统错误
5  parse error / 解析错误（预留）
```
