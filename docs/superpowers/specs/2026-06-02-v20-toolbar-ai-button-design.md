# V20 工具条 AI 按钮与 AI 面板设计

## 背景

V25 截屏工具已具备 AI 截图助手能力，包括解释、总结、翻译、问问 AI 等功能。当前需求是在 V20 截屏工具中先增加 AI 按钮入口和 AI 面板展示能力，用于打通 UI 入口链路。

本设计只覆盖 V20 工具条按钮和右侧 AI 面板，不实现截图静默保存、剪贴板复制或 AI DBus 调用。

## 目标

在 V20 普通截图工具条中新增 AI 按钮。用户点击 AI 按钮后，右侧二级面板切换为 AI 面板，并展示四个 AI 功能按钮：

- Explain / 解释
- Summary / 总结
- Translate / 翻译
- Ask AI / 问问 AI

四个功能按钮点击后只发出功能选择信号，后续业务处理在下一阶段接入。

## 非目标

本次不实现以下内容：

- AI 按钮红点或 badge
- `A` 快捷键打开 AI 面板
- 截图静默保存
- 复制截图到剪贴板
- 调用 `launchAiQuickOCR`
- 调用 `launchChatUploadImage`
- AI 调用完成后退出截图工具
- 滚动截图 AI 支持

## 已确认需求

| 项目 | 决策 |
|------|------|
| AI 按钮位置 | 放在贴图按钮后、滚动截图/OCR 前 |
| 功能范围 | AI 按钮 + AI 面板 |
| 面板承载方式 | 复用 V20 右侧二级面板机制 |
| AI 服务不可用表现 | 隐藏 AI 按钮 |
| 红点/badge | 本次不做 |
| 面板功能按钮点击 | 只发出 `functionSelected(...)` 信号，不接业务 |
| 面板样式 | 复用 V25 四个 AI 功能按钮样式和图标资源 |
| 快捷键 | 本次不加 `A` 快捷键 |

## 方案选择

采用方案一：在 `SideBarWidget` 中新增独立 `AIAssistantWidget`。

该方案与 V25 的设计方向一致：AI 面板是独立组件，通过 `"aiassistant"` 工具类型接入工具条和二级面板分发链路，而不是作为绘图工具页塞入 `ShotToolWidget`。

### 选择理由

- AI 面板不是绘图参数面板，不应污染 `ShotToolWidget` 的形状、粗细、颜色逻辑。
- `SideBarWidget` 已负责右侧二级面板展示，适合管理 AI 面板显示/隐藏。
- 后续接入完整 AI 调用链路时，`AIAssistantWidget::functionSelected(...)` 可以继续向 `MainWindow` 或 AI controller 转发。
- 相比独立浮窗方案，不需要新增多屏边界、窗口避让、浮窗定位逻辑。

## 整体架构

核心链路：

```text
SubToolWidget::m_aiAssistantButton
  -> click
  -> emit changeShotToolFunc("aiassistant")
  -> MainWindow::changeShotToolEvent("aiassistant")
  -> SideBar::changeShotToolFunc("aiassistant")
  -> SideBarWidget::changeShotToolWidget("aiassistant")
  -> 显示 AIAssistantWidget
```

AI 面板按钮链路：

```text
AIAssistantWidget 功能按钮点击
  -> emit functionSelected(AIFunction)
  -> 本次暂不接保存、剪贴板或 DBus 调用
```

## 组件设计

### SubToolWidget

职责：新增并管理 V20 工具条 AI 入口按钮。

修改点：

- 在 `SubToolWidget::initShotLabel()` 中新增 `m_aiAssistantButton`。
- 按钮位置放在 `m_pinButton` 之后、`m_scrollShotButton` 和 `m_ocrButton` 之前。
- 按钮图标使用 `ai_assistant`。
- tooltip 使用 `AI Screenshot` 或中文翻译后的“AI 截图”。
- AI 服务不可用时隐藏按钮。
- 第三方调用截图时隐藏按钮。
- 点击按钮后发出 `changeShotToolFunc("aiassistant")`。

不做：

- 不调用 `setShowRedDot(...)`。
- 不读写 `ai_assistant_used`。
- 不接 `A` 快捷键。

### AIAssistantWidget

职责：独立展示 AI 功能按钮，并发出用户选择的 AI 功能。

新增文件：

```text
src/widgets/aiassistantwidget.h
src/widgets/aiassistantwidget.cpp
```

接口设计：

```cpp
class AIAssistantWidget : public DWidget
{
    Q_OBJECT
public:
    enum AIFunction {
        Explain,
        Summarize,
        Translate,
        AskAI
    };
    Q_ENUM(AIFunction)

signals:
    void functionSelected(AIFunction function);
};
```

按钮：

| 按钮 | 枚举 | 图标名 |
|------|------|--------|
| Explain | `Explain` | `explain` |
| Summary | `Summarize` | `summary` |
| Translate | `Translate` | `translate` |
| Ask AI | `AskAI` | `askai` |

本次 `AIAssistantWidget` 不直接依赖 `MainWindow`、截图保存逻辑或 AI DBus 接口。

### SideBarWidget

职责：在原绘图二级面板和 AI 面板之间切换。

新增成员：

```cpp
AIAssistantWidget *m_aiAssistantWidget = nullptr;
```

`changeShotToolWidget(const QString &func)` 行为：

