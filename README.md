# 🟢 Rate24 — Cyberpunk Productivity Tracker

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus&logoColor=white)
![Qt](https://img.shields.io/badge/Qt-6.5%2B-41CD52?logo=qt&logoColor=white)
![QML](https://img.shields.io/badge/UI-QML-00ff41)
![Platform](https://img.shields.io/badge/platform-Linux-black)

> *Rate your day like you're running a root shell.*

**Rate24** is a sleek, terminal-inspired desktop application built with **C++** and **Qt6/QML**.
It tracks your daily habits, scores your performance with a custom weighted algorithm, and visualizes your progress over time, all wrapped in a living **Matrix/Hacker** aesthetic.

<!-- Add a screenshot or GIF here, e.g.:
![Rate24 demo](docs/demo.gif)
-->

---

## ✨ Key Features

### 🎨 Look & feel
* **Cyberpunk UI:** custom terminal-style QML components (`[ Buttons ]`, blinking block cursor, neon-green on black) and a live **Matrix Rain** background drawn on a `Canvas`. The animation pauses automatically when the window is minimized, so it doesn't waste CPU.
* **Keyboard-first:** everything can be driven without touching the mouse (see [Controls](#️-controls)).
* **Zero bloat:** no heavy chart libraries (`QtCharts` is gone). Progress charts are a lightweight, custom rendering made of native QML items that mimic terminal block bars (`▰▰▰▱▱▱`), colored green / amber / red by performance.

### 📊 Tracking & scoring
* **Guided daily flow:** log *time* and *quality* for each activity, one by one.
* **Weighted scoring:** activities have priorities, minimum time and minimum quality. Your day score is a weighted combination of how well you met those targets (see [How scoring works](#-how-scoring-works)).
* **SD / TO checkpoint:** missed the time goal but kept quality high? You're asked to decide honestly: was it a **Strategic Decision (SD)** or a **Tragic Oversight (TO)**? SD earns a bonus, TO doesn't.
* **Smart input rules:** quality is limited to 0–100, time to 24 h, and if you log **0 minutes** the quality field is ignored entirely.
* **Locked notes cards:** at the end of the day, every activity gets its own card. The title is **locked** (you can't accidentally edit or delete it) and you write your reflection in the field below. Long text wraps inside the box, so it never pushes the layout around, and the view **follows your cursor** as you type, so you always see the line you're writing.
* **Day boundary at 04:00:** a "day" runs from 04:00 to 03:59. Finish your session at 03:59 and it still counts for the previous date.

### 🕳️ Missed days never leave holes
* **Computer was off / the app never ran:** on startup, every day without an entry (from the first day in your history up to yesterday) is filled automatically with **zeros**.
* **Closed the window with ✕ (or quit without saving):** the day is recorded right away. Activities you had already logged keep their values, everything else is `0`. If you reopen the app on the same day, you **resume where you left off**, and a normal save replaces the placeholder.
* **Crashed or killed?** A leftover draft from a previous day is turned into that day's entry the next time you start the app.
* These entries are flagged in the data (`missed`, `incomplete`) and clearly labelled in the day view, so you can always tell them apart from real runs.

### ✏️ Day viewer & editor
Click any day in the total chart (or use `Tab` → `↑/↓` → `Enter`) to open its report:
* **Everything is editable:** time, quality, SD and notes of every activity, so if you forgot to log a day you can fill it in the next one.
* **Automatic recalculation:** on save, every activity score and the day's total are recomputed with the **exact same algorithm** as a normal run. The same rules apply: time ≤ 24 h, quality 0–100, quality ignored at 0 minutes, and **SD only when time is below the minimum and quality is above it**.
* **Safe editing:** `Ctrl+S` saves; pressing `Esc` with unsaved changes warns you first, and a second `Esc` discards them.
* **Older entries** that only contain a single combined notes text are shown (and editable) as one block.
* **Past scores stay stable:** each activity's priority is stored inside the entry, so changing priorities (or names) in the code later never changes how an old day is recalculated. Entries from before this feature get their priority stamped automatically the first time the app starts.

### 🏆 Personal bests
When you finish a day, Rate24 compares it with everything you've logged before. If your **day score** beats your previous best, a pulsing `★ NEW PERSONAL BEST!` banner appears with the old record and the improvement. Any **individual activity** that beats its own previous best is listed under `# New bests:`. Only strictly better results count, and the current date is never compared with itself.

### 🔎 Browse first, log later
You don't have to enter your progress just to look at the data. On the first screen choose **Browse History** (or press `Ctrl+H`) to jump straight to the charts and the day editor. Nothing is written for today while you browse, and closing the window in this mode **does not** fill the day with zeros. `Back` (or `Esc`) returns you to the activity flow exactly where you were.

### 📄 Export everything to a text file
**Export TXT** (on the first screen and on the charts screen) writes all your data and notes to `~/Documents/Rate24/rate24_export.txt`, neatly aligned: a header with totals, an overview with mini bars for every day, a table per day (time, quality, SD, score) followed by its notes, and per-activity averages at the end. The file is overwritten on each export, so copy it elsewhere if you want to keep a snapshot.

### 📈 Progress charts
* **Total chart:** your daily score over the last *N* days (type any number from 0 to 365, press **Enter**; `0` = everything). Bars are green at **≥ 100** points or ≥ 80 % of your best day, amber at ≥ 50 %, red below.
* **Per-activity charts:** one click on **Activity Charts** reveals an individual evolution chart for every activity, each scaled to its own best day, just like the CLI version.

### 🛡️ Bulletproof data safety
* **Atomic writes:** uses `QSaveFile` to write to a temporary file first, preventing data corruption after power loss or sudden crashes.
* **Auto-backups:** if `performance.json` turns out to be corrupted, a `.bak` copy is created before anything else happens, and automatic steps (zero-filling, exit recording) refuse to write over it.
* **Draft saving:** closed the app halfway through your day? Rate24 saves your progress after every activity and restores it when you reopen the app on the same day.
* **One entry per day:** run it 30 times in a day and only the **latest** real run is kept in the history.
* **Undo:** typed a wrong number? Press **Esc** to step back one activity.

### 🗂️ Where your data lives
Everything is stored in a fixed folder, regardless of where you launch the app from:

```
~/Documents/Rate24/
├── performance.json      # your full history
├── performance.json.bak  # only appears if the file was ever found corrupted
├── rate24_export.txt     # created by "Export TXT"
└── draft.json            # temporary, exists only while a day is in progress
```

The folder is created automatically. If an older `performance.json` exists in the folder you launch the app from, it is copied over on first run.

---

## 🚀 Build & Run

**Requirements**

* A C++17 compiler, `cmake` and `ninja`
* **Qt 6.5 or newer** (inline QML components are used) with the modules **Core, Gui, Qml, Quick**
* Qt Quick Controls 2 and Qt Quick Layouts at runtime (provided by the `qt6-declarative` / `qml6-module-qtquick-controls` packages on most distros)
* *Optional but recommended:* a monospace font such as **JetBrains Mono**, **Fira Code**, **Hack** or **DejaVu Sans Mono**. Rate24 picks the best one it finds.

```bash
# 1. Clone the repository
git clone https://github.com/VasilisPetropoulos/Rate24_GUI.git
cd Rate24_GUI

# 2. Configure and build the project
mkdir build && cd build
cmake -G Ninja ..
ninja

# 3. Run the application
./Rate24_GUI
```

---

## ⌨️ Controls

| Key | Where | Action |
|---|---|---|
| `Enter` | Time field | Move to Quality (or submit directly if time is `0`) |
| `Enter` | Quality field / any button | Submit / press the focused button |
| `Tab` / `←` `→` | SD / TO question | Switch between **SD** and **TO**, then `Enter` to choose |
| `Tab` / `Shift+Tab` | Notes | Jump between note fields, then to the buttons |
| `Esc` | Input / SD / warning | Step back (cancel the question, or undo the previous activity) |
| `Esc` | Notes screen | Undo the last activity (only while notes are empty, so you never lose text) |
| Type a number + `Enter` | Chart screen | Show the last *N* days (0 = all) |
| Click, or `Tab` → `↑/↓` → `Enter` | Total chart | Open that day's report |
| `Ctrl+H` | First screen | Browse history / charts without logging today |
| `Esc` | Activity charts | Back to the total chart |
| `Esc` | Charts opened with *Browse History* | Back to the activity flow |
| `Ctrl+S` | Day report | Save your edits and recalculate the scores |
| `Esc` | Day report | Back to the chart (asks for confirmation if there are unsaved changes) |
| `↑` `↓` `PgUp` `PgDn` | Day report | Scroll |

---

## 🧮 How scoring works

Every activity has a **minimum time**, a **minimum quality** and a **priority** (1 = most important). With `N` activities, an activity's weight is:

```
importance = N − priority
```

For each activity:

```
completion = real_time / min_time
quality    = quality   / min_quality      (ignored when real_time = 0)
ratio      = completion × quality
```

The day score is the weighted sum, normalized to a 0–100 scale at the minimum targets:

```
day_score = Σ(ratio × importance  +  SD bonus) / Σ(importance) × 100
```

* An **SD** adds a bonus worth **10 %** of the maximum possible score.
* The score is **not capped**. Working twice as long, or earning SD bonuses, can push the day **above 100**. That's a feature: it rewards going beyond the plan.
* Hitting exactly your minimum time and quality on every activity gives **100**.
* The same function computes scores for a normal run and for edited days, so results are always consistent.

Default activities:

| Activity | Min time | Min quality | Priority |
|---|---|---|---|
| Cybersecurity Studying | 60 min | 65 | 1 |
| University Studying | 30 min | 65 | 2 |
| Piano Studying | 30 min | 65 | 3 |
| Book Reading | 30 min | 65 | 4 |
| Greek Studying | 15 min | 65 | 5 |
| English Studying | 15 min | 65 | 5 |

---

## 💾 Data format

Each day is one entry in `performance.json`:

```json
{
    "date": "2026-10-25",
    "score": 136.3,
    "notes": "Piano Studying:\nWorked on scales\n\n...",
    "activities": [
        {
            "name": "Piano Studying",
            "real_time": 20,
            "min_time": 30,
            "quality": 80,
            "min_quality": 65,
            "score": 25.4,
            "sd_used": true,
            "priority": 3,
            "note": "Worked on scales"
        }
    ]
}
```

Entries created or changed automatically may carry one of these optional flags:

| Flag | Meaning |
|---|---|
| `"missed": true` | Nothing was logged for that day (all zeros) |
| `"incomplete": true` | The app was closed before the day was saved; only the activities you had already logged have values |
| `"edited": true` | The day was modified afterwards in the day editor |

---

## ⚙️ Nightly Automation (Systemd)

Rate24 is designed to prompt you every night before bed. Unlike CLI apps, GUI applications need access to the display server, so we provide ready-to-use **Systemd user services** for seamless Wayland/X11 integration.

```bash
# From the project root, copy the service files to your local systemd folder:
mkdir -p ~/.config/systemd/user/
cp linux_automation/rate24.service ~/.config/systemd/user/
cp linux_automation/rate24.timer ~/.config/systemd/user/

# Reload the daemon and enable the nightly prompt (triggers at 23:08)
systemctl --user daemon-reload
systemctl --user enable --now rate24.timer
```

Useful commands:

```bash
systemctl --user list-timers           # see when the next prompt fires
systemctl --user status rate24.timer   # check the timer
systemctl --user disable --now rate24.timer   # turn the nightly prompt off
```

If the computer is off at 23:08, nothing is lost: the missing day is filled with zeros the next time the app starts, and you can fill it in later from the day editor.

### Already filled the day earlier? Skip the nightly prompt

Add the `--auto` flag to the service so the timer-launched instance knows it is the automatic one:

```ini
# ~/.config/systemd/user/rate24.service
ExecStart=/full/path/to/Rate24_GUI --auto
```

```bash
systemctl --user daemon-reload
```

With `--auto`, if today has **already been saved**, the app quits silently before any window appears. If you only filled part of the day (or closed it with ✕ halfway), it opens normally so you can continue. Launching the app by hand (without the flag) always opens it.

---

## 🛠️ Customizing

* **Activities:** names, minimum time/quality and priorities are defined in `Rate_24_Backend::loadDefaultActivities()`.
* **Data folder:** set in `dataDir()` at the top of `Rate_24_Backend.cpp` (default `~/Documents/Rate24`).
* **Day boundary:** change `kDayStartHour` (default `4`) in `Rate_24_Backend.cpp`.
* **Matrix rain:** tweak speed, density and characters in the `Canvas` at the top of the QML file (`interval`, `cell`, `glyphs`).
* **Colors:** all theme colors are `readonly property color` values on the root window.

---

## 🤝 Contributing

Issues and pull requests are welcome. If you add a feature, keep the philosophy: **fast, keyboard-friendly, zero bloat, and hacker-approved.**

*Stay green.* 🟢
