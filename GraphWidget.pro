TEMPLATE = subdirs
CONFIG += ordered

SUBDIRS +=  graphwidget \
            graphwidgetplugin

graphwidgetplugin.depends = graphwidget
