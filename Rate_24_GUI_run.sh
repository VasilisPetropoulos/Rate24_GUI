#!/bin/bash

export XDG_RUNTIME_DIR=/run/user/1000
export DISPLAY=:0
export WAYLAND_DISPLAY=wayland-0
export DBUS_SESSION_BUS_ADDRESS="unix:path=/run/user/1000/bus"

/home/Vasilis/Desktop/Vasilis/Rating_Project_GUI/build/Rate24_GUI
