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
* **Keyboard-first:** everything can be driven without touching the mouse (see [Controls](#-controls)).
* **Zero bloat:** no heavy chart libraries (`QtCharts` is gone). Progress charts are a lightweight, custom rendering made of native QML items that mimic terminal block bars (`▰▰▰▱▱▱`), colored green / amber / red by performance.

### 📊 Tracking & scoring
* **Guided daily flow:** log *time* and *quality* for each activity, one by one.
* **Weighted scoring:** activities have priorities, minimum time and minimum quality. Your day score is a weighted combination of how well you met those targets (see [How scoring works](#-how-scoring-works)).
* **SD / TO checkpoint:** missed the time goal but kept quality high? You're asked to decide honestly: was it a **Strategic Decision (SD)** or a **Tragic Oversight (TO)**? SD earns a bonus, TO doesn't.
* **Smart input rules:** quality is limited to 0–100, time to 24 h, and if you log **0 minutes** the quality field is ignored entirely.
* **Locked notes cards:** at the end of the day, every activity gets its own card. The title is **locked** (you can't accidentally edit or delete it) and you write your reflection in the field below. Notes are saved both as a combined text and **per activity** in the JSON.

### 📈 Progress charts
* **Total chart:** your daily score over the last *N* days (type any number from 0 to 365, press **Enter**; `0` = everything). Bars are green at **≥ 100** points or ≥ 80 % of your best day, amber at ≥ 50 %, red below.
* **Per-activity charts:** one click on **Activity Charts** reveals an individual evolution chart for every activity, each scaled to its own best day, exactly like the CLI version.

### 🛡️ Bulletproof data safety
* **Atomic writes:** uses `QSaveFile` to write to a temporary file first, preventing data corruption after power loss or sudden crashes.
* **Auto-backups:** detects corrupted JSON and creates a `.bak` backup before generating fresh data.
* **Draft saving:** closed the app halfway through your day? Rate24 saves your progress after every activity and restores it when you reopen the app on the same day.
* **One entry per day:** run it 30 times in a day and only the **latest** run is kept in the history.
* **Undo:** typed a wrong number? Press **Esc** to step back one activity.

### 🗂️ Clean data location (XDG-compliant)
Your data lives in `~/.local/share` (the app's data directory), not in your source tree, and it doesn't matter where you launch the app from.
Prefer a custom location? Set the `RATE24_DATA_DIR` environment variable.
If an old `performance.json` exists in the current folder, it is copied over automatically on first run.

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

Use a custom data directory (optional):

```bash
RATE24_DATA_DIR=~/Documents/rate24 ./Rate24_GUI
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
| `Esc` | Activity charts | Back to the total chart |
| Type a number + `Enter` | Chart screen | Show the last *N* days (0 = all) |

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
            "note": "Worked on scales"
        }
    ]
}
```

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

---

## 🛠️ Customizing

* **Activities:** names, minimum time/quality and priorities are defined in `Rate_24_Backend::loadDefaultActivities()`.
* **Matrix rain:** tweak speed, density and characters in the `Canvas` at the top of the QML file (`interval`, `cell`, `glyphs`).
* **Colors:** all theme colors are `readonly property color` values on the root window.

---

## 🤝 Contributing

Issues and pull requests are welcome. If you add a feature, keep the philosophy: **fast, keyboard-friendly, zero bloat, and hacker-approved.**

*Stay green.* 🟢
