# AI Service Detection Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add V20 AI assistant service detection that reports both availability and the matched DBus service name.

**Architecture:** Keep detection inside existing `DBusUtils`, matching the current V20 utility-class style. Implement `aiAssistantServiceName()` as the single source of truth and derive `isAiAssistantAvailable()` from it, so the later AI invocation step can use the same matched service instead of hard-coding `com.deepin.copilot`.

**Tech Stack:** C++11, Qt 5, QtDBus, qmake, deepin-screen-recorder V20.

---

## File Structure

- Modify: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.h`
  - Responsibility: Declare `isAiAssistantAvailable()` and `aiAssistantServiceName()`.
- Modify: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.cpp`
  - Responsibility: Implement AI service detection via session bus, optional `version()` probe, DBus introspection, and fallback service ordering.
- Spec: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/docs/superpowers/specs/2026-06-02-ai-service-detection-design.md`
  - Responsibility: Documents approved scope and exclusions.

---

### Task 1: Baseline RED Verification

**Files:**
- Inspect: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.h`
- Inspect: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.cpp`

- [ ] **Step 1: Verify current code does not expose matched service name**

Run:

```bash
grep -n "aiAssistantServiceName" /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.h /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.cpp
```

Expected before implementation: no matching output, exit status `1`. If it already exists, inspect the implementation before editing to avoid overwriting user work.

- [ ] **Step 2: Verify unsupported V25 logging dependency status**

Run:

```bash
grep -nE '#include "log.h"|qCDebug\(|dsrApp' /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.cpp
```

Expected before cleanup: matching lines may exist. They are not valid for this V20 tree because `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/log.h` does not exist.

- [ ] **Step 3: Verify V20 log header is absent**

Run:

```bash
test ! -e /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/log.h
```

Expected: command exits with status `0`.

---

### Task 2: Update DBusUtils Header

**Files:**
- Modify: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.h`

- [ ] **Step 1: Ensure QString is included**

The header must include both `QVariant` and `QString`:

```cpp
#ifndef DBUSUTILS_H
#define DBUSUTILS_H
#include <QVariant>
#include <QString>
```

- [ ] **Step 2: Declare AI detection APIs**

The public class declaration must contain:

```cpp
    static QVariant redDBusProperty(const QString &service, const QString &path, const QString &interface = QString(), const char* propert = "");
    static QVariant redDBusMethod(const QString &service, const QString &path, const QString &interface, const char *method);
    static bool isAiAssistantAvailable();
    static QString aiAssistantServiceName();
```

- [ ] **Step 3: Verify header declarations**

Run:

```bash
grep -nE "QString|isAiAssistantAvailable|aiAssistantServiceName" /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.h
```

Expected: output includes `#include <QString>`, `static bool isAiAssistantAvailable();`, and `static QString aiAssistantServiceName();`.

---

### Task 3: Implement AI Service Detection

**Files:**
- Modify: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.cpp`

- [ ] **Step 1: Remove unsupported V25 logging include**

The top include block must be:

```cpp
#include "dbusutils.h"
#include <QDBusConnection>
#include <QDBusReply>
#include <QDBusInterface>
#include <QDebug>
#include <QDBusError>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusConnectionInterface>
```

Do not include:

```cpp
#include "log.h"
```

- [ ] **Step 2: Use V20-compatible debug logging in existing methods**

If constructor/destructor and existing methods contain `qCDebug(dsrApp)`, replace them with `qDebug()` while keeping the same diagnostic text. For example:

```cpp
DBusUtils::DBusUtils()
{
    qDebug() << "DBusUtils constructor called.";
}

DBusUtils::~DBusUtils()
{
    qDebug() << "DBusUtils destructor called.";
}
```

- [ ] **Step 3: Add single-source AI service detection implementation**

Replace any existing `isAiAssistantAvailable()` implementation with:

```cpp
bool DBusUtils::isAiAssistantAvailable()
{
    return !aiAssistantServiceName().isEmpty();
}

