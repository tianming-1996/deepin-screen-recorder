# V20 截屏 AI 面板移植设计

日期：2026-06-02

## 背景

V25 截屏 AI 助手下放 V20 的第 5 步需要移植/适配 AI 功能面板。前置步骤已经完成 AI DBus 接口、AI 服务检测、AI 图标资源和 AI 翻译字符串准备。本步骤只新增独立可复用的 `AIAssistantWidget`，不接入 V20 工具栏、二级面板或主窗口调用链路。

参考文档：

- `/home/ut006498@uos/Desktop/ai-assistant-porting-guide.md`
- `/home/ut006498@uos/Desktop/V25截屏AI助手下放V20_方案一完整实现顺序.md`
- `/home/ut006498@uos/Desktop/V25截屏AI助手下放V20_方案ABC修改大纲需求技术文档.md`

## 目标

1. 在 V20 新增 AI 面板控件类；
2. 保留 V25 面板的 4 个 AI 功能按钮；
3. 保留功能枚举和 `functionSelected` 信号；
4. 使用 V20 已存在的 `ToolButton`、`DBlurEffectWidget`、图标资源和翻译；
5. 加入 `src/src.pro` 构建并通过 qmake/编译验证。

## 修改范围

### 新增文件

- `src/widgets/aiassistantwidget.h`
- `src/widgets/aiassistantwidget.cpp`

### 修改文件

- `src/src.pro`

## Widget 设计

新增类：

```cpp
class AIAssistantWidget : public DWidget
```

保留枚举：

```cpp
enum AIFunction {
    Explain,
    Summarize,
    Translate,
    AskAI
};
```

保留信号：

```cpp
void functionSelected(AIFunction function);
void requestClose();
```

按钮：

| 按钮 | 文案 | 图标 theme name | 枚举 |
|---|---|---|---|
| 解释 | `Explain` | `explain` | `Explain` |
| 总结 | `Summary` | `summary` | `Summarize` |
| 翻译 | `Translate` | `translate` | `Translate` |
| 问问 AI | `Ask AI` | `askai` | `AskAI` |

## V20 适配点

从 V25 移植时需要去掉 V25 专属日志依赖：

```cpp
#include "../utils/log.h"
qCWarning(dsrApp) << ...
```

改成 V20 可编译日志：

```cpp
qDebug() << "AI function selected:" << function;
```

Qt 信号连接使用 V20/Qt 5.11 更常见的：

```cpp
connect(buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
        this, &AIAssistantWidget::onToolButtonClicked);
```

避免使用较新 Qt 才有的 `idClicked`。

## 布局策略

本步骤保持 V25 的横向布局：

```text
Explain | Summary | Translate | Ask AI
```

原因：

1. 第 5 步目标是新增可复用 AI 面板控件；
2. 第 7 步才决定如何接入 V20 二级面板/浮动面板；
3. 先保持 V25 行为，减少面板类自身移植风险。

## 构建接入

在 `src/src.pro` 中增加：

```qmake
widgets/aiassistantwidget.h
widgets/aiassistantwidget.cpp
```

## 验证方式

1. `src/widgets/aiassistantwidget.h` 和 `.cpp` 存在；
2. `src/src.pro` 包含新增 header/source；
3. 源码不包含 `../utils/log.h`、`qCWarning(dsrApp)`、`qCDebug(dsrApp)`；
4. 源码包含 4 个按钮文案和 4 个图标 theme name；
5. qmake 成功；
6. Qt Creator 编译时不报 `AIAssistantWidget` 相关错误。

## 明确不做

本步骤不做：

- 不在 `SubToolWidget` 增加 AI 按钮；
- 不在 `SideBarWidget` 展示 AI 面板；
- 不新增 MainWindow AI 功能入口；
- 不调用 AI DBus；
- 不实现截图静默保存；
- 不实现红点或快捷键。
