TEMPLATE = lib
CONFIG += plugin qtuiplugin debug_and_release
QT += widgets designer opengl openglwidgets

TARGET = graphwidgetplugin

include(../dest.pri)

win32 {
    IMPLIB = $$OBJECTS_DIR/../$${TARGET}.lib
    QMAKE_LFLAGS += /IMPLIB:$$IMPLIB
}

HEADERS += graphwidgetplugin.h
SOURCES += graphwidgetplugin.cpp
RESOURCES += icons.qrc

INCLUDEPATH += ../graphwidget
LIBS += -L$$DESTDIR -lgraphwidget -lopengl32

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS += target
