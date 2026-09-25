THIS IS OLD CODE, CREATED AT SOME POINT BETWEEN 2006 AND 2014

Autoplayer

A program that automatically starts playing music from a memory stick after inserted (on a Linux OS variant)

This is an old education POC about using D-Bus on Linux

Three examples using:
- Python-DBUS
- QT
- Glic

Building / running (tested on Debian 13 with Qt 6.8, GLib 2.84, Python 3.13):

- Python: requires Python 3 with dbus-python and PyGObject
  (`apt install python3-dbus python3-gi`), then `python3 autoplayer.py`
- Qt: requires Qt 6 (Core, D-Bus) and libmagic
  (`apt install qt6-base-dev libmagic-dev`), then
  `cd autoplayer-qt-cpp && qmake6 && make`
- GLib: requires GLib/GIO 2.36+ (`apt install libglib2.0-dev pkg-config`), then
  `cd autoplayer-glib-c && make`

Note: the examples talk to the UDisks 1 (`org.freedesktop.UDisks`) and
Rhythmbox 3 playlist-manager D-Bus APIs, which modern distributions no
longer ship (UDisks 2 uses `org.freedesktop.UDisks2`).
