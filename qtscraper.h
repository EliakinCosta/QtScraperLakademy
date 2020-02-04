#ifndef QTSCRAPER_H
#define QTSCRAPER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

#include "qscrapengine.h"

class QtScraper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QJsonArray actions READ actions WRITE setActions)
    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(QJsonObject ctx READ ctx)
public:
    explicit QtScraper(QObject *parent = nullptr);
    virtual ~QtScraper();
    QJsonArray actions() const;
    void setActions(const QJsonArray &actions);    
    QString url() const;
    void setUrl(const QString url);
    QJsonObject ctx() const;
    Q_INVOKABLE void scrap();
private:
    void saveToContext();
private:    
    QJsonArray m_actions;    
    QString m_url;
    QScrapEngine m_scrapEngine;
};

#endif // QTSCRAPER_H
