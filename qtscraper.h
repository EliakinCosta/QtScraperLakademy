#ifndef QTSCRAPER_H
#define QTSCRAPER_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>

#include "qscrapengine.h"
#include "qwebscraperstatus.h"

class QtScraper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QJsonArray actions READ actions WRITE setActions)
    Q_PROPERTY(QString url READ url WRITE setUrl)
    Q_PROPERTY(QJsonObject ctx READ ctx)
    Q_PROPERTY(QWebScraperStatus::Status status READ status NOTIFY statusChanged)
public:
    explicit QtScraper(QObject *parent = nullptr);
    virtual ~QtScraper();
    QJsonArray actions() const;
    void setActions(const QJsonArray &actions);    
    QString url() const;
    void setUrl(const QString url);
    QJsonObject ctx() const;

    QWebScraperStatus::Status status() const;

    Q_INVOKABLE void scrap();

Q_SIGNALS:
    void statusChanged(QWebScraperStatus::Status);

private:
    void saveToContext();
private:    
    QJsonArray m_actions;    
    QString m_url;
    QScrapEngine m_scrapEngine;
};

#endif // QTSCRAPER_H
