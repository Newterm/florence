#include <QtPlugin>
#include "florenceplugin.h"
#include "florence.h"

florencePlugin::florencePlugin( QObject *parent )
        : QObject( parent )
{
    this->initialized = false;
}

bool florencePlugin::isInitialized() const
{
    return this->initialized;
}

void florencePlugin::initialize( QDesignerFormEditorInterface *core )
{
    if ( !initialized ) QDesignerCustomWidgetInterface::initialize( core );
    this->initialized = true;
}

QWidget *florencePlugin::createWidget(QWidget *parent)
{
    return new Florence( parent );
}

QString florencePlugin::name() const
{
    return "Florence";
}

QString florencePlugin::group() const
{
    return "Input Widgets";
}

QIcon florencePlugin::icon() const
{
    return QIcon();
}

QString florencePlugin::toolTip() const
{
    return "Florence Virtual Keyboard for Qt";
}

QString florencePlugin::whatsThis() const
{
    return "Florence is a Virtual Keyboard that can be used to input text in another widget.";
}

bool florencePlugin::isContainer() const
{
    return false;
}

QString florencePlugin::domXml() const
{
    return "<widget class=\"Florence\" name=\"florence\">\n"
           " <property name=\"geometry\">\n"
           "  <rect>\n"
           "   <x>0</x>\n"
           "   <y>0</y>\n"
           "   <width>300</width>\n"
           "   <height>100</height>\n"
           "  </rect>\n"
           " </property>\n"
           "</widget>\n";
}

QString florencePlugin::includeFile() const
{
    return "florence.h";
}

Q_EXPORT_PLUGIN2(florenceplugin, florencePlugin)
