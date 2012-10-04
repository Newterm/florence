QT += svg xml
TARGET = florence_qt
TEMPLATE = lib
VERSION = 1.0.0
SOURCES += key.cpp \
    florence.cpp \
    style.cpp \
    keymap.cpp \
    symbol.cpp \
    stylesymbol.cpp \
    styleshape.cpp \
    styleitem.cpp \
    settings.cpp
HEADERS += key.h \
    florence.h \
    style.h \
    keymap.h \
    symbol.h \
    stylesymbol.h \
    styleshape.h \
    style.h \
    keymap.h \
    key.h \
    florence.h \
    styleitem.h \
    settings.h
OTHER_FILES += COPYING
RESOURCES = florence_qt.qrc

prefix.path = /usr
target.path = $$prefix.path/lib
INSTALLS += target

plugin {
    SOURCES += florenceplugin.cpp
    HEADERS += florenceplugin.h
    CONFIG += designer plugin debug_and_release
    target.path = $$[QT_INSTALL_PLUGINS]/designer
    INSTALLS += target
}
