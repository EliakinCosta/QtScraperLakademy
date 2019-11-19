#include "qtscraperplugin.h"
#include "qtscraper.h"
#include <qqml.h>

void QtScraperPlugin::registerTypes(const char *uri)
{
    qmlRegisterType<QtScraper>(uri, 1, 0, "com.ifba.QWebScraper");
}
