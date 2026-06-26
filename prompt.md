请你作为资深 C++ CLI 工具维护者、构建系统工程师和开源项目 reviewer，阅读当前项目源码，并基于 `README.md` 中描述的项目目标，对 `md-archive` 进行一次现代化改造。

## 项目背景

`md-archive` 是一个轻量级 C++ CLI 工具，用于将 Markdown 文档按 YAML frontmatter 中的 `tags` 和 `title` 自动归档。

核心行为：

1. 解析 Markdown 文件顶部 YAML frontmatter：

   * `tags: [...]`
   * `title: ...`
2. 将完整 Markdown 内容复制到 `.archive/` 目录，镜像工作区目录结构。
3. 在 `.tags/<tag>/` 下创建符号链接，指向 `.archive/` 中的副本。
4. 生成或更新 `.tags/<tag>.md` 标签索引文件，索引链接指向原始文件，方便在 VS Code 中点击编辑。
5. 源文件删除后，`.archive/` 副本仍然保留，标签目录下的符号链接不应断裂。

当前 README 中已有 CLI：

```bash
md-archive add path/to/note.md
md-archive add path/to/note.md --force
md-archive scan
md-archive list
md-archive list 算法
md-archive docs
md-archive remove path/to/note.md
md-archive rebuild
```

当前配置文件：

```ini
[archive]
workspace   = /Users/you/your-notebook
tags_dir    = .tags
archive_dir = .archive
```

当前技术栈：

* C++23
* clang++
* 标准库 `std::filesystem`
* 零外部依赖
* Makefile 构建
* macOS 优先

## 总目标

请把这个项目改造成一个更现代、可维护、适合开源协作的 C++ CLI 项目。

重点包括：

1. 使用 Clang + C++20/C++23 Modules 进行编译。
2. 保持 CLI 为主要入口。
3. 从 Makefile 迁移或补充到 CMake。
4. 添加 `.github/` 目录，包括 Issue、PR 模板和 CI。
5. 创建 `docs/` 目录，说明开发、CLI、贡献、分支与 PR 规范。
6. 所有协作文档使用中英文双语，英文和中文放在同一个文件中。
7. 主动发现 README、CLI、构建系统、测试、配置、错误处理中的不完善点，并尽可能落地优化。
8. 保持项目“零外部运行时依赖”的特点，不要随意引入大型依赖。

## 一、先理解项目

请先阅读：

* `README.md`
* 现有源码
* Makefile
* 配置文件示例
* 目录结构

然后判断：

* 当前源码入口在哪里
* 是否已经有命令解析逻辑
* frontmatter 解析是否健壮
* `config.ini` 读取是否清晰
* `.archive/` 和 `.tags/` 的路径处理是否安全
* `--force` 行为是否符合 README
* 是否已有测试
* 是否适合模块化拆分

补充要求：

## 源码优先原则

README 只能作为项目意图参考，**不能视为最新事实来源**。

请优先阅读和理解源码，实际行为以源码为准：

1. 如果 README 与源码不一致，以源码为准。
2. 如果源码行为明显有 bug 或不符合项目目标，请修复源码，并同步更新 README。
3. 不要为了强行贴合 README 而破坏已有可运行逻辑。
4. 在最终总结中列出：

   * README 与源码不一致的地方
   * 已经修复的不一致
   * 暂时保留的 TODO

## 配置系统智能化

请重点优化 `config.ini` 的发现、读取和容错能力。

当前 README 说 `config.ini` 必须放在当前工作目录，但这可能不是最终设计。请基于源码和 CLI 使用体验改进配置系统。

建议实现：

1. 配置文件查找顺序：

```text
1. CLI 参数指定：--config <path>
2. 当前工作目录：./config.ini
3. 从当前目录向上查找 config.ini，直到文件系统根目录
4. 用户配置目录：
   - macOS/Linux: ~/.config/md-archive/config.ini
   - Windows: %APPDATA%/md-archive/config.ini
5. 如果仍找不到，给出清晰错误和 config.example.ini 示例路径
```

2. CLI 增加全局参数：

```bash
md-archive --config <path> <command>
md-archive --workspace <path> <command>
```

其中：

* `--config` 显式指定配置文件
* `--workspace` 临时覆盖配置中的 workspace

3. 配置默认值：

如果配置文件存在但字段不完整，可以使用合理默认值：

```ini
[archive]
tags_dir    = .tags
archive_dir = .archive
```

`workspace` 的默认策略：

* 如果配置中有 `workspace`，使用它
* 如果用户传了 `--workspace`，优先使用 CLI 参数
* 如果没有 workspace，但通过向上查找发现了 config.ini，则使用 config.ini 所在目录作为 workspace
* 如果没有 config.ini，则默认当前目录为 workspace，但需要在首次写入前提示或生成配置

4. 新增命令：

```bash
md-archive init
md-archive config show
md-archive config path
```

行为建议：

```bash
md-archive init
```

在当前目录生成 `config.ini`，如果已存在则不覆盖，除非传入 `--force`。

