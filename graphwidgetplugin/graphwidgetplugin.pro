TEMPLATE = lib
CONFIG += plugin qtuiplugin debug_and_release
QT += widgets designer

TARGET = graphwidgetplugin

HEADERS += graphwidgetplugin.h
SOURCES += graphwidgetplugin.cpp
RESOURCES += icons.qrc

INCLUDEPATH += ../graphwidget
LIBS += -L$$OUT_PWD/../graphwidget/release -lgraphwidget

target.path = $$[QT_INSTALL_PLUGINS]/designer
INSTALLS += target
