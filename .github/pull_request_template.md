## Summary / 改动概述

Describe the change and why it is needed.

请说明改动内容和原因。

## Type of Change / 改动类型

- [ ] feat / 新功能
- [ ] fix / 问题修复
- [ ] docs / 文档
- [ ] refactor / 重构
- [ ] test / 测试
- [ ] build or ci / 构建或 CI

## Related Issue / 关联 Issue

Closes #

## Testing / 测试情况

Commands run:

```bash
cmake -S . -B build -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

## Documentation / 文档更新

- [ ] README updated / 已更新 README
- [ ] docs updated / 已更新 docs
- [ ] Not needed / 不需要

## Checklist / 检查清单

- [ ] The CLI behavior is documented / CLI 行为已记录
- [ ] Tests cover the changed behavior / 测试覆盖改动行为
- [ ] Generated `.archive/` and `.tags/` files are not committed / 未提交生成物
- [ ] Error messages are clear / 错误信息清晰