QString DBusUtils::aiAssistantServiceName()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qDebug() << "Session bus is not connected, AI assistant is unavailable.";
        return QString();
    }

    const QString copilotService = QStringLiteral("com.deepin.copilot");
    const QString iflytekService = QStringLiteral("com.iflytek.aiassistant");
    const QString objectPath = QStringLiteral("/com/deepin/copilot");
    const QString copilotInterface = QStringLiteral("com.deepin.copilot");

    QDBusInterface copilot(copilotService,
                           objectPath,
                           copilotInterface,
                           bus);
    if (copilot.isValid()) {
        copilot.call(QStringLiteral("version"));
    }

    auto hasAiAssistantMethods = [&](const QString &service) -> bool {
        QDBusInterface introspect(service,
                                  objectPath,
                                  QStringLiteral("org.freedesktop.DBus.Introspectable"),
                                  bus);
        if (!introspect.isValid()) {
            qDebug() << "AI assistant introspection interface is invalid for service:" << service
                     << "error:" << bus.lastError().message();
            return false;
        }

        QDBusReply<QString> xml = introspect.call(QStringLiteral("Introspect"));
        if (!xml.isValid()) {
            qDebug() << "AI assistant introspection failed for service:" << service
                     << "error:" << xml.error().message();
            return false;
        }

        const QString xmlStr = xml.value();
        const bool hasQuickOcr = xmlStr.contains(QStringLiteral("launchAiQuickOCR"));
        const bool hasChatUpload = xmlStr.contains(QStringLiteral("launchChatUploadImage"));
        qDebug() << "AI assistant service:" << service
                 << "has launchAiQuickOCR:" << hasQuickOcr
                 << "has launchChatUploadImage:" << hasChatUpload;
        return hasQuickOcr && hasChatUpload;
    };

    if (hasAiAssistantMethods(copilotService)) {
        return copilotService;
    }

    if (hasAiAssistantMethods(iflytekService)) {
        return iflytekService;
    }

    return QString();
}
```

- [ ] **Step 4: Verify unsupported logging symbols are gone**

Run:

```bash
grep -nE '#include "log.h"|qCDebug\(|dsrApp' /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.cpp
```

Expected after implementation: no matching output, exit status `1`.

- [ ] **Step 5: Verify AI detection symbols are present**

Run:

```bash
grep -nE "aiAssistantServiceName|com.deepin.copilot|com.iflytek.aiassistant|launchAiQuickOCR|launchChatUploadImage" /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.cpp
```

Expected: output includes both service names and both required method names.

---

### Task 4: Build Verification

**Files:**
- Verify: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.h`
- Verify: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/utils/dbusutils.cpp`

- [ ] **Step 1: Run qmake from a temporary build directory**

Run:

```bash
cd /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder \
  && rm -rf /tmp/dsr-ai-service-detection-build \
  && mkdir -p /tmp/dsr-ai-service-detection-build \
  && cd /tmp/dsr-ai-service-detection-build \
  && qmake /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro
```

Expected: qmake completes successfully and generates a Makefile. If using Qt Creator, the equivalent is `Build -> Run qmake`.

- [ ] **Step 2: Compile with Qt Creator or command line**

Preferred user workflow:

```text
Qt Creator -> Build -> Rebuild Project
```

Command-line fallback:

```bash
cd /tmp/dsr-ai-service-detection-build && make -j$(nproc)
```

Expected: no compile errors from `dbusutils.cpp`, `dbusutils.h`, or `aiassistantinterface.cpp`. If unrelated legacy project files fail, record the first error and identify whether it is related to AI service detection.

- [ ] **Step 3: Inspect diff**

Run:

```bash
cd /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder && git diff -- src/utils/dbusutils.h src/utils/dbusutils.cpp src/dbusinterface/aiassistantinterface.cpp src/src.pro docs/superpowers/specs/2026-06-02-ai-service-detection-design.md docs/superpowers/plans/2026-06-02-ai-service-detection.md
```

Expected: diff only includes AI DBus interface build entries, V20-compatible AI interface debug logging, AI service detection APIs, and spec/plan docs.

---

## Self-Review

- Spec coverage: The plan adds `isAiAssistantAvailable()`, adds `aiAssistantServiceName()`, checks both services in priority order, requires both AI DBus methods, removes unsupported V25 logging dependency, and excludes UI/call-chain work.
- Placeholder scan: No unresolved placeholders are present.
- Type consistency: `aiAssistantServiceName()` is consistently declared and defined as `static QString`; `isAiAssistantAvailable()` returns `bool`; service names and method names match the spec.