```text
if func == "aiassistant":
    隐藏 ShotToolWidget
    隐藏 ColorToolWidget
    显示 AIAssistantWidget
else:
    隐藏 AIAssistantWidget
    显示 ShotToolWidget
    显示 ColorToolWidget
    执行原逻辑：
        m_shotTool->switchContent(func)
        m_colorTool->setFunction(func)
```

这样 AI 面板不会影响原绘图工具的颜色、粗细和形状状态。

### MainWindow

职责：继续承担工具类型分发，但需要避免把 `"aiassistant"` 当作绘图形状处理。

建议在 `MainWindow::changeShotToolEvent(const QString &func)` 中增加轻量特殊分支：

```text
if func == "aiassistant":
    updateSideBarPos()
    m_sideBar->changeShotToolFunc("aiassistant")
    return
```

避免 AI 入口进入以下绘图逻辑：

```cpp
initShapeWidget(func);
m_shapesWidget->setCurrentShape(func);
```

## 资源设计

复用 V25 AI 资源。

需要资源：

```text
assets/icons/texts/ai_assistant_32px.svg
assets/icons/texts/explain_32px.svg
assets/icons/texts/summary_32px.svg
assets/icons/texts/translate_32px.svg
assets/icons/texts/askai_32px.svg
```

本次不需要：

```text
assets/icons/texts/ai_badge_32px.svg
assets/icons/texts/ai_assistant_unused_32px.svg
```

优先使用主题图标名：

```cpp
QIcon::fromTheme("ai_assistant")
QIcon::fromTheme("explain")
QIcon::fromTheme("summary")
QIcon::fromTheme("translate")
QIcon::fromTheme("askai")
```

如果 V20 资源体系不能通过主题名加载，再使用明确 qrc 路径作为降级方案。

## 构建设计

V20 工程同时存在 qmake 和 CMake 文件。新增文件需要加入实际使用的构建系统；如双构建仍保留，则两个都补齐。

需要检查并更新：

```text
src/src.pro
src/CMakeLists.txt
assets/icons/icons.qrc
```

新增文案使用 `tr(...)` 包裹，后续通过翻译流程提取：

```text
AI Screenshot
Explain
Summary
Translate
Ask AI
```

## AI 服务检测设计

因为已确认 AI 服务不可用时隐藏按钮，需要提供轻量检测接口：

```cpp
bool DBusUtils::isAiAssistantAvailable();
```

检测逻辑：

```text
1. 检查 session bus 是否连接。
2. 优先检测 com.deepin.copilot。
3. 可回退检测 com.iflytek.aiassistant。
4. 检查服务接口中是否包含：
   - launchAiQuickOCR
   - launchChatUploadImage
5. 两个方法都存在时认为 AI 服务可用。
```

本次该接口只用于按钮可见性判断，不用于 AI 调用。

## 错误处理

### AI 服务不可用

- 隐藏 AI 按钮。
- 不弹窗，不显示置灰入口。

### 图标资源缺失

- 构建阶段确认资源加入 qrc。
- 运行时如果图标未加载，不影响按钮信号和 tooltip。

### 点击 AI 按钮

期望行为：

- 显示 AI 面板。
- 隐藏原 `ShotToolWidget` 和 `ColorToolWidget`。
- 不初始化绘图 shape。
- 不改变当前绘图参数。
- 不保存截图。
- 不退出截图工具。

### 点击 AI 功能按钮

期望行为：

- 只发出 `functionSelected(...)`。
- 不调用 DBus。
- 不保存截图。
- 不退出截图工具。

## 测试计划

### 构建测试

- qmake/CMake 构建通过。
- `aiassistantwidget.h/.cpp` 正确进入构建。
- 资源正确进入 qrc。
- 无 moc 缺失或未定义符号。

### UI 显示测试

- AI 服务可用时，截图工具条显示 AI 按钮。
- AI 按钮位置在贴图按钮后、滚动截图/OCR 前。
- AI 服务不可用时，AI 按钮隐藏。
- 第三方调用截图时，AI 按钮隐藏。
- 不显示红点/badge。
- tooltip 正常显示。

### 面板切换测试

- 点击 AI 按钮，右侧二级面板显示 AI 面板。
- AI 面板显示四个功能按钮：Explain、Summary、Translate、Ask AI。
- 切换回矩形、画笔、文本等工具时，AI 面板隐藏。
- 原绘图工具面板和颜色面板恢复正常。
- AI 面板不触发 `ShapesWidget` 初始化。

### 信号测试

- 点击 Explain，发出 `functionSelected(Explain)`。
- 点击 Summary，发出 `functionSelected(Summarize)`。
- 点击 Translate，发出 `functionSelected(Translate)`。
- 点击 Ask AI，发出 `functionSelected(AskAI)`。
- 本次不验证 AI DBus 调用。

### 回归测试

- 普通截图绘制矩形/圆形/箭头/画笔/文本不回归。
- OCR 按钮不回归。
- 贴图按钮不回归。
- 滚动截图按钮不回归。
- Options 菜单不回归。
- 第三方调用隐藏逻辑不回归。

## 后续阶段

本次完成后，后续可以在独立设计中继续接入：

1. `AIAssistantWidget::functionSelected(...)` 到 `MainWindow` 的业务槽函数。
2. AI 专用截图静默保存。
3. 剪贴板复制策略。
4. `AiAssistantInterface` DBus 调用。
5. 调用完成后的状态恢复或退出策略。
6. 可选红点/badge。
7. 可选 `A` 快捷键。
8. 可选滚动截图完成后 AI。
