#-------------------------------------------------
#
# Project created by QtCreator 2013-10-20T06:28:36
#
#-------------------------------------------------

QT       += core

QT       += dbus

QT       -= gui

TARGET = autoplayer-qt-cpp
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    udisks.cpp \
    autoplayer.cpp \
    rhythmbox_mpris.cpp \
    playlist_manager.cpp \
    mpris_playlist.cpp

HEADERS += \
    udisks.h \
    autoplayer.h \
    rhythmbox_mpris.h \
    playlist_manager.h \
    mpris_playlist.h

OTHER_FILES += \
    xml/org.freedesktop.UDisks.xml \
    xml/org.freedesktop.UDisks.Device.xml

unix:!macx:!symbian: LIBS += -L/usr/lib/x86_64-linux-gnu/ -lQtDBus

INCLUDEPATH += $$PWD/
DEPENDPATH += $$PWD/

unix:!macx:!symbian: LIBS += -L/usr/lib/ -lmagic

INCLUDEPATH += /usr/include
DEPENDPATH += /usr/include
