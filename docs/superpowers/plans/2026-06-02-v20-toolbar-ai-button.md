# V20 Toolbar AI Button Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a V20 screenshot toolbar AI button that opens a right-side AI panel with Explain, Summary, Translate, and Ask AI actions, without connecting screenshot saving or AI DBus invocation.

**Architecture:** Add a standalone `AIAssistantWidget` and route the new `"aiassistant"` tool type through the existing `SubToolWidget -> MainWindow -> SideBar -> SideBarWidget` chain. Keep AI UI separate from drawing tool state so `ShotToolWidget`, `ColorToolWidget`, and `ShapesWidget` are not used for the AI panel.

**Tech Stack:** C++14, Qt 5 Widgets, DTK Widgets (`DWidget`, `DFloatingWidget`, `DBlurEffectWidget`, `DFontSizeManager`), qmake/CMake build metadata, GoogleTest/QtTest unit tests.

---

## Scope and Guardrails

- Source root: `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder`
- Approved spec: `docs/superpowers/specs/2026-06-02-v20-toolbar-ai-button-design.md`
- Do not implement AI screenshot saving, clipboard copying, `launchAiQuickOCR`, `launchChatUploadImage`, red dot/badge, `A` shortcut, or scrolling screenshot AI.
- The worktree already contains uncommitted AI-related files and translation/resource changes. Stage only the exact files listed in each commit step.

## File Structure

### New files

- `src/widgets/aiassistantwidget.h` — standalone AI panel public API and `AIFunction` enum.
- `src/widgets/aiassistantwidget.cpp` — AI panel layout, four buttons, and `functionSelected(...)` emission.
- `tests/ut_screen_shot_recorder/widgets/ut_aiassistantwidget.h` — focused tests for the AI panel signal contract and button text.

### Modified files

- `src/widgets/subtoolwidget.h` — adds `m_aiAssistantButton` member.
- `src/widgets/subtoolwidget.cpp` — creates AI toolbar button after pin button; hides it when service/third-party conditions disallow it; emits `changeShotToolFunc("aiassistant")`.
- `src/widgets/sidebar.h` — exposes AI panel storage and current panel sizing hooks.
- `src/widgets/sidebar.cpp` — displays `AIAssistantWidget` for `"aiassistant"`; hides drawing panel widgets in AI mode; restores normal drawing panel for other tools.
- `src/main_window.cpp` — adds special branch for `"aiassistant"` so AI does not initialize or update drawing shapes.
- `src/utils/dbusutils.h` — declares AI service availability helper.
- `src/utils/dbusutils.cpp` — checks session bus and AI service introspection methods for button visibility.
- `src/src.pro` — includes `aiassistantwidget` files and DBus interface files in qmake builds.
- `src/CMakeLists.txt` — includes `aiassistantwidget` files in CMake builds.
- `assets/icons/icons.qrc` — registers AI SVG resources.
- `tests/ut_screen_shot_recorder/test_all_interfaces.h` — includes AI widget tests.
- `tests/ut_screen_shot_recorder/ut_screen_shot_recorder.pro` — compiles AI widget tests and source.
- `tests/ut_screen_shot_recorder/widgets/ut_subtoolwidget.h` — tests toolbar button creation and signal emission.
- `tests/ut_screen_shot_recorder/widgets/ut_sidebarwidget.h` — tests AI sidebar switching.
- `tests/ut_screen_shot_recorder/ut_main_window.h` — tests `"aiassistant"` does not create a drawing `ShapesWidget`.

---

### Task 1: Add AIAssistantWidget Contract Tests

**Files:**
- Create: `tests/ut_screen_shot_recorder/widgets/ut_aiassistantwidget.h`
- Modify: `tests/ut_screen_shot_recorder/test_all_interfaces.h`
- Modify: `tests/ut_screen_shot_recorder/ut_screen_shot_recorder.pro`

- [ ] **Step 1: Write the failing AI widget test header**

Create `tests/ut_screen_shot_recorder/widgets/ut_aiassistantwidget.h` with this content:

```cpp
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <gtest/gtest.h>
#include <QSignalSpy>
#include <QStringList>

#define private public
#include "../../src/widgets/aiassistantwidget.h"
#undef private

class AIAssistantWidgetTest : public testing::Test
{
public:
    AIAssistantWidget *m_widget = nullptr;

    void SetUp() override
    {
        m_widget = new AIAssistantWidget();
    }

    void TearDown() override
    {
        delete m_widget;
        m_widget = nullptr;
    }
};

TEST_F(AIAssistantWidgetTest, buttonsHaveExpectedText)
{
    ASSERT_NE(nullptr, m_widget->m_explainButton);
    ASSERT_NE(nullptr, m_widget->m_summarizeButton);
    ASSERT_NE(nullptr, m_widget->m_translateButton);
    ASSERT_NE(nullptr, m_widget->m_askAIButton);

    EXPECT_EQ(QStringLiteral("Explain"), m_widget->m_explainButton->text());
    EXPECT_EQ(QStringLiteral("Summary"), m_widget->m_summarizeButton->text());
    EXPECT_EQ(QStringLiteral("Translate"), m_widget->m_translateButton->text());
    EXPECT_EQ(QStringLiteral("Ask AI"), m_widget->m_askAIButton->text());
}

TEST_F(AIAssistantWidgetTest, emitsSelectedFunctionForEachButton)
{
    QSignalSpy spy(m_widget, SIGNAL(functionSelected(AIAssistantWidget::AIFunction)));
    ASSERT_TRUE(spy.isValid());

    m_widget->m_explainButton->click();
    ASSERT_EQ(1, spy.count());
    EXPECT_EQ(AIAssistantWidget::Explain, spy.takeFirst().at(0).value<AIAssistantWidget::AIFunction>());

    m_widget->m_summarizeButton->click();
    ASSERT_EQ(1, spy.count());
    EXPECT_EQ(AIAssistantWidget::Summarize, spy.takeFirst().at(0).value<AIAssistantWidget::AIFunction>());

    m_widget->m_translateButton->click();
    ASSERT_EQ(1, spy.count());
    EXPECT_EQ(AIAssistantWidget::Translate, spy.takeFirst().at(0).value<AIAssistantWidget::AIFunction>());

    m_widget->m_askAIButton->click();
    ASSERT_EQ(1, spy.count());
    EXPECT_EQ(AIAssistantWidget::AskAI, spy.takeFirst().at(0).value<AIAssistantWidget::AIFunction>());
}

TEST_F(AIAssistantWidgetTest, sizeHintIsWideEnoughForHorizontalPanel)
{
    const QSize hint = m_widget->sizeHint();
    EXPECT_GE(hint.width(), 240);
    EXPECT_EQ(68, hint.height());
}
```

