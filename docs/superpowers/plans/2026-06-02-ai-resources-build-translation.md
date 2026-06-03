# AI Resources Build Translation Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Copy V25 AI assistant icons into V20, register them in `icons.qrc`, and sync AI panel translation strings for the V20 translation build set.

**Architecture:** Treat this as a resource-preparation step only. Source SVG files and translation contexts come from the V25 tree; V20 receives files under the same `assets/icons/texts/` resource namespace, qrc entries under the existing `/icons/deepin/builtin` resource, and `AIAssistantWidget` contexts only in `.ts` files listed by `translations.pri`.

**Tech Stack:** Qt qrc, Qt Linguist `.ts` XML, qmake, Python 3 for deterministic file updates.

---

## File Structure

- Create/copy 7 SVG files under `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/texts/`.
- Modify `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/icons.qrc` to register those SVG files.
- Modify `.ts` files listed by `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/translations.pri`, copying the `AIAssistantWidget` context from the matching V25 translation file.
- Verify `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro` already references `../assets/icons/icons.qrc`.

---

### Task 1: RED Baseline Checks

**Files:**
- Inspect: V20 `assets/icons/texts/`
- Inspect: V20 `assets/icons/icons.qrc`
- Inspect: V20 `translations.pri`

- [ ] **Step 1: Verify the AI SVG files are not all present yet**

Run:

```bash
python3 - <<'PY'
from pathlib import Path
root = Path('/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/texts')
files = ['ai_assistant_32px.svg','ai_assistant_unused_32px.svg','ai_badge_32px.svg','askai_32px.svg','explain_32px.svg','summary_32px.svg','translate_32px.svg']
missing = [f for f in files if not (root / f).exists()]
print('\n'.join(missing))
raise SystemExit(0 if missing else 1)
PY
```

Expected before implementation: prints one or more missing file names.

- [ ] **Step 2: Verify qrc entries are not all present yet**

Run:

```bash
python3 - <<'PY'
from pathlib import Path
qrc = Path('/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/icons.qrc').read_text()
entries = ['texts/ai_assistant_32px.svg','texts/ai_assistant_unused_32px.svg','texts/ai_badge_32px.svg','texts/askai_32px.svg','texts/explain_32px.svg','texts/summary_32px.svg','texts/translate_32px.svg']
missing = [e for e in entries if e not in qrc]
print('\n'.join(missing))
raise SystemExit(0 if missing else 1)
PY
```

Expected before implementation: prints one or more missing qrc entries.

- [ ] **Step 3: Verify V20 zh_CN lacks AIAssistantWidget context**

Run:

```bash
grep -n "<name>AIAssistantWidget</name>" /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/translations/deepin-screen-recorder_zh_CN.ts
```

Expected before implementation: no matching output, exit status `1`.

---

### Task 2: Copy Icons and Update qrc

