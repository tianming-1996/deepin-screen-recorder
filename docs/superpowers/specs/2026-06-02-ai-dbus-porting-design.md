# V20 截屏 AI DBus 接口移植设计

日期：2026-06-02

## 背景

根据以下文档，本次只完成 V25 截屏 AI 助手下放 V20 的第一步基础移植：AI DBus 接口。

- `/home/ut006498@uos/Desktop/ai-assistant-porting-guide.md`
- `/home/ut006498@uos/Desktop/V25截屏AI助手下放V20_方案一完整实现顺序.md`
- `/home/ut006498@uos/Desktop/V25截屏AI助手下放V20_方案ABC修改大纲需求技术文档.md`

## 目标

在 V20 工程中新增 AI 助手 DBus 代理接口，并接入 qmake 构建，使后续服务检测、AI 按钮、AI 面板、静默保存和实际 DBus 调用可以复用该接口。

## 修改范围

### 新增文件

在 V20 工程新增：

- `src/dbusinterface/aiassistantinterface.h`
- `src/dbusinterface/aiassistantinterface.cpp`

参考 V25 文件：

- `/data/home/ut006498@uos/code1/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.h`
- `/data/home/ut006498@uos/code1/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.cpp`

### 修改文件

- `src/src.pro`

将新增头文件加入 `HEADERS`，将新增源文件加入 `SOURCES`，确保接口文件参与 V20 构建。

## 接口设计

新增 `AiAssistantInterface`，继承 `QDBusAbstractInterface`，保持与 V20 现有 `OcrInterface`、`PinScreenShotsInterface` 相同的手写 DBus 代理风格。

静态接口名：

```cpp
static inline const char *staticInterfaceName()
{
    return "com.deepin.copilot";
}
```

本步骤提供以下方法封装：

```cpp
QDBusPendingReply<> launchAiQuickOCR(int type,
                                    const QString &query,
                                    const QPoint &pos,
                                    bool isCustom,
                                    const QString &imagePath);

QDBusPendingReply<> launchChatUploadImage(const QString &imagePath);
```

可以同步保留 V25 中已有的 `launchAiQuickOCRWithImage(int, const QImage &, const QString &)` 作为兼容封装，但本步骤不接入调用链。

## 数据流

本步骤只建立调用封装，不产生实际业务数据流。后续步骤的数据流将是：

```text
MainWindow / AI Controller
  -> AiAssistantInterface
  -> session bus
  -> com.deepin.copilot 或后续检测命中的 AI 服务
```

当前步骤只保证 `AiAssistantInterface` 这一层存在并可编译。

## 错误处理

本步骤不主动发起 DBus 调用，因此不处理运行时调用失败。接口方法返回 `QDBusPendingReply<>`，后续调用方负责：

- 校验 DBus interface 是否有效；
- 监听 `QDBusPendingCallWatcher`；
- 处理调用失败和状态恢复。

## 明确不做

本步骤不包含以下内容：

- 不新增 `DBusUtils::isAiAssistantAvailable()`；
- 不实现 `com.deepin.copilot` / `com.iflytek.aiassistant` fallback 检测；
- 不增加 AI 按钮；
- 不增加 AI 面板；
- 不修改 `MainWindow` AI 业务入口；
- 不实现截图静默保存；
- 不实际调用 AI DBus 方法。

## 验证方式

优先验证：

1. 新增文件存在于 V20 `src/dbusinterface/` 下；
2. `src/src.pro` 中包含新增头文件和源文件；
3. V20 工程 qmake/编译阶段能够识别并编译 `AiAssistantInterface`。

如果本地环境缺少依赖导致全量构建失败，需要如实记录失败原因；但接口文件和工程文件接入仍应保持正确。