- [ ] **Step 2: Register the test header**

In `tests/ut_screen_shot_recorder/test_all_interfaces.h`, add the include after the sidebar includes:

```cpp
#include "widgets/ut_sidebar.h"
#include "widgets/ut_sidebarwidget.h"
#include "widgets/ut_aiassistantwidget.h"
#include "widgets/ut_scrollshottip.h"
```

- [ ] **Step 3: Add AI widget test/source entries to the qmake test project**

In `tests/ut_screen_shot_recorder/ut_screen_shot_recorder.pro`, add these entries to `HEADERS +=` near the other widget test headers:

```qmake
           widgets/ut_sidebar.h \
           widgets/ut_sidebarwidget.h \
           widgets/ut_aiassistantwidget.h \
```

Add the source header and source file entries near other widget source entries:

```qmake
        ../../src/widgets/sidebar.h \
        ../../src/widgets/aiassistantwidget.h \
        ../../src/widgets/toolbar.h \
```

```qmake
    ../../src/widgets/sidebar.cpp \
    ../../src/widgets/aiassistantwidget.cpp \
    ../../src/widgets/toolbar.cpp \
```

- [ ] **Step 4: Run the focused AI widget tests and verify failure**

Run from `/data/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder`:

```bash
qmake tests/ut_screen_shot_recorder/ut_screen_shot_recorder.pro -o build-ut-aiwidget/Makefile
make -C build-ut-aiwidget -j$(nproc)
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='AIAssistantWidgetTest.*'
```

Expected before implementation is one of these failures:

```text
fatal error: ../../src/widgets/aiassistantwidget.h: No such file or directory
```

or:

```text
Expected equality of these values ...
```

- [ ] **Step 5: Commit the test-only changes**

```bash
git add tests/ut_screen_shot_recorder/widgets/ut_aiassistantwidget.h \
        tests/ut_screen_shot_recorder/test_all_interfaces.h \
        tests/ut_screen_shot_recorder/ut_screen_shot_recorder.pro
git commit -m "test: add AI assistant widget coverage"
```

---

### Task 2: Implement Standalone AIAssistantWidget

**Files:**
- Create: `src/widgets/aiassistantwidget.h`
- Create: `src/widgets/aiassistantwidget.cpp`

- [ ] **Step 1: Add the AI panel header**

Create `src/widgets/aiassistantwidget.h` with this content:

```cpp
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AIASSISTANTWIDGET_H
#define AIASSISTANTWIDGET_H

#include <DWidget>
#include <DBlurEffectWidget>

#include "toolbutton.h"

DWIDGET_USE_NAMESPACE

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

    explicit AIAssistantWidget(QWidget *parent = nullptr);
    QSize sizeHint() const override;

signals:
    void functionSelected(AIAssistantWidget::AIFunction function);

private slots:
    void onToolButtonClicked(int functionId);

private:
    ToolButton *m_explainButton = nullptr;
    ToolButton *m_summarizeButton = nullptr;
    ToolButton *m_translateButton = nullptr;
    ToolButton *m_askAIButton = nullptr;
    DBlurEffectWidget *m_blurArea = nullptr;
};

Q_DECLARE_METATYPE(AIAssistantWidget::AIFunction)

#endif // AIASSISTANTWIDGET_H
```

- [ ] **Step 2: Add the AI panel implementation**

Create `src/widgets/aiassistantwidget.cpp` with this content:

