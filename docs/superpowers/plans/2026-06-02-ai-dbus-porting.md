# AI DBus Interface Porting Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add the V25 AI assistant DBus proxy interface to the V20 deepin-screen-recorder project and include it in the qmake build.

**Architecture:** Follow the existing V20 DBus proxy pattern used by `OcrInterface` and `PinScreenShotsInterface`: a small hand-written `QDBusAbstractInterface` subclass under `src/dbusinterface/`. The interface only wraps AI DBus method calls; service availability detection and business invocation remain out of scope for this first step.

**Tech Stack:** C++11, Qt 5, QtDBus, qmake (`src/src.pro`), deepin-screen-recorder V20 source tree.

---

## File Structure

- Create: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.h`
  - Responsibility: Declare `AiAssistantInterface`, its static DBus interface name, constructor/destructor, and inline DBus method wrappers.
- Create: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.cpp`
  - Responsibility: Define constructor/destructor and keep runtime behavior minimal.
- Modify: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro`
  - Responsibility: Add the new header/source to `HEADERS` and `SOURCES` so qmake compiles the interface.
- Already written spec: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/docs/superpowers/specs/2026-06-02-ai-dbus-porting-design.md`
  - Responsibility: Documents the approved scope and exclusions.

---

### Task 1: Baseline Verification

**Files:**
- Inspect: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/`
- Inspect: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro`

- [ ] **Step 1: Verify AI interface files are not already present**

Run:

```bash
test ! -e /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.h \
  && test ! -e /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.cpp
```

Expected: command exits with status `0`. If it exits non-zero, inspect the existing files before overwriting because they may contain user work.

- [ ] **Step 2: Verify current project file does not already reference the interface**

Run:

```bash
grep -n "aiassistantinterface" /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro
```

Expected: no matching lines and exit status `1`. If matching lines exist, avoid duplicate `HEADERS`/`SOURCES` entries.

---

### Task 2: Add AiAssistantInterface Header

**Files:**
- Create: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.h`

- [ ] **Step 1: Write the header file**

Create `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.h` with exactly this content:

```cpp
// Copyright (C) 2020 ~ 2021 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AIASSISTANTINTERFACE_H
#define AIASSISTANTINTERFACE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>
#include <QImage>
#include <QBuffer>
#include <QDebug>

class AiAssistantInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    {
        return "com.deepin.copilot";
    }

public:
    /*
    * @param: serviceName QDBusConnection 注册的服务名字
    * @param: ObjectPath QDBusConnection 注册的对象路径
    */
    AiAssistantInterface(const QString &serviceName, const QString &ObjectPath,
                 const QDBusConnection &connection, QObject *parent = nullptr);
    QDBusConnection dbus = QDBusConnection::sessionBus();
    ~AiAssistantInterface();

public Q_SLOTS: // METHODS
    /*
    * @brief: launchAiQuickOCR 启动AI快速OCR功能
    * @param: type 类型 (1=解释, 2=总结, 3=翻译, 4=问问AI)
    * @param: query 查询文本内容
    * @param: pos 位置坐标 QPoint(x, y)
    * @param: isCustom 是否自定义
    * @param: imagePath 图片路径
    * @return: QDBusPendingReply
    * @note: 正确的签名是 'is(ii)bs' (int, string, (int,int), bool, string)
    */
    inline QDBusPendingReply<> launchAiQuickOCR(int type, const QString &query,
                                                const QPoint &pos,
                                                bool isCustom, const QString &imagePath)
    {
        qDebug() << __FUNCTION__ << "type:" << type << "query:" << query
                 << "pos:" << pos << "isCustom:" << isCustom << "imagePath:" << imagePath;
        return call(QStringLiteral("launchAiQuickOCR"), type, query,
                   pos, isCustom, imagePath);
    }

    /*
    * @brief: launchAiQuickOCRWithImage 使用图片数据启动AI快速OCR功能
    * @param: type 类型 (1=解释, 2=总结, 3=翻译, 4=问问AI)
    * @param: image 图片数据
    * @param: imageName 图片名称
    * @return: QDBusPendingReply
    * @note: 使用图片数据而不是文件路径，最后一个参数传递编码后的图片数据
    */
    inline QDBusPendingReply<> launchAiQuickOCRWithImage(int type, const QImage &image, const QString &imageName)
    {
        qDebug() << __FUNCTION__ << "type:" << type << "imageName:" << imageName;
        QByteArray data;
        QBuffer buf(&data);
        if (image.save(&buf, "PNG")) {
            data = qCompress(data, 9);
            data = data.toBase64();
        }

        QPoint pos(0, 0);  // 默认位置

        // 使用编码后的图片数据作为最后一个字符串参数
        return call(QStringLiteral("launchAiQuickOCR"), type, QString(""),
                   pos, false, QString::fromUtf8(data));
    }

    /*
    * @brief: launchChatUploadImage 直接上传图片到聊天面板
    * @param: imagePath 图片路径
    * @return: QDBusPendingReply
    */
    inline QDBusPendingReply<> launchChatUploadImage(const QString &imagePath)
    {
        qDebug() << __FUNCTION__ << "imagePath:" << imagePath;
        return call(QStringLiteral("launchChatUploadImage"), imagePath);
    }

