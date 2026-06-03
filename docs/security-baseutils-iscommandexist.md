# BaseUtils::isCommandExist 安全修改影响分析

## 1. 背景

在安全性编程检查过程中，发现 `src/utils/baseutils.cpp` 中的 `BaseUtils::isCommandExist()` 使用 `QProcess` 启动拼接后的命令字符串来判断外部命令是否存在。

当前实现如下：

```cpp
bool BaseUtils::isCommandExist(QString command)
{
    QProcess *proc = new QProcess;
    if (!proc) {
        return false;
    }
    QString cm = QString("which %1\n").arg(command);
    proc->start(cm);
    proc->waitForFinished(1000);
    int ret = proc->exitCode() == 0;
    delete proc;
    return ret;
}
```

该函数目前仅有一处调用：

```cpp
if (BaseUtils::isCommandExist("dman")) {
    QShortcut *helpSC = new QShortcut(QKeySequence("F1"), this);
    helpSC->setAutoRepeat(false);
    connect(helpSC,  SIGNAL(activated()), this, SLOT(onHelp()));
}
```

调用位置：

```text
src/main_window.cpp:1050
```

该调用用于判断系统中是否存在 `dman` 命令。如果存在，则注册 `F1` 帮助快捷键。

---

## 2. 安全风险

### 2.1 命令拼接风险

当前代码通过字符串拼接构造命令：

```cpp
QString cm = QString("which %1\n").arg(command);
```

如果未来该函数被其他地方复用，并且传入参数来自用户输入、配置文件、DBus 参数或其他不可信来源，则可能产生命令注入风险。

例如，如果传入：

```text
dman; malicious-command
```

拼接后可能形成：

```bash
which dman; malicious-command
```

虽然当前调用处传入的是固定字符串 `"dman"`，实际利用风险较低，但从工具函数设计角度看，该函数不应保留 shell 拼接执行模式。

### 2.2 依赖外部 which 命令

当前实现依赖系统中的 `which` 命令：

```cpp
proc->start(cm);
```

这会带来以下问题：

- 额外创建子进程，增加运行开销；
- 依赖外部命令是否存在；
- 受运行环境 `PATH` 影响；
- 代码行为不如 Qt 原生接口稳定。

### 2.3 不必要的动态分配

当前实现使用：

```cpp
QProcess *proc = new QProcess;
```

该对象只在函数内部临时使用，没有必要动态分配。虽然当前代码在正常路径中会 `delete proc`，但整体实现仍可进一步简化。

---

## 3. 推荐修改方案

推荐使用 Qt 提供的 `QStandardPaths::findExecutable()` 代替外部 `which` 命令。

### 3.1 最小修改方案

在 `src/utils/baseutils.cpp` 中增加头文件：

```cpp
#include <QStandardPaths>
```

将函数修改为：

```cpp
bool BaseUtils::isCommandExist(QString command)
{
    return !QStandardPaths::findExecutable(command).isEmpty();
}
```

该方案只修改 `.cpp` 文件，不修改头文件声明，影响范围最小。

### 3.2 更规范修改方案

如果允许同步调整函数签名，建议将参数改为常量引用，减少不必要的字符串拷贝。

`src/utils/baseutils.h`：

```cpp
static bool isCommandExist(const QString &command);
```

`src/utils/baseutils.cpp`：

```cpp
#include <QStandardPaths>

bool BaseUtils::isCommandExist(const QString &command)
{
    return !QStandardPaths::findExecutable(command).isEmpty();
}
```

---

## 4. 修改影响分析

### 4.1 对现有调用点的影响

当前唯一调用点是：

```cpp
BaseUtils::isCommandExist("dman")
```

功能含义是判断 `dman` 是否存在。

修改前后行为对比如下：

| 场景 | 修改前 | 修改后 | 影响 |
|---|---|---|---|
| 系统存在 `dman` | `which dman` 返回 0，函数返回 `true` | `findExecutable("dman")` 非空，返回 `true` | 无影响 |
| 系统不存在 `dman` | `which dman` 返回非 0，函数返回 `false` | `findExecutable("dman")` 为空，返回 `false` | 无影响 |
| `PATH` 正常 | 能找到命令 | 能找到命令 | 无影响 |
| `PATH` 异常 | 可能找不到命令 | 可能找不到命令 | 行为基本一致 |
| 参数包含 shell 特殊字符 | 存在命令注入风险 | 按普通文件名查找，不执行 shell | 安全性提升 |

因此，对当前功能影响较小。

### 4.2 对 F1 帮助功能的影响

该函数控制是否注册 `F1` 帮助快捷键：

```cpp
if (BaseUtils::isCommandExist("dman")) {
    QShortcut *helpSC = new QShortcut(QKeySequence("F1"), this);
    helpSC->setAutoRepeat(false);
    connect(helpSC,  SIGNAL(activated()), this, SLOT(onHelp()));
}
```

修改后：

- 如果系统中存在 `dman`，仍会注册 `F1` 快捷键；
- 如果系统中不存在 `dman`，仍不会注册 `F1` 快捷键；
- `onHelp()` 的 DBus 调用逻辑不受影响。

因此，该修改不会改变帮助功能的预期行为。

### 4.3 对安全性的影响

修改后不再拼接并执行外部命令，安全收益包括：

- 消除潜在命令注入风险；
- 避免 shell 解释参数；
- 减少对外部 `which` 命令的依赖；
- 减少子进程创建；
- 代码更符合 Qt 项目习惯。

---

## 5. 验证建议

修改后建议进行以下验证：

### 5.1 编译验证

确认新增 `#include <QStandardPaths>` 后项目可以正常编译。

### 5.2 功能验证

在存在 `dman` 的环境中启动应用：

1. 启动 deepin-screen-recorder；
2. 按下 `F1`；
3. 确认可以打开帮助手册。

在不存在 `dman` 的环境中：

1. 启动 deepin-screen-recorder；
2. 确认不会因为找不到 `dman` 导致程序异常；
3. 确认截图、录屏主流程不受影响。

### 5.3 安全验证

可以增加临时测试代码或单元测试验证：

```cpp
BaseUtils::isCommandExist("dman; echo injected")
```

修改后该字符串只会作为普通可执行文件名查找，不会执行 `echo injected`。

---

## 6. 建议提交说明

建议提交信息：

```text
fix: avoid shell-based command lookup in BaseUtils

Use QStandardPaths::findExecutable instead of spawning "which" with a
concatenated command string. This avoids potential command injection risks
and removes unnecessary process creation.
```

中文说明可写为：

```text
修复 BaseUtils::isCommandExist 中通过拼接字符串调用 which 的问题，
改为使用 QStandardPaths::findExecutable 判断命令是否存在，避免潜在
命令注入风险，并减少不必要的外部进程创建。
```

---

## 7. 结论

`BaseUtils::isCommandExist()` 当前只用于判断 `dman` 是否存在，修改为 `QStandardPaths::findExecutable()` 后，对现有功能影响很小。

该修改属于低风险、高收益的安全性改进，建议纳入本次安全性编程修改范围。
