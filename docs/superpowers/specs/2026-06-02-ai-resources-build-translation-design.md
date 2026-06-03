# V20 截屏 AI 资源、构建、翻译设计

日期：2026-06-02

## 背景

V25 截屏 AI 助手下放 V20 的第 4 步需要补齐后续 UI 步骤会依赖的图标资源、qrc 构建注册和翻译源字符串。本步骤只处理资源和翻译准备，不实现 AI 面板、AI 按钮或 MainWindow 调用链路。

参考文档：

- `/home/ut006498@uos/Desktop/ai-assistant-porting-guide.md`
- `/home/ut006498@uos/Desktop/V25截屏AI助手下放V20_方案一完整实现顺序.md`
- `/home/ut006498@uos/Desktop/V25截屏AI助手下放V20_方案ABC修改大纲需求技术文档.md`

## 目标

1. 从 V25 同步 AI 相关 SVG 图标到 V20；
2. 将 AI 图标注册进 V20 `assets/icons/icons.qrc`；
3. 确认 `src.pro` 已包含 `../assets/icons/icons.qrc`，无需新增资源入口；
4. 将 AI 面板会使用的翻译源字符串同步到 V20 当前构建使用的 `.ts` 文件。

## 图标资源范围

从 V25：

`/data/home/ut006498@uos/code1/deepin-screen-recorder/assets/icons/texts/`

复制到 V20：

`/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/texts/`

文件列表：

- `ai_assistant_32px.svg`
- `ai_assistant_unused_32px.svg`
- `ai_badge_32px.svg`
- `askai_32px.svg`
- `explain_32px.svg`
- `summary_32px.svg`
- `translate_32px.svg`

## qrc 注册

修改 V20：

`/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/icons.qrc`

在已有 `<qresource prefix="/icons/deepin/builtin">` 中追加：

```xml
<file>texts/ai_assistant_32px.svg</file>
<file>texts/ai_assistant_unused_32px.svg</file>
<file>texts/askai_32px.svg</file>
<file>texts/explain_32px.svg</file>
<file>texts/summary_32px.svg</file>
<file>texts/translate_32px.svg</file>
<file>texts/ai_badge_32px.svg</file>
```

## 构建影响

V20 `src/src.pro` 已包含：

```qmake
RESOURCES = ../assets/image/deepin-screen-recorder.qrc \
    ../assets/resources/resources.qrc \
    ../assets/icons/icons.qrc
```

因此本步骤无需修改 `src/src.pro` 的 `RESOURCES`。更新 `icons.qrc` 后，Qt Creator 重新 Run qmake 即可把新增 SVG 编入资源。

## 翻译范围

后续 AI 面板会使用 V25 中已有源字符串：

- `Explain`
- `Summary`
- `Translate`
- `Ask AI`

用户选择方案 A：只同步 `translations.pri` 中列出的 `translations/deepin-screen-recorder_*.ts` 文件。

原因：

- 这些文件参与当前 V20 翻译构建和发布；
- 未列入 `translations.pri` 的 `.ts` 当前通常不会生成/安装对应 `.qm`；
- 限制 diff 范围，便于审查。

## 翻译条目策略

每个目标 `.ts` 文件新增或更新 `AIAssistantWidget` context：

```xml
<context>
    <name>AIAssistantWidget</name>
    <message>
        <source>Explain</source>
        <translation>...</translation>
    </message>
    <message>
        <source>Translate</source>
        <translation>...</translation>
    </message>
    <message>
        <source>Ask AI</source>
        <translation>...</translation>
    </message>
    <message>
        <source>Summary</source>
        <translation>...</translation>
    </message>
</context>
```

优先从 V25 对应语言 `.ts` 复制已有翻译；如果 V25 缺少某个目标语言，则保留源文本作为 translation，避免空缺导致 XML 不完整。

## 验证方式

1. 7 个 SVG 文件存在于 V20 `assets/icons/texts/`；
2. `assets/icons/icons.qrc` 包含 7 个 AI `<file>` 条目；
3. `src/src.pro` 仍包含 `../assets/icons/icons.qrc`；
4. `translations.pri` 中列出的每个 `deepin-screen-recorder_*.ts` 都包含 `AIAssistantWidget` context 和 4 个源字符串；
5. Qt Creator 执行 Run qmake 后构建不报 qrc 或 XML 错误。

## 明确不做

本步骤不做：

- 不新增 `AIAssistantWidget`；
- 不新增 AI 按钮；
- 不接入 Sidebar 或 MainWindow；
- 不修改红点配置；
- 不生成或提交所有历史未参与构建语言的翻译；
- 不重新绘制或修改 SVG 内容。