```bash
md-archive config show
```

显示最终生效配置，包括：

* config file path
* workspace
* tags_dir
* archive_dir

```bash
md-archive config path
```

显示当前实际使用的配置文件路径；如果使用默认配置，也说明没有找到配置文件。

5. 路径安全：

请确保：

* `workspace` 必须转换为 canonical/absolute path
* 被归档文件必须位于 workspace 内
* `tags_dir` 和 `archive_dir` 必须是 workspace 内部路径
* 禁止 `tags_dir = ../xxx`
* 禁止 `archive_dir = ../xxx`
* 避免路径逃逸 workspace

6. README 和 docs 同步更新：

如果你实现了智能配置查找，请更新 README、`docs/CLI.md` 和 `docs/DEVELOPMENT.md`，不要继续写“config.ini 必须在当前工作目录”。


## 二、现代化目录结构建议

请尽量整理成类似结构：

```text
md-archive/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── config.example.ini
├── .clang-format
├── .gitignore
├── .github/
│   ├── ISSUE_TEMPLATE/
│   │   ├── bug_report.md
│   │   └── feature_request.md
│   ├── pull_request_template.md
│   └── workflows/
│       └── ci.yml
├── docs/
│   ├── CONTRIBUTING.md
│   ├── BRANCHING.md
│   ├── DEVELOPMENT.md
│   └── CLI.md
├── src/
│   ├── main.cpp
│   ├── md_archive.cppm
│   ├── cli.cppm
│   ├── config.cppm
│   ├── frontmatter.cppm
│   ├── archive.cppm
│   └── index.cppm
└── tests/
    ├── CMakeLists.txt
    └── ...
```

如果当前项目结构不适合一次性改成这样，请采用渐进式方案，不要为了结构漂亮而破坏可构建性。

## 三、CMake + Clang + Modules

请添加或改进 `CMakeLists.txt`，目标是可以这样构建：

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

要求：

* 使用 C++23，必要时兼容 C++20 Modules。
* 优先使用 Clang。
* 尽量使用 CMake 原生 module 支持。
* 如果当前 CMake/Clang 版本对 modules 支持不稳定，请保留 fallback：

  * 可以先使用传统 `.cpp` 构建保证可用
  * 同时提供 `.cppm` module interface
  * 在 `docs/DEVELOPMENT.md` 中说明 module 构建的限制和 TODO
* 不要盲目删除 Makefile；如果保留 Makefile，请让它调用 CMake，或者在 README 中说明旧构建方式与新构建方式。

## 四、模块化拆分建议

请根据项目职责，把代码拆分为清晰模块。推荐模块：

### `cli`

负责：

* 命令解析
* `--help`
* `--version`
* 错误信息
* 子命令分发

### `config`

负责：

* 读取 `config.ini`
* 解析 `[archive]`
* 校验 `workspace`
* 解析 `tags_dir`
* 解析 `archive_dir`

### `frontmatter`

负责：

* 检测 Markdown 文件顶部 `---`
* 解析 `tags`
* 解析 `title`
* 对缺失字段给出清晰错误

### `archive`

负责：

* 复制源文件到 `.archive/`
* 镜像 workspace 相对路径
* 处理 `--force`
* 清理旧符号链接
* remove 行为

### `index`

负责：

* 创建 `.tags/<tag>/` 目录
* 创建符号链接
* 更新 `.tags/<tag>.md`
* rebuild 所有索引

### `filesystem_utils`

负责：

* 路径规范化
* 相对路径计算
* 安全检查，避免路径逃逸 workspace

如果一次性 module 化风险较高，请至少把逻辑拆分为传统 `.hpp/.cpp`，并为后续 module 化留下接口边界。

## 五、CLI 行为优化

请保证或补充以下命令：

```bash
md-archive --help
md-archive --version

md-archive add <file> [--force]
md-archive scan [--force]
md-archive list [tag]
md-archive docs
md-archive remove <file>
md-archive rebuild
```

请改进：

* 参数错误时输出简洁帮助
* 未找到 `config.ini` 时给出明确提示
* frontmatter 缺少 `tags` 或 `title` 时给出文件名和原因
* 路径不在 workspace 内时拒绝处理
* 符号链接创建失败时说明原因
* `scan` 时跳过 `.archive/` 和 `.tags/` 目录，避免递归归档生成物
* 所有命令返回合理 exit code

建议错误码：

```text
0  success
1  general error
2  invalid arguments
3  config error
4  filesystem error
5  parse error
```

## 六、测试

如果项目还没有测试，请添加最小测试体系。

可以使用：

* CTest
* 简单 C++ 测试程序
* shell 测试脚本

测试至少覆盖：

1. 解析 frontmatter：

   * 正常 `tags: [算法, 图论]`
   * 正常 `title: Dijkstra 最短路径`
   * 缺少 tags
   * 缺少 title
   * 无 frontmatter
2. archive 行为：

   * 归档后 `.archive/` 存在副本
   * `.tags/<tag>/` 下符号链接存在
   * `.tags/<tag>.md` 索引存在
