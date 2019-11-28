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

QString QtScraper::url() const
{
    return m_url;
}

void QtScraper::setUrl(const QString url)
{
    m_url = url;
}

void QtScraper::scrap()
{
    m_scrapEngine.setBaseUrl(m_url);
    foreach(QJsonValue jsonValue, m_actions)
    {
        QJsonObject jsonObject = jsonValue.toObject();
        QString endpoint = jsonObject.value("endpoint").toString();
        if (jsonObject.value("method").isString())
        {
            if(jsonObject.value("method").toString() == "GET")
            {
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
            } else {
                if (jsonObject.value("data").isArray())
                {
                    QJsonArray postData = jsonObject.value("data").toArray();
                    m_scrapEngine.addRequest(
                        "POST",
                        m_scrapEngine.evaluateStringToContext(endpoint),
                        postData
                    );
                }
            }
        }

    }
    m_scrapEngine.scrap();
}

void QtScraper::saveToContext()
{
    return;
}
