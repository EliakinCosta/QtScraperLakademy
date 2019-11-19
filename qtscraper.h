#ifndef QTSCRAPER_H
#define QTSCRAPER_H

#include <QObject>
#include <QJsonArray>

#include "qscrapengine.h"

class QtScraper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QJsonArray actions READ actions WRITE setActions)
public:
    explicit QtScraper(QObject *parent = nullptr);
    virtual ~QtScraper();
    QJsonArray actions() const;
    void setActions(const QJsonArray &actions);
    Q_INVOKABLE void scrap();
private:
    void saveToContext();
private:    
    QJsonArray m_actions;
    QScrapEngine m_scrapEngine;
};

#endif // QTSCRAPER_H
