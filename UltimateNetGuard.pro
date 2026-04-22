##############################################################################
#   UltimateNetGuard
#   Copyright © 2026  AliSakkaf  |  All Rights Reserved
#-----------------------------------------------------------------------------
#   Website  : https://mysterious-dev.com/
#   GitHub   : https://github.com/alisakkaf
#   Facebook : https://www.facebook.com/AliSakkaf.Dev/
##############################################################################


QT += core gui widgets network charts winextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG   += c++14
CONFIG   -= app_bundle

TARGET   = UltimateNetGuard
TEMPLATE = app

VERSION  = 1.0.0
RC_FILE  = appicon.rc

# ─── Source files ───────────────────────────────────────────────────────────
SOURCES += \
    src/main.cpp \
    src/core/mainwindow.cpp \
    src/core/apptheme.cpp \
    src/core/stylemanager.cpp \
    src/core/updatechecker.cpp \
    src/core/appinstaller.cpp \
    src/network/networkmonitor.cpp \
    src/network/networkmodel.cpp \
    src/network/networkwidget.cpp \
    src/network/packetparser.cpp \
    src/firewall/firewallmanager.cpp \
    src/firewall/firewallmodel.cpp \
    src/firewall/firewallwidget.cpp \
    src/hardware/hardwaremonitor.cpp \
    src/hardware/hardwarewidget.cpp \
    src/taskbar/taskbaroverlay.cpp \
    src/taskbar/systemtrayicon.cpp

# ─── Headers ────────────────────────────────────────────────────────────────
HEADERS += \
    src/core/version.h \
    src/core/mainwindow.h \
    src/core/apptheme.h \
    src/core/stylemanager.h \
    src/core/updatechecker.h \
    src/core/appinstaller.h \
    src/network/networkmonitor.h \
    src/network/networkmodel.h \
    src/network/networkwidget.h \
    src/network/packetparser.h \
    src/firewall/firewallmanager.h \
    src/firewall/firewallmodel.h \
    src/firewall/firewallwidget.h \
    src/hardware/hardwaremonitor.h \
    src/hardware/hardwarewidget.h \
    src/taskbar/taskbaroverlay.h \
    src/taskbar/systemtrayicon.h

# ─── Qt Designer UI forms ────────────────────────────────────────────────────
FORMS += \
    src/ui/mainwindow.ui \
    src/ui/networkwidget.ui \
    src/ui/firewallwidget.ui \
    src/ui/hardwarewidget.ui

# ─── Resources ──────────────────────────────────────────────────────────────
RESOURCES += resources.qrc

# ─── Include paths ──────────────────────────────────────────────────────────
INCLUDEPATH += \
    src \
    src/core \
    src/network \
    src/firewall \
    src/hardware \
    src/taskbar

# ─── Windows system libraries ────────────────────────────────────────────────
win32: LIBS += \
    -lws2_32 \
    -liphlpapi \
    -lole32 \
    -loleaut32 \
    -lwbemuuid \
    -lpdh \
    -ladvapi32 \
    -lshell32 \
    -luser32 \
    -lgdi32 \
    -luuid \
    -lshlwapi

# ─── Compiler / preprocessor flags ──────────────────────────────────────────
mingw {
    QMAKE_CXXFLAGS += -O2 -Wno-unknown-pragmas -Wno-unused-parameter
    DEFINES += UNICODE _UNICODE \
               WIN32_LEAN_AND_MEAN \
               NOMINMAX \
               _WIN32_WINNT=0x0601 \
               WINVER=0x0601
}

# ─── Output dirs ────────────────────────────────────────────────────────────
DESTDIR     = $$OUT_PWD/bin
OBJECTS_DIR = $$OUT_PWD/.obj
MOC_DIR     = $$OUT_PWD/.moc
RCC_DIR     = $$OUT_PWD/.rcc
UI_DIR      = $$OUT_PWD/.ui