```cpp
// SPDX-FileCopyrightText: 2022 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "aiassistantwidget.h"

#include <DFontSizeManager>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QIcon>
#include <QSizePolicy>

DWIDGET_USE_NAMESPACE

namespace {
const QSize kIconSize(24, 24);
const int kPanelHeight = 68;
const int kExtraWidth = 20;
}

AIAssistantWidget::AIAssistantWidget(QWidget *parent)
    : DWidget(parent)
{
    qRegisterMetaType<AIAssistantWidget::AIFunction>("AIAssistantWidget::AIFunction");

    m_blurArea = new DBlurEffectWidget(this);
    m_blurArea->setBlurRectXRadius(7);
    m_blurArea->setBlurRectYRadius(7);
    m_blurArea->setRadius(15);
    m_blurArea->setMode(DBlurEffectWidget::GaussianBlur);
    m_blurArea->setBlurEnabled(true);
    m_blurArea->setBlendMode(DBlurEffectWidget::InWidgetBlend);
    m_blurArea->setMaskColor(QColor(255, 255, 255, 0));

    QHBoxLayout *buttonLayout = new QHBoxLayout(m_blurArea);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(0);

    QButtonGroup *buttonGroup = new QButtonGroup(this);
    buttonGroup->setExclusive(false);

    m_explainButton = new ToolButton(this);
    m_explainButton->setText(tr("Explain"));
    m_explainButton->setIcon(QIcon::fromTheme("explain"));
    m_explainButton->setIconSize(kIconSize);
    m_explainButton->setCheckable(false);
    m_explainButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_summarizeButton = new ToolButton(this);
    m_summarizeButton->setText(tr("Summary"));
    m_summarizeButton->setIcon(QIcon::fromTheme("summary"));
    m_summarizeButton->setIconSize(kIconSize);
    m_summarizeButton->setCheckable(false);
    m_summarizeButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_translateButton = new ToolButton(this);
    m_translateButton->setText(tr("Translate"));
    m_translateButton->setIcon(QIcon::fromTheme("translate"));
    m_translateButton->setIconSize(kIconSize);
    m_translateButton->setCheckable(false);
    m_translateButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_askAIButton = new ToolButton(this);
    m_askAIButton->setText(tr("Ask AI"));
    m_askAIButton->setIcon(QIcon::fromTheme("askai"));
    m_askAIButton->setIconSize(kIconSize);
    m_askAIButton->setCheckable(false);
    m_askAIButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    DFontSizeManager::instance()->bind(m_explainButton, DFontSizeManager::T6);
    DFontSizeManager::instance()->bind(m_summarizeButton, DFontSizeManager::T6);
    DFontSizeManager::instance()->bind(m_translateButton, DFontSizeManager::T6);
    DFontSizeManager::instance()->bind(m_askAIButton, DFontSizeManager::T6);

    buttonGroup->addButton(m_explainButton, Explain);
    buttonGroup->addButton(m_summarizeButton, Summarize);
    buttonGroup->addButton(m_translateButton, Translate);
    buttonGroup->addButton(m_askAIButton, AskAI);

    buttonLayout->addWidget(m_explainButton);
    buttonLayout->addWidget(m_summarizeButton);
    buttonLayout->addWidget(m_translateButton);
    buttonLayout->addWidget(m_askAIButton);

    connect(buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &AIAssistantWidget::onToolButtonClicked);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_blurArea);
    setLayout(mainLayout);
}

QSize AIAssistantWidget::sizeHint() const
{
    const QSize blurAreaSize = m_blurArea->sizeHint();
    return QSize(blurAreaSize.width() + kExtraWidth, kPanelHeight);
}

void AIAssistantWidget::onToolButtonClicked(int functionId)
{
    emit functionSelected(static_cast<AIAssistantWidget::AIFunction>(functionId));
}
```

- [ ] **Step 3: Run focused AI widget tests and verify pass**

```bash
make -C build-ut-aiwidget -j$(nproc)
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='AIAssistantWidgetTest.*'
```

Expected:

```text
[  PASSED  ] 3 tests.
```

- [ ] **Step 4: Commit the AI widget implementation**

```bash
git add src/widgets/aiassistantwidget.h src/widgets/aiassistantwidget.cpp
git commit -m "feat: add AI assistant panel widget"
```

---

### Task 3: Register AI Resources and Build Metadata

**Files:**
- Modify: `assets/icons/icons.qrc`
- Modify: `src/src.pro`
- Modify: `src/CMakeLists.txt`

- [ ] **Step 1: Register AI icon resources**

In `assets/icons/icons.qrc`, ensure these files are listed inside `<qresource prefix="/icons/deepin/builtin">` after the existing screenshot/OCR icon entries:

```xml
        <file>texts/ai_assistant_32px.svg</file>
        <file>texts/askai_32px.svg</file>
        <file>texts/explain_32px.svg</file>
        <file>texts/summary_32px.svg</file>
        <file>texts/translate_32px.svg</file>
```

Do not add badge resources for this feature. The approved spec excludes red dot and badge.

- [ ] **Step 2: Register AI widget in qmake build**

In `src/src.pro`, ensure `widgets/aiassistantwidget.h` appears in `HEADERS +=` near the other widget headers:

```qmake
    widgets/sidebar.h \
    widgets/shottoolwidget.h \
    widgets/aiassistantwidget.h \
    widgets/colortoolwidget.h \
```

Ensure `widgets/aiassistantwidget.cpp` appears in `SOURCES +=` near the other widget sources:

```qmake
    widgets/sidebar.cpp \
    widgets/shottoolwidget.cpp \
    widgets/aiassistantwidget.cpp \
    widgets/colortoolwidget.cpp \
```

- [ ] **Step 3: Register AI widget in CMake build**

In `src/CMakeLists.txt`, add `widgets/aiassistantwidget.h` to `deepin-screen-recorder_HDRS` near `widgets/shottoolwidget.h`:

```cmake
    widgets/sidebar.h
    widgets/shottoolwidget.h
    widgets/aiassistantwidget.h
    widgets/colortoolwidget.h
```

