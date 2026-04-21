TEMPLATE = lib
CONFIG += staticlib
QT += widgets opengl openglwidgets

TARGET = graphwidget

include(../dest.pri)

HEADERS +=  graphwidget.h \
            graphdata.h

SOURCES +=  graphwidget.cpp \
            graphdata.cpp

INCLUDEPATH += $$PWD
