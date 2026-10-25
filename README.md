🟢 Rate24 - Cyberpunk Productivity Tracker

A sleek, terminal-inspired desktop application built with C++ and Qt6/QML.
Rate24 is designed to track your daily habits, evaluate your performance based on a custom scoring algorithm, and visualize your progress over time—all wrapped in a highly optimized "Matrix/Hacker" aesthetic.

✨ Key Features

Cyberpunk UI: Custom-built terminal-like QML components with dynamic "Matrix Rain" canvas animations (CPU-optimized to pause when minimized).

Zero Bloat: Complete removal of heavy chart libraries (like QtCharts). The progress visualizer is a custom, lightweight rendering built entirely with native QML items to mimic terminal block characters.

Bulletproof Data Safety:

Atomic Writes: Utilizes QSaveFile to write data to a temporary file first, ensuring zero data corruption in case of power loss or sudden crashes.

Auto-Backups: Automatically detects corrupted JSON files and creates .bak backups before generating fresh data.

Draft Saving: Accidentally closed the app halfway through your day? Rate24 saves your state dynamically and restores your progress upon reopening.

XDG Compliant: Safely stores your performance.json data in ~/.local/share (or any custom directory via the RATE24_DATA_DIR environment variable), keeping your source tree clean.

🚀 Build & Run Instructions

Dependencies:
Ensure you have cmake, ninja, and qt6 (Core, Gui, Qml, Quick) installed on your system.

# 1. Clone the repository
git clone https://github.com/VasilisPetropoulos/Rate24_GUI.git
cd Rate24_GUI

# 2. Configure and build the project
mkdir build && cd build
cmake -G Ninja ..
ninja

# 3. Run the application
./Rate24_GUI


⚙️ Nightly Automation (Systemd)

Rate24 is designed to prompt you every night before bed. Unlike CLI apps, GUI applications require access to the display server. We provide ready-to-use Systemd user services for seamless Wayland/X11 integration.

# From the project root, copy the service files to your local systemd folder:
mkdir -p ~/.config/systemd/user/
cp linux_automation/rate24.service ~/.config/systemd/user/
cp linux_automation/rate24.timer ~/.config/systemd/user/

# Reload the daemon and enable the nightly prompt (triggers at 23:08)
systemctl --user daemon-reload
systemctl --user enable --now rate24.timer
