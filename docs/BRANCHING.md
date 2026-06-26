# Branching / 分支规范

Use short, scoped branch names:

请使用简短、带模块范围的分支名：

```text
feat(<module>)/short-description
fix(<module>)/short-description
docs(<module>)/short-description
refactor(<module>)/short-description
test(<module>)/short-description
build(<module>)/short-description
ci(<module>)/short-description
chore(<module>)/short-description
```

Common modules / 常见模块：

```text
cli
config
frontmatter
archive
index
filesystem
docs
ci
build
```

Examples / 示例：

```text
feat(cli)/add-version-command
fix(frontmatter)/handle-empty-tags
docs(readme)/update-cmake-build
refactor(archive)/split-path-normalization
test(scan)/skip-generated-directories
ci(github)/add-macos-runner
```

Prefer one topic per branch. If a change touches code and docs for the same behavior, keep them together.

每个分支尽量只处理一个主题。若同一行为需要同时修改代码和文档，可以放在同一分支。