Add `widgets/aiassistantwidget.cpp` to `deepin-screen-recorder_SRCS` near `widgets/shottoolwidget.cpp`:

```cmake
    widgets/sidebar.cpp
    widgets/shottoolwidget.cpp
    widgets/aiassistantwidget.cpp
    widgets/colortoolwidget.cpp
```

- [ ] **Step 4: Run build metadata verification**

```bash
qmake src/src.pro -o build-src-metadata/Makefile
make -C build-src-metadata -j$(nproc)
cmake -S src -B build-src-cmake-metadata
cmake --build build-src-cmake-metadata -j$(nproc)
```

Expected:

```text
Built target deepin-screen-recorder
```

and no errors mentioning `aiassistantwidget`, `ai_assistant`, `explain`, `summary`, `translate`, or `askai`.

- [ ] **Step 5: Commit resource and build metadata changes**

```bash
git add assets/icons/icons.qrc src/src.pro src/CMakeLists.txt
git commit -m "build: register AI assistant panel resources"
```

---

### Task 4: Add AI Service Detection for Button Visibility

**Files:**
- Modify: `src/utils/dbusutils.h`
- Modify: `src/utils/dbusutils.cpp`

- [ ] **Step 1: Add DBusUtils declarations**

In `src/utils/dbusutils.h`, declare these public helpers:

```cpp
class DBusUtils
{
public:
    DBusUtils();
    static QVariant redDBusProperty(const QString &service, const QString &path, const QString &interface = QString(), const char* propert = "");
    static QVariant redDBusMethod(const QString &service, const QString &path, const QString &interface, const char *method);
    static bool isAiAssistantAvailable();
    static QString aiAssistantServiceName();
};
```

- [ ] **Step 2: Add service detection implementation**

In `src/utils/dbusutils.cpp`, append this implementation after `redDBusMethod(...)`:

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

- [ ] **Step 3: Compile after service detection**

```bash
make -C build-src-metadata -j$(nproc)
```

Expected:

```text
No compiler errors in src/utils/dbusutils.cpp or src/utils/dbusutils.h
```

- [ ] **Step 4: Commit service detection**

```bash
git add src/utils/dbusutils.h src/utils/dbusutils.cpp
git commit -m "feat: detect AI assistant service availability"
```

---

### Task 5: Add AI Button to V20 Screenshot Toolbar

**Files:**
- Modify: `tests/ut_screen_shot_recorder/widgets/ut_subtoolwidget.h`
- Modify: `src/widgets/subtoolwidget.h`
- Modify: `src/widgets/subtoolwidget.cpp`

- [ ] **Step 1: Add toolbar button tests**

In `tests/ut_screen_shot_recorder/widgets/ut_subtoolwidget.h`, add this include near the existing source includes:

```cpp
#include "../../src/utils/dbusutils.h"
```

Add these stub functions near the other free stub functions:

```cpp
bool isAiAssistantAvailable_true_stub()
{
    return true;
}

bool isAiAssistantAvailable_false_stub()
{
    return false;
}
```

Add these tests after `shapeClickedFromWidget`:

```cpp
TEST_F(SubToolWidgetTest, initShotLabelCreatesAiAssistantButtonWhenServiceAvailable)
{
    delete m_subToolWidget;
    m_subToolWidget = nullptr;

    stub.set(ADDR(DBusUtils, isAiAssistantAvailable), isAiAssistantAvailable_true_stub);
    m_subToolWidget = new SubToolWidget(m_mainWindow);
    stub.reset(ADDR(DBusUtils, isAiAssistantAvailable));

    ASSERT_NE(nullptr, m_subToolWidget->m_aiAssistantButton);
    EXPECT_FALSE(m_subToolWidget->m_aiAssistantButton->isHidden());
    EXPECT_EQ(QSize(35, 35), m_subToolWidget->m_aiAssistantButton->iconSize());
}

TEST_F(SubToolWidgetTest, initShotLabelHidesAiAssistantButtonWhenServiceUnavailable)
{
    delete m_subToolWidget;
    m_subToolWidget = nullptr;

    stub.set(ADDR(DBusUtils, isAiAssistantAvailable), isAiAssistantAvailable_false_stub);
    m_subToolWidget = new SubToolWidget(m_mainWindow);
    stub.reset(ADDR(DBusUtils, isAiAssistantAvailable));

    ASSERT_NE(nullptr, m_subToolWidget->m_aiAssistantButton);
    EXPECT_TRUE(m_subToolWidget->m_aiAssistantButton->isHidden());
}

TEST_F(SubToolWidgetTest, aiAssistantButtonEmitsAiAssistantToolFunction)
{
    delete m_subToolWidget;
    m_subToolWidget = nullptr;

    stub.set(ADDR(DBusUtils, isAiAssistantAvailable), isAiAssistantAvailable_true_stub);
    m_subToolWidget = new SubToolWidget(m_mainWindow);
    stub.reset(ADDR(DBusUtils, isAiAssistantAvailable));

    QSignalSpy spy(m_subToolWidget, SIGNAL(changeShotToolFunc(QString)));
    ASSERT_TRUE(spy.isValid());

    m_subToolWidget->m_aiAssistantButton->click();

    ASSERT_EQ(1, spy.count());
    EXPECT_EQ(QStringLiteral("aiassistant"), spy.takeFirst().at(0).toString());
}
```

- [ ] **Step 2: Run toolbar tests and verify failure**

