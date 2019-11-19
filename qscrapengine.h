#ifndef QSCRAPENGINE_H
#define QSCRAPENGINE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

class QByteArray;
class QNetworkReply;

class ScrapReply : public QObject
{
    Q_OBJECT

public:
    explicit ScrapReply(QObject *parent = nullptr);

Q_SIGNALS:
    void finished(const QStringList &result);
    void finished(const QHash<QString, QStringList> &result);
    void finished(int statusCode, bool result);
};

class QScrapEngine : public QObject
{
    Q_OBJECT
public:
    explicit QScrapEngine(QObject *parent = nullptr);
    virtual ~QScrapEngine();
    void scrap();
    void addRequest(QString httpMethod, QString endpoint, QString var, QString query);
    QString evaluateStringToContext(QString value);
    static void tidyPayload(QString &payload);
    static QJsonObject CONTEXT;
//public slots:
//    void replyFinished (QNetworkReply *reply);
private:
    QString fromByteArrayToString(QByteArray html);
    void saveToContext(QString key, QStringList value);
private:
    QNetworkAccessManager m_manager;
    QNetworkRequest m_request;
    QList<QHash<QString, QString>> m_requestsSchedule;
    int m_scheduleIndex = 0;
};

#endif // QSCRAPENGINE_H