3. `--force` 行为：

   * 同路径重复 add
   * 不同文件相同 title
4. scan 行为：

   * 能扫描普通 md
   * 跳过 `.archive/`
   * 跳过 `.tags/`

如果测试无法完全实现，请至少创建测试框架和 TODO。

## 七、GitHub 文件

请创建：

```text
.github/ISSUE_TEMPLATE/bug_report.md
.github/ISSUE_TEMPLATE/feature_request.md
.github/pull_request_template.md
.github/workflows/ci.yml
```

所有模板中英文放在同一个文件。

### Bug Report 模板包含：

* Summary / 问题概述
* Environment / 环境信息
* Steps to Reproduce / 复现步骤
* Expected Behavior / 期望行为
* Actual Behavior / 实际行为
* Logs or Screenshots / 日志或截图
* Additional Context / 额外信息

### Feature Request 模板包含：

* Motivation / 动机
* Proposed Solution / 期望方案
* Alternatives / 替代方案
* CLI Design / CLI 设计
* Compatibility / 兼容性影响
* Willing to Submit PR / 是否愿意提交 PR

### PR 模板包含：

* Summary / 改动概述
* Type of Change / 改动类型
* Related Issue / 关联 Issue
* Testing / 测试情况
* Documentation / 文档更新
* Checklist / 检查清单

### CI 要求：

GitHub Actions 至少在 Ubuntu 上执行：

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

如果 macOS 支持是项目重点，可以再添加 macOS runner。

## 八、docs 文档

请创建以下文档，全部中英双语：

```text
docs/CONTRIBUTING.md
docs/BRANCHING.md
docs/DEVELOPMENT.md
docs/CLI.md
```

### `docs/CONTRIBUTING.md`

说明：

* 如何报告 bug
* 如何请求功能
* 如何提交 PR
* 代码风格
* 测试要求
* 文档要求
* 不要提交 `.archive/` 和 `.tags/` 生成物

### `docs/BRANCHING.md`

说明分支命名：

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

模块名示例：

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

示例：

```text
feat(cli)/add-version-command
fix(frontmatter)/handle-empty-tags
docs(readme)/update-cmake-build
refactor(archive)/split-path-normalization
test(scan)/skip-generated-directories
ci(github)/add-macos-runner
```

### `docs/DEVELOPMENT.md`

说明：

* 推荐环境
* Clang 版本
* CMake 版本
* Debug 构建
* Release 构建
* 运行测试
* 本地运行 CLI
* module 构建注意事项
* 常见问题

### `docs/CLI.md`

说明：

* 命令总览
* 每个命令的用途
* 参数
* 示例
* exit code
* `config.ini` 查找规则
* `.archive/` 与 `.tags/` 的行为

## 九、README 优化

请把 README 改成更适合开源项目的结构，同时保留现有中文说明中的核心图示和行为解释。

README 至少包含：

* Project name
* 中文简介
* English introduction
* Features / 功能特性
* How it works / 工作原理
* Installation / 安装
* Build from source / 从源码构建
* Configuration / 配置
* CLI usage / 命令行使用
* Force behavior / `--force` 行为
* Directory layout / 目录结构
* Development / 开发
* Contributing / 贡献
* License / 许可证

请注意：

* README 中英文可以分段并列，不需要完全逐句翻译。
* 保留 `.archive/` 是内容副本、`.tags/` 是导航系统这一核心设计。
* 明确说明 `.tags/<tag>/` 的 symlink 指向 `.archive/` 副本，而 `.tags/<tag>.md` 索引链接指向源文件。
* 明确说明 `config.ini` 当前只从当前工作目录读取，不向上查找；如果你实现了向上查找，请同步更新 README。

## 十、代码质量文件

请添加：

```text
.clang-format
.gitignore
config.example.ini
```

`.gitignore` 至少包含：

```gitignore
build/
.cache/
.DS_Store
compile_commands.json
.archive/
.tags/
config.ini
```

`config.example.ini` 基于 README 中的配置。

`.clang-format` 使用现代 C++ 风格，保持可读性。

## 十一、不要做的事

请避免：

* 不要引入大型第三方库。
* 不要破坏 README 中承诺的核心归档语义。
* 不要把 `.archive/` 设计成易丢数据的临时缓存。
* 不要让 `scan` 扫描 `.archive/` 或 `.tags/`。
* 不要只写文档不改代码。
* 不要假装完全支持 modules；如果编译器或 CMake 限制导致暂时不能完整 module 化，要诚实写 TODO。

## 十二、最终输出格式

完成后，请输出一份总结，包含：

````markdown
## Summary

- ...

## Files Changed

- Modified:
  - ...
- Added:
  - ...

## Build

```bash
cmake -S . -B build -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
````

## Test

```bash
ctest --test-dir build --output-on-failure
```

## Notes

* 已完成：

  * ...
* TODO：

  * ...
* README 与源码不一致之处：

  * ...

```

请直接修改项目文件，尽量让项目在本地可以构建和运行。不要只给建议。
```
