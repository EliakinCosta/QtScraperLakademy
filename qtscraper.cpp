#include "qtscraper.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QDebug>

#include "qscrapengine.h"

QtScraper::QtScraper(QObject *parent) : QObject(parent)
{

}

QtScraper::~QtScraper()
{

}

QJsonArray QtScraper::actions() const
{
    return m_actions;
}

void QtScraper::setActions(const QJsonArray &actions)
{
    m_actions = actions;
}

void QtScraper::scrap()
{
    foreach(QJsonValue jsonValue, m_actions)
    {
        QJsonObject jsonObject = jsonValue.toObject();
        QString endpoint = jsonObject.value("endpoint").toString();
        if (jsonObject.value("scraps").isArray())
        {
            QJsonArray scraps = jsonObject.value("scraps").toArray();
            foreach(QJsonValue scrap, scraps)
            {
                QJsonObject scrapObject = scrap.toObject();
                m_scrapEngine.addRequest(
                    "GET",
                    m_scrapEngine.evaluateStringToContext(endpoint),
                    m_scrapEngine.evaluateStringToContext(scrapObject.value("name").toString()),
                    m_scrapEngine.evaluateStringToContext(scrapObject.value("query").toString())
                );
            }
        }

    }
    m_scrapEngine.scrap();
}

void QtScraper::saveToContext()
{
    return;
}