```bash
make -C build-ut-aiwidget -j$(nproc)
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='SubToolWidgetTest.*AiAssistant*'
```

Expected failure before implementation:

```text
class SubToolWidget has no member named m_aiAssistantButton
```

- [ ] **Step 3: Add AI button member**

In `src/widgets/subtoolwidget.h`, add this private member after `m_pinButton`:

```cpp
    /**
     * @brief 截图功能中 AI 助手按钮
     */
    ToolButton *m_aiAssistantButton = nullptr;
```

The surrounding block becomes:

```cpp
    /**
     * @brief 贴图工具栏按钮
     */
    ToolButton *m_pinButton = nullptr;
    /**
     * @brief 截图功能中 AI 助手按钮
     */
    ToolButton *m_aiAssistantButton = nullptr;
    /**
     * @brief 截图功能中矩形工具按钮
     */
    ToolButton *m_rectButton = nullptr;
```

- [ ] **Step 4: Include DBusUtils in SubToolWidget implementation**

In `src/widgets/subtoolwidget.cpp`, add this include with the other utils includes:

```cpp
#include "../utils/dbusutils.h"
```

- [ ] **Step 5: Create the AI button after the pin button**

In `SubToolWidget::initShotLabel()`, immediately after:

```cpp
    installTipHint(m_pinButton, tr("Pin Screenshots"));
    btnList.append(m_pinButton);
```

add:

```cpp
    // 添加 AI 助手按钮
    m_aiAssistantButton = new ToolButton();
    m_aiAssistantButton->setIconSize(QSize(35, 35));
    m_aiAssistantButton->setIcon(QIcon::fromTheme("ai_assistant"));
    Utils::setAccessibility(m_aiAssistantButton, "aiAssistantButton");
    m_shotBtnGroup->addButton(m_aiAssistantButton);
    m_aiAssistantButton->setFixedSize(MIN_TOOL_BUTTON_SIZE);
    installTipHint(m_aiAssistantButton, tr("AI Screenshot"));
    btnList.append(m_aiAssistantButton);
    connect(m_aiAssistantButton, &DPushButton::clicked, this, [ = ] {
        emit changeShotToolFunc("aiassistant");
    });
    if (!DBusUtils::isAiAssistantAvailable()) {
        m_aiAssistantButton->hide();
    }
```

- [ ] **Step 6: Hide AI button for third-party screenshot calls**

In the existing `if (Utils::is3rdInterfaceStart)` block inside `initShotLabel()`, change it to include the AI button:

```cpp
    if (Utils::is3rdInterfaceStart) {
        m_shotOptionButton->hide();
        m_scrollShotButton->hide(); //隐藏滚动截图按钮
        m_ocrButton->hide(); //隐藏ocr按钮
        m_pinButton->hide(); //隐藏pin按钮
        m_aiAssistantButton->hide(); //隐藏AI助手按钮
    }
```

- [ ] **Step 7: Run toolbar tests and verify pass**

```bash
make -C build-ut-aiwidget -j$(nproc)
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='SubToolWidgetTest.*AiAssistant*'
```

Expected:

```text
[  PASSED  ] 3 tests.
```

- [ ] **Step 8: Commit toolbar button changes**

```bash
git add src/widgets/subtoolwidget.h src/widgets/subtoolwidget.cpp \
        tests/ut_screen_shot_recorder/widgets/ut_subtoolwidget.h
git commit -m "feat: add AI assistant toolbar button"
```

---

### Task 6: Show AI Panel in SideBarWidget

**Files:**
- Modify: `tests/ut_screen_shot_recorder/widgets/ut_sidebarwidget.h`
- Modify: `src/widgets/sidebar.h`
- Modify: `src/widgets/sidebar.cpp`

- [ ] **Step 1: Add sidebar AI panel tests**

In `tests/ut_screen_shot_recorder/widgets/ut_sidebarwidget.h`, change the include block so private members are visible to the test:

```cpp
#define private public
#include "../../src/main_window.h"
#include "../../src/widgets/sidebar.h"
#undef private
```

Replace the existing `changeShotToolWidget` test with:

```cpp
TEST_F(SideBarWidgetTest, changeShotToolWidget)
{
    stub.set(ADDR(QVariant,toInt),toInt_stub2);
    SideBarWidget();
    stub.reset(ADDR(QVariant,toInt));
    m_sideBarWidget->changeShotToolWidget(QString("rectangle"));
    m_sideBarWidget->changeShotToolWidget(QString("oval"));
    m_sideBarWidget->changeShotToolWidget(QString("arrow"));
    m_sideBarWidget->changeShotToolWidget(QString("line"));
    m_sideBarWidget->changeShotToolWidget(QString("text"));

    EXPECT_FALSE(m_sideBarWidget->m_shotTool->isHidden());
    EXPECT_FALSE(m_sideBarWidget->m_colorTool->isHidden());
    EXPECT_TRUE(m_sideBarWidget->m_aiAssistantWidget->isHidden());
}

TEST_F(SideBarWidgetTest, changeShotToolWidgetShowsAiAssistantPanel)
{
    m_sideBarWidget->changeShotToolWidget(QStringLiteral("aiassistant"));

    ASSERT_NE(nullptr, m_sideBarWidget->m_aiAssistantWidget);
    EXPECT_TRUE(m_sideBarWidget->m_shotTool->isHidden());
    EXPECT_TRUE(m_sideBarWidget->m_colorTool->isHidden());
    EXPECT_FALSE(m_sideBarWidget->m_aiAssistantWidget->isHidden());
    EXPECT_GE(m_sideBarWidget->width(), m_sideBarWidget->m_aiAssistantWidget->sizeHint().width());
}

TEST_F(SideBarWidgetTest, changeShotToolWidgetRestoresDrawingPanelAfterAiAssistant)
{
    m_sideBarWidget->changeShotToolWidget(QStringLiteral("aiassistant"));
    m_sideBarWidget->changeShotToolWidget(QStringLiteral("rectangle"));

    EXPECT_FALSE(m_sideBarWidget->m_shotTool->isHidden());
    EXPECT_FALSE(m_sideBarWidget->m_colorTool->isHidden());
    EXPECT_TRUE(m_sideBarWidget->m_aiAssistantWidget->isHidden());
}
```

