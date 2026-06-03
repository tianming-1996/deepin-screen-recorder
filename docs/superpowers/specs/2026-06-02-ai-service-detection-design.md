# V20 截屏 AI 服务检测设计

日期：2026-06-02

## 背景

第一步已经在 V20 工程中新增 AI DBus 代理接口 `AiAssistantInterface`。第二步需要在 V20 `DBusUtils` 中补齐 AI 服务可用性检测，供后续 AI 按钮显示和 AI DBus 调用链路复用。

参考文档：

- `/home/ut006498@uos/Desktop/ai-assistant-porting-guide.md`
- `/home/ut006498@uos/Desktop/V25截屏AI助手下放V20_方案一完整实现顺序.md`
- `/home/ut006498@uos/Desktop/V25截屏AI助手下放V20_方案ABC修改大纲需求技术文档.md`

## 目标

在 V20 工程中新增/修正 AI 服务检测能力：

1. 检测 `com.deepin.copilot` 是否提供 AI 截图所需方法；
2. 若不满足，fallback 检测 `com.iflytek.aiassistant`；
3. 同时检测 `launchAiQuickOCR` 和 `launchChatUploadImage`；
4. 对外提供 bool 可用性接口；
5. 对外提供实际命中的 serviceName，供后续 DBus 调用避免检测/调用不一致。

## 修改范围

### 修改文件

- `src/utils/dbusutils.h`
- `src/utils/dbusutils.cpp`

### 不修改文件

本步骤不修改：

- `src/widgets/subtoolwidget.*`
- `src/widgets/sidebar.*`
- `src/main_window.*`
- `src/dbusinterface/aiassistantinterface.*`

## 接口设计

在 `DBusUtils` 中新增：

```cpp
static bool isAiAssistantAvailable();
static QString aiAssistantServiceName();
```

设计关系：

```cpp
bool DBusUtils::isAiAssistantAvailable()
{
    return !aiAssistantServiceName().isEmpty();
}
```

`aiAssistantServiceName()` 返回：

| 检测结果 | 返回值 |
|---|---|
| `com.deepin.copilot` 可用 | `com.deepin.copilot` |
| copilot 不可用，`com.iflytek.aiassistant` 可用 | `com.iflytek.aiassistant` |
| 都不可用 | 空 `QString()` |

## 检测逻辑

1. 获取 session bus；
2. 如果 session bus 未连接，返回空字符串；
3. 优先对 `com.deepin.copilot` 调用 `version()` 进行探测/唤醒；
4. 对候选服务执行 `org.freedesktop.DBus.Introspectable.Introspect`；
5. 检查返回 XML 是否同时包含：
   - `launchAiQuickOCR`
   - `launchChatUploadImage`
6. copilot 满足则返回 copilot；
7. 否则检测 `com.iflytek.aiassistant`；
8. fallback 满足则返回 iflytek；
9. 都不满足则返回空字符串。

## 日志风格

V20 主程序通过 `Dtk::Core::DLogManager::registerConsoleAppender()` 和 `registerFileAppender()` 接管 Qt 标准日志，不存在 V25 的 `src/utils/log.h` / `dsrApp` 日志分类。

因此本步骤使用 V20 可编译的日志方式：

```cpp
qDebug() << "...";
```

不引入：

```cpp
#include "log.h"
qCDebug(dsrApp) << "...";
```

## 错误处理

| 场景 | 处理 |
|---|---|
| session bus 未连接 | 返回空 serviceName / false |
| 候选服务 interface 无效 | 该服务判定不可用，继续 fallback |
| Introspect 调用失败 | 该服务判定不可用，继续 fallback |
| XML 缺少任一关键方法 | 该服务判定不可用，继续 fallback |
| 两个服务都不可用 | `aiAssistantServiceName()` 返回空；`isAiAssistantAvailable()` 返回 false |

## 验证方式

1. `dbusutils.h` 声明 `isAiAssistantAvailable()` 和 `aiAssistantServiceName()`；
2. `dbusutils.cpp` 不引用 V20 不存在的 `log.h`、`dsrApp`；
3. `dbusutils.cpp` 中包含两个候选服务名；
4. `dbusutils.cpp` 中同时检查 `launchAiQuickOCR` 和 `launchChatUploadImage`；
5. Qt Creator 重新 Run qmake 并编译通过。

## 明确不做

本步骤不做：

- 不显示 AI 按钮；
- 不接入工具栏；
- 不接入 AI 面板；
- 不创建 `AiAssistantInterface` 实例；
- 不实际调用 `launchAiQuickOCR` 或 `launchChatUploadImage`；
- 不做红点、快捷键、静默保存。
