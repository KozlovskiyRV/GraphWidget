#include "graphwidgetplugin.h"
#include "graphwidget.h"

#include <QtPlugin>

GraphWidgetPlugin::GraphWidgetPlugin(QObject *parent)
    : QObject(parent)
{}

void GraphWidgetPlugin::initialize(QDesignerFormEditorInterface * /* core */)
{
    if (m_initialized)
        return;

    // Add extension registrations, etc. here

    m_initialized = true;
}

bool GraphWidgetPlugin::isInitialized() const
{
    return m_initialized;
}

QWidget *GraphWidgetPlugin::createWidget(QWidget *parent)
{
    return new GraphWidget(parent);
}

QString GraphWidgetPlugin::name() const
{
    return QLatin1String("GraphWidget");
}

QString GraphWidgetPlugin::group() const
{
    return QLatin1String("My OpenGL Group");
}

QIcon GraphWidgetPlugin::icon() const
{
    return QIcon();
}

QString GraphWidgetPlugin::toolTip() const
{
    return QLatin1String("");
}

QString GraphWidgetPlugin::whatsThis() const
{
    return QLatin1String("OpenGL-based Qt Designer Widget for charts rendering");
}

bool GraphWidgetPlugin::isContainer() const
{
    return false;
}

QString GraphWidgetPlugin::domXml() const
{
    return QLatin1String(R"(<widget class="GraphWidget" name="graphWidget">
</widget>)");
}

QString GraphWidgetPlugin::includeFile() const
{
    return QLatin1String("graphwidget.h");
}