**Files:**
- Copy from: `/data/home/ut006498@uos/code1/deepin-screen-recorder/assets/icons/texts/*.svg`
- Copy to: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/texts/*.svg`
- Modify: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/icons.qrc`

- [ ] **Step 1: Run deterministic icon/qrc update script**

Run:

```bash
python3 - <<'PY'
from pathlib import Path
import shutil

v25 = Path('/data/home/ut006498@uos/code1/deepin-screen-recorder/assets/icons/texts')
v20 = Path('/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/texts')
qrc_path = Path('/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/icons.qrc')
files = [
    'ai_assistant_32px.svg',
    'ai_assistant_unused_32px.svg',
    'askai_32px.svg',
    'explain_32px.svg',
    'summary_32px.svg',
    'translate_32px.svg',
    'ai_badge_32px.svg',
]
for name in files:
    src = v25 / name
    dst = v20 / name
    if not src.exists():
        raise FileNotFoundError(src)
    shutil.copyfile(src, dst)

text = qrc_path.read_text()
insert_lines = [f'        <file>texts/{name}</file>' for name in files]
missing = [line for line in insert_lines if line not in text]
if missing:
    marker = '    </qresource>'
    replacement = '\n'.join(missing) + '\n' + marker
    text = text.replace(marker, replacement, 1)
    qrc_path.write_text(text)
PY
```

- [ ] **Step 2: Verify copied icon files exist**

Run:

```bash
python3 - <<'PY'
from pathlib import Path
root = Path('/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/texts')
files = ['ai_assistant_32px.svg','ai_assistant_unused_32px.svg','ai_badge_32px.svg','askai_32px.svg','explain_32px.svg','summary_32px.svg','translate_32px.svg']
missing = [f for f in files if not (root / f).is_file()]
if missing:
    print('missing:', missing)
    raise SystemExit(1)
print('all ai icon files present')
PY
```

Expected: `all ai icon files present`.

- [ ] **Step 3: Verify qrc entries exist exactly once**

Run:

```bash
python3 - <<'PY'
from pathlib import Path
qrc = Path('/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/assets/icons/icons.qrc').read_text()
entries = ['texts/ai_assistant_32px.svg','texts/ai_assistant_unused_32px.svg','texts/ai_badge_32px.svg','texts/askai_32px.svg','texts/explain_32px.svg','texts/summary_32px.svg','texts/translate_32px.svg']
for entry in entries:
    count = qrc.count(entry)
    print(entry, count)
    if count != 1:
        raise SystemExit(1)
PY
```

Expected: each entry count is `1`.

---

### Task 3: Sync AI Translation Contexts

**Files:**
- Read: `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/translations.pri`
- Read from V25 matching `.ts` files under `/data/home/ut006498@uos/code1/deepin-screen-recorder/translations/`
- Modify matching V20 `.ts` files under `/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/translations/`

- [ ] **Step 1: Run deterministic translation sync script**

Run:

```bash
python3 - <<'PY'
from pathlib import Path
import re

v20_root = Path('/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder')
v25_trans = Path('/data/home/ut006498@uos/code1/deepin-screen-recorder/translations')
pri = v20_root / 'translations.pri'

pri_text = pri.read_text()
rel_paths = re.findall(r'\$\$PWD/(translations/deepin-screen-recorder_[^\s\\]+\.ts)', pri_text)
if not rel_paths:
    raise RuntimeError('No deepin-screen-recorder translation files found in translations.pri')

def extract_context(text):
    m = re.search(r'<context>\s*\n\s*<name>AIAssistantWidget</name>.*?</context>', text, re.S)
    if not m:
        return None
    return m.group(0)

fallback = '''<context>
    <name>AIAssistantWidget</name>
    <message>
        <source>Explain</source>
        <translation>Explain</translation>
    </message>
    <message>
        <source>Translate</source>
        <translation>Translate</translation>
    </message>
    <message>
        <source>Ask AI</source>
        <translation>Ask AI</translation>
    </message>
    <message>
        <source>Summary</source>
        <translation>Summary</translation>
    </message>
</context>'''

updated = []
for rel in rel_paths:
    target = v20_root / rel
    source = v25_trans / Path(rel).name
    if not target.exists():
        raise FileNotFoundError(target)
    target_text = target.read_text()
    if '<name>AIAssistantWidget</name>' in target_text:
        continue
    context = extract_context(source.read_text()) if source.exists() else fallback
    if context is None:
        context = fallback
    insert_at = target_text.find('<context>')
    if insert_at == -1:
        raise RuntimeError(f'No <context> found in {target}')
    target_text = target_text[:insert_at] + context + '\n' + target_text[insert_at:]
    target.write_text(target_text)
    updated.append(str(target))
print('\n'.join(updated))
PY
```

- [ ] **Step 2: Verify every translations.pri-listed file has AI context and strings**

Run:

```bash
python3 - <<'PY'
from pathlib import Path
import re
v20_root = Path('/home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder')
pri_text = (v20_root / 'translations.pri').read_text()
rel_paths = re.findall(r'\$\$PWD/(translations/deepin-screen-recorder_[^\s\\]+\.ts)', pri_text)
required = ['<name>AIAssistantWidget</name>', '<source>Explain</source>', '<source>Summary</source>', '<source>Translate</source>', '<source>Ask AI</source>']
failed = []
for rel in rel_paths:
    text = (v20_root / rel).read_text()
    missing = [item for item in required if item not in text]
    if missing:
        failed.append((rel, missing))
if failed:
    for rel, missing in failed:
        print(rel, missing)
    raise SystemExit(1)
print(f'all {len(rel_paths)} translation files contain AI context')
PY
```

Expected: reports all listed files contain AI context.

---

### Task 4: Build-Level Verification

**Files:**
- Verify: V20 `assets/icons/icons.qrc`
- Verify: V20 `src/src.pro`
- Verify: V20 translation XML files

- [ ] **Step 1: Verify src.pro already includes icons.qrc**

Run:

```bash
grep -n "../assets/icons/icons.qrc" /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro
```

Expected: one matching `RESOURCES` entry.

- [ ] **Step 2: Run qmake**

Run:

```bash
cd /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder \
  && rm -rf /tmp/dsr-ai-resources-build \
  && mkdir -p /tmp/dsr-ai-resources-build \
  && cd /tmp/dsr-ai-resources-build \
  && qmake /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder/src/src.pro
```

Expected: qmake exits successfully. Qt Creator equivalent: `Build -> Run qmake`.

- [ ] **Step 3: Inspect relevant diff**

Run:

```bash
cd /home/ut006498@uos/code/deepin-screen-recorder/deepin-screen-recorder && git status --short && git diff -- assets/icons/icons.qrc translations.pri src/src.pro
```

Expected: status shows 7 new SVG files, modified `icons.qrc`, modified translations listed by `translations.pri`, and existing earlier AI DBus/source changes.

---

## Self-Review

- Spec coverage: The plan copies all 7 AI SVGs, registers them in `icons.qrc`, confirms `src.pro` resource coverage, and updates only translations.pri-listed `.ts` files with `AIAssistantWidget` strings.
- Placeholder scan: No unresolved placeholders are present.
- Type consistency: Paths consistently use V25 source under `/data/home/.../code1` and V20 target under `/home/.../code`.