- [ ] **Step 2: Run sidebar tests and verify failure**

```bash
make -C build-ut-aiwidget -j$(nproc)
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='SideBarWidgetTest.changeShotToolWidget*'
```

Expected failure before implementation:

```text
class SideBarWidget has no member named m_aiAssistantWidget
```

- [ ] **Step 3: Add AI widget include and member to sidebar header**

In `src/widgets/sidebar.h`, add the AI widget include:

```cpp
#include "aiassistantwidget.h"
```

Add a size helper declaration to `SideBarWidget` public methods:

```cpp
    QSize currentPanelSize() const;
```

Add this private member after `m_shotTool`:

```cpp
    AIAssistantWidget *m_aiAssistantWidget;
```

The relevant private block becomes:

```cpp
private:
    DLabel *m_hSeparatorLine;
    ColorToolWidget *m_colorTool;
    ShotToolWidget *m_shotTool;
    AIAssistantWidget *m_aiAssistantWidget;
    DImageButton *m_closeButton;
    bool  m_expanded;
```

- [ ] **Step 4: Initialize and hide the AI panel in SideBarWidget constructor**

In `src/widgets/sidebar.cpp`, change the constructor member initialization and widget creation:

```cpp
SideBarWidget::SideBarWidget(DWidget *parent)
    : DFloatingWidget(parent)
    ,m_expanded(false)
{
```

Inside the constructor after `m_shotTool = new ShotToolWidget(this);`, add:

```cpp
    m_aiAssistantWidget = new AIAssistantWidget(this);
    m_aiAssistantWidget->hide();
```

Change the layout setup from:

```cpp
    VLayout->addWidget(m_shotTool, 0,  Qt::AlignCenter);
    VLayout->addWidget(m_colorTool, 1,   Qt::AlignCenter);
    VLayout->addWidget(m_closeButton, 2, Qt::AlignCenter);
```

to:

```cpp
    VLayout->addWidget(m_shotTool, 0,  Qt::AlignCenter);
    VLayout->addWidget(m_colorTool, 1,   Qt::AlignCenter);
    VLayout->addWidget(m_aiAssistantWidget, 0, Qt::AlignCenter);
    VLayout->addWidget(m_closeButton, 2, Qt::AlignCenter);
```

- [ ] **Step 5: Add panel size helper and AI switching behavior**

In `src/widgets/sidebar.cpp`, add this method after the destructor:

```cpp
QSize SideBarWidget::currentPanelSize() const
{
    if (m_aiAssistantWidget && !m_aiAssistantWidget->isHidden()) {
        return QSize(qMax(TOOLBAR_WIDGET_SIZE.width(), m_aiAssistantWidget->sizeHint().width()), TOOLBAR_WIDGET_SIZE.height());
    }

    return TOOLBAR_WIDGET_SIZE;
}
```

Replace `SideBarWidget::changeShotToolWidget` with:

```cpp
void SideBarWidget::changeShotToolWidget(const QString &func)
{
    if (func == QStringLiteral("aiassistant")) {
        m_shotTool->hide();
        m_colorTool->hide();
        m_aiAssistantWidget->show();
        setFixedSize(currentPanelSize());
        return;
    }

    m_aiAssistantWidget->hide();
    m_shotTool->show();
    m_colorTool->show();
    setFixedSize(TOOLBAR_WIDGET_SIZE);

    m_shotTool->switchContent(func);
    m_colorTool->setFunction(func);
}
```

- [ ] **Step 6: Update SideBar size after function switch**

In `SideBar::changeShotToolFunc`, replace:

```cpp
void SideBar::changeShotToolFunc(const QString &func)
{
    m_sidebarWidget->changeShotToolWidget(func);
}
```

with:

```cpp
void SideBar::changeShotToolFunc(const QString &func)
{
    m_sidebarWidget->changeShotToolWidget(func);
    const QSize panelSize = m_sidebarWidget->currentPanelSize();
    setFixedSize(panelSize.width(), TOOLBAR_HEIGHT);
}
```

- [ ] **Step 7: Run sidebar tests and verify pass**

```bash
make -C build-ut-aiwidget -j$(nproc)
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='SideBarWidgetTest.changeShotToolWidget*'
```

Expected:

```text
[  PASSED  ] 3 tests.
```

- [ ] **Step 8: Commit sidebar panel changes**

```bash
git add src/widgets/sidebar.h src/widgets/sidebar.cpp \
        tests/ut_screen_shot_recorder/widgets/ut_sidebarwidget.h
git commit -m "feat: show AI assistant panel in sidebar"
```

---