Q_SIGNALS: // SIGNALS
};

namespace com {
namespace iflytek {
typedef ::AiAssistantInterface AiAssistant;
}
}
#endif // AIASSISTANTINTERFACE_H
```

- [ ] **Step 2: Verify the header contains both required DBus methods**

Run:

```bash
grep -nE "launchAiQuickOCR|launchChatUploadImage" /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.h
```

Expected: output includes `launchAiQuickOCR` and `launchChatUploadImage` declarations/wrappers.

---

### Task 3: Add AiAssistantInterface Source

**Files:**
- Create: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.cpp`

- [ ] **Step 1: Write the source file**

Create `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.cpp` with exactly this content:

```cpp
// Copyright (C) 2020 ~ 2021 Deepin Technology Co., Ltd.
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aiassistantinterface.h"

AiAssistantInterface::AiAssistantInterface(const QString &serviceName, const QString &ObjectPath,
                           const QDBusConnection &connection, QObject *parent)
    : QDBusAbstractInterface(serviceName, ObjectPath, staticInterfaceName(), connection, parent)
{
    qDebug() << "AiAssistantInterface created for service:" << serviceName << "path:" << ObjectPath;
}

AiAssistantInterface::~AiAssistantInterface()
{
    qDebug() << "Destroying AiAssistantInterface";
}
```

- [ ] **Step 2: Verify the source references the new header**

Run:

```bash
grep -n "#include \"aiassistantinterface.h\"" /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.cpp
```

Expected: one matching include line.

---

### Task 4: Add Interface to qmake Build

**Files:**
- Modify: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro`

- [ ] **Step 1: Add the header to `HEADERS`**

In `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro`, find this existing block:

```qmake
    recordertablet.h \
    dbusinterface/ocrinterface.h \
    dbusinterface/pinscreenshotsinterface.h \
    gstrecord/gstrecordx.h \
```

Change it to:

```qmake
    recordertablet.h \
    dbusinterface/ocrinterface.h \
    dbusinterface/pinscreenshotsinterface.h \
    dbusinterface/aiassistantinterface.h \
    gstrecord/gstrecordx.h \
```

- [ ] **Step 2: Add the source to `SOURCES`**

In the same file, find this existing block:

```qmake
    recordertablet.cpp \
    dbusinterface/ocrinterface.cpp \
    dbusinterface/pinscreenshotsinterface.cpp \
    gstrecord/gstrecordx.cpp \
```

Change it to:

```qmake
    recordertablet.cpp \
    dbusinterface/ocrinterface.cpp \
    dbusinterface/pinscreenshotsinterface.cpp \
    dbusinterface/aiassistantinterface.cpp \
    gstrecord/gstrecordx.cpp \
```

- [ ] **Step 3: Verify qmake entries exist exactly once**

Run:

```bash
grep -n "dbusinterface/aiassistantinterface" /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro
```

Expected output contains exactly two lines:

```text
...:    dbusinterface/aiassistantinterface.h \
...:    dbusinterface/aiassistantinterface.cpp \
```

---

### Task 5: Build-Level Verification

**Files:**
- Verify: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.h`
- Verify: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/dbusinterface/aiassistantinterface.cpp`
- Verify: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro`

- [ ] **Step 1: Check the working tree diff**

Run:

```bash
cd /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder && git diff -- src/src.pro src/dbusinterface/aiassistantinterface.h src/dbusinterface/aiassistantinterface.cpp docs/superpowers/specs/2026-06-02-ai-dbus-porting-design.md docs/superpowers/plans/2026-06-02-ai-dbus-porting.md
```

Expected: diff only includes the spec, this plan, two new AI DBus files, and two `src.pro` entries.

- [ ] **Step 2: Run qmake from a temporary build directory**

Run:

```bash
cd /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder \
  && rm -rf /tmp/dsr-ai-dbus-build \
  && mkdir -p /tmp/dsr-ai-dbus-build \
  && cd /tmp/dsr-ai-dbus-build \
  && qmake /data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro
```

Expected: qmake completes successfully and generates a Makefile. If it fails because system dependencies or qmake features are unavailable, record the exact error.

- [ ] **Step 3: Compile the new interface object if qmake succeeded**

Run:

```bash
cd /tmp/dsr-ai-dbus-build && make -j$(nproc) dbusinterface/aiassistantinterface.o
```

Expected: `aiassistantinterface.o` compiles successfully. If the project Makefile does not expose that target, run:

```bash
cd /tmp/dsr-ai-dbus-build && make -j$(nproc)
```

Expected: build reaches or passes compilation of `aiassistantinterface.cpp`. If unrelated legacy project files fail later, record the first error and identify whether it is related to `AiAssistantInterface`.

---

## Self-Review

- Spec coverage: The plan creates `aiassistantinterface.h`, creates `aiassistantinterface.cpp`, updates `src/src.pro`, and verifies build integration. It does not include DBus service detection, AI UI, static save, or actual AI invocation, matching the spec exclusions.
- Placeholder scan: No `TBD`, `TODO`, or unresolved placeholders are present.
- Type consistency: The class name is consistently `AiAssistantInterface`; required methods are consistently `launchAiQuickOCR` and `launchChatUploadImage`; paths consistently target the V20 project.