### Task 7: Route aiassistant Without Drawing Shape Initialization

**Files:**
- Modify: `tests/ut_screen_shot_recorder/ut_main_window.h`
- Modify: `src/main_window.cpp`

- [ ] **Step 1: Add MainWindow AI routing test**

In `tests/ut_screen_shot_recorder/ut_main_window.h`, add this private field accessor near the existing `m_toolBar` accessor:

```cpp
ACCESS_PRIVATE_FIELD(MainWindow, SideBar *, m_sideBar);
```

Add this stub function near the other `MainWindow` stubs:

```cpp
void updateSideBarPos_stub()
{
}
```

Add this test after `onViewShortcut`:

```cpp
TEST_F(MainWindowTest, changeShotToolEventAiAssistantDoesNotCreateShapeWidget)
{
    stub.set(ADDR(MainWindow, initMainWindow), initMainWindow_stub);
    stub.set(ADDR(MainWindow, initAttributes), initAttributes_stub);
    stub.set(ADDR(MainWindow, initLaunchMode), initLaunchMode_stub);
    stub.set(ADDR(MainWindow, showFullScreen), showFullScreen_stub);
    stub.set(ADDR(MainWindow, initResource), initResource_stub);
    stub.set(ADDR(MainWindow, updateSideBarPos), updateSideBarPos_stub);

    MainWindow *window = new MainWindow();
    access_private_field::MainWindowm_sideBar(*window) = new SideBar(window);
    access_private_field::MainWindowm_sideBar(*window)->initSideBar();
    access_private_field::MainWindowm_isShapesWidgetExist(*window) = false;
    access_private_field::MainWindowm_shapesWidget(*window) = nullptr;

    window->changeShotToolEvent(QStringLiteral("aiassistant"));

    EXPECT_FALSE(access_private_field::MainWindowm_isShapesWidgetExist(*window));
    EXPECT_EQ(nullptr, access_private_field::MainWindowm_shapesWidget(*window));

    stub.reset(ADDR(MainWindow, initMainWindow));
    stub.reset(ADDR(MainWindow, initAttributes));
    stub.reset(ADDR(MainWindow, initLaunchMode));
    stub.reset(ADDR(MainWindow, showFullScreen));
    stub.reset(ADDR(MainWindow, initResource));
    stub.reset(ADDR(MainWindow, updateSideBarPos));
    delete window;
}
```

- [ ] **Step 2: Run MainWindow AI routing test and verify failure**

```bash
make -C build-ut-aiwidget -j$(nproc)
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='MainWindowTest.changeShotToolEventAiAssistantDoesNotCreateShapeWidget'
```

Expected failure before implementation:

```text
Value of: access_private_field::MainWindowm_isShapesWidgetExist(*window)
  Actual: true
Expected: false
```

- [ ] **Step 3: Add aiassistant special branch**

In `src/main_window.cpp`, update `MainWindow::changeShotToolEvent(const QString &func)` by inserting this branch after the `scrollShot` branch and before the final drawing-tool `else` block:

```cpp
    } else if (func == "aiassistant") {
        if (!m_sideBar->isVisible()) {
            updateSideBarPos();
        }
        m_sideBar->changeShotToolFunc(func);
        updateSideBarPos();
```

The relevant section becomes:

```cpp
    } else if (func == "scrollShot") { //点击滚动截图
        //捕捉区域的固件不显示
        drawDragPoint = false;
        repaint();
        //延时100ms防止预览款将捕捉区域的骨架截取到图片中
        QTimer::singleShot(100, this, [ = ] {
            //初始化滚动截图
            initScrollShot();
        });

    } else if (func == "aiassistant") {
        if (!m_sideBar->isVisible()) {
            updateSideBarPos();
        }
        m_sideBar->changeShotToolFunc(func);
        updateSideBarPos();

    } else {
        if (!m_sideBar->isVisible()) {
            updateSideBarPos();
        }
```

- [ ] **Step 4: Run MainWindow AI routing test and verify pass**

```bash
make -C build-ut-aiwidget -j$(nproc)
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='MainWindowTest.changeShotToolEventAiAssistantDoesNotCreateShapeWidget'
```

Expected:

```text
[  PASSED  ] 1 test.
```

- [ ] **Step 5: Commit MainWindow routing changes**

```bash
git add src/main_window.cpp tests/ut_screen_shot_recorder/ut_main_window.h
git commit -m "fix: route AI assistant without drawing shape state"
```

---

### Task 8: Full Verification and Manual UI Check

**Files:**
- No source changes expected.

- [ ] **Step 1: Run focused unit tests**

```bash
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder --gtest_filter='AIAssistantWidgetTest.*:SubToolWidgetTest.*AiAssistant*:SideBarWidgetTest.changeShotToolWidget*:MainWindowTest.changeShotToolEventAiAssistantDoesNotCreateShapeWidget'
```

Expected:

```text
[  PASSED  ] 10 tests.
```

- [ ] **Step 2: Run full unit test binary**

```bash
QT_QPA_PLATFORM=offscreen ./build-ut-aiwidget/ut_screen_shot_recorder
```

Expected:

```text
[  PASSED  ]
```

No failures in existing screenshot, sidebar, toolbar, OCR, pin screenshot, or drawing tests.

- [ ] **Step 3: Run qmake application build**

```bash
qmake screen_shot_recorder.pro -o build-app-qmake/Makefile
make -C build-app-qmake -j$(nproc)
```

Expected:

```text
deepin-screen-recorder
```

binary is produced under `build-app-qmake` without compiler or linker errors.

- [ ] **Step 4: Run CMake application build**

```bash
cmake -S . -B build-app-cmake
cmake --build build-app-cmake -j$(nproc)
```

Expected:

```text
Built target deepin-screen-recorder
```

- [ ] **Step 5: Manual UI verification with AI service available**

Run the built app in a V20 desktop session with `com.deepin.copilot` or `com.iflytek.aiassistant` available:

```bash
./build-app-cmake/src/deepin-screen-recorder
```

Verify:

```text
1. Start normal screenshot mode.
2. AI button appears after the pin button and before scrollshot/OCR.
3. AI button has no red dot or badge.
4. Clicking AI button opens the right-side AI panel.
5. AI panel shows Explain, Summary, Translate, Ask AI.
6. Clicking each AI function does not save a screenshot, does not call AI, and does not exit the screenshot tool.
7. Switching to Rectangle, Pencil, Text hides the AI panel and restores drawing sidebar controls.
```

- [ ] **Step 6: Manual UI verification with AI service unavailable**

Stop or mask the AI service in a test session, then run the app again:

```bash
./build-app-cmake/src/deepin-screen-recorder
```

Verify:

```text
1. Start normal screenshot mode.
2. AI button is hidden.
3. Pin, scrollshot, OCR, drawing tools, and Options keep their original behavior.
```

- [ ] **Step 7: Review staged diff before final commit**

```bash
git status --short
git diff --stat
git diff -- src/widgets/aiassistantwidget.h \
            src/widgets/aiassistantwidget.cpp \
            src/widgets/subtoolwidget.h \
            src/widgets/subtoolwidget.cpp \
            src/widgets/sidebar.h \
            src/widgets/sidebar.cpp \
            src/main_window.cpp \
            src/utils/dbusutils.h \
            src/utils/dbusutils.cpp \
            src/src.pro \
            src/CMakeLists.txt \
            assets/icons/icons.qrc \
            tests/ut_screen_shot_recorder/widgets/ut_aiassistantwidget.h \
            tests/ut_screen_shot_recorder/widgets/ut_subtoolwidget.h \
            tests/ut_screen_shot_recorder/widgets/ut_sidebarwidget.h \
            tests/ut_screen_shot_recorder/ut_main_window.h \
            tests/ut_screen_shot_recorder/test_all_interfaces.h \
            tests/ut_screen_shot_recorder/ut_screen_shot_recorder.pro
```

Expected:

```text
Only AI button, AI panel, AI service detection, resources/build metadata, and related tests are changed.
No screenshot saving, clipboard, launchAiQuickOCR, launchChatUploadImage, red dot, badge, or A shortcut implementation appears in the diff.
```

- [ ] **Step 8: Commit verification-only updates**

If Step 7 shows only intended files and there are remaining test/build metadata adjustments, commit them:

```bash
git add src/widgets/aiassistantwidget.h \
        src/widgets/aiassistantwidget.cpp \
        src/widgets/subtoolwidget.h \
        src/widgets/subtoolwidget.cpp \
        src/widgets/sidebar.h \
        src/widgets/sidebar.cpp \
        src/main_window.cpp \
        src/utils/dbusutils.h \
        src/utils/dbusutils.cpp \
        src/src.pro \
        src/CMakeLists.txt \
        assets/icons/icons.qrc \
        tests/ut_screen_shot_recorder/widgets/ut_aiassistantwidget.h \
        tests/ut_screen_shot_recorder/widgets/ut_subtoolwidget.h \
        tests/ut_screen_shot_recorder/widgets/ut_sidebarwidget.h \
        tests/ut_screen_shot_recorder/ut_main_window.h \
        tests/ut_screen_shot_recorder/test_all_interfaces.h \
        tests/ut_screen_shot_recorder/ut_screen_shot_recorder.pro
git commit -m "test: verify V20 AI assistant toolbar panel"
```

---

## Self-Review

### Spec coverage

- AI button in V20 toolbar: Task 5.
- Button location after pin and before scrollshot/OCR: Task 5 Step 5.
- Hide button when AI service is unavailable: Task 4 and Task 5.
- Hide button for third-party screenshot calls: Task 5 Step 6.
- Right-side panel using independent `AIAssistantWidget`: Task 2 and Task 6.
- Four AI buttons and `functionSelected(...)`: Task 1 and Task 2.
- Avoid drawing shape state for AI: Task 7.
- Build/resource registration: Task 3.
- Tests and manual verification: Task 8.
- Excluded red dot, badge, shortcut, save, clipboard, DBus AI call, scrolling screenshot AI: Scope guardrails and Task 8 diff check.

### Placeholder scan

This plan contains no placeholder markers, open-ended implementation slots, or unspecified edge handling. All code-changing steps include exact code or exact replacement blocks.

### Type consistency

- `AIAssistantWidget::AIFunction` is defined in Task 2 and used consistently in Task 1.
- `m_aiAssistantButton` is defined in Task 5 and used only in `SubToolWidget` tests and implementation.
- `m_aiAssistantWidget` and `currentPanelSize()` are defined in Task 6 and used only by `SideBarWidget`/`SideBar` tests and implementation.
- `DBusUtils::isAiAssistantAvailable()` and `DBusUtils::aiAssistantServiceName()` are declared and implemented in Task 4, then used in Task 5.
