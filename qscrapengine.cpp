#include "qscrapengine.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTextCodec>
#include <QXmlQuery>
#include <QRegularExpression>

#include <tidy.h>
#include <tidybuffio.h>

QJsonObject QScrapEngine::CONTEXT;

ScrapReply::ScrapReply(QObject *parent)
    : QObject {parent}
{

}

QScrapEngine::QScrapEngine(QObject *parent) : QObject(parent)
{
    QSslConfiguration conf = m_request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    m_request.setSslConfiguration(conf);
}

QScrapEngine::~QScrapEngine()
{
}

void QScrapEngine::tidyPayload(QString &payload)
{
    TidyDoc tdoc = tidyCreate();
    tidyOptSetBool(tdoc, TidyXmlOut, yes);
    tidyOptSetBool(tdoc, TidyQuiet, yes);
    tidyOptSetBool(tdoc, TidyNumEntities, yes);
    tidyOptSetBool(tdoc, TidyShowWarnings, no);

    tidyParseString(tdoc, payload.toUtf8());
    tidyCleanAndRepair(tdoc);
    TidyBuffer output = {nullptr, nullptr, 0, 0, 0};
    tidySaveBuffer(tdoc, &output);

    payload = QString::fromUtf8(reinterpret_cast<char*>(output.bp)); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
}


void QScrapEngine::scrap()
{
    if (m_requestsSchedule.size() == 0 || m_scheduleIndex >= m_requestsSchedule.size())
    {
        qDebug() << QScrapEngine::CONTEXT;
        return;
    }

    QHash<QString, QString> requestObj;
    requestObj = m_requestsSchedule.at(m_scheduleIndex);

    m_request.setUrl(QUrl(requestObj.value("endpoint")));
    auto reply = m_manager.get(m_request);
    auto scrapReply = new ScrapReply {this};

    connect (reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NetworkError::NoError) {
            Q_EMIT scrapReply->finished(QHash<QString, QStringList>());
            return;
        }
        auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode != 200 && statusCode != 302) {
            Q_EMIT scrapReply->finished(QHash<QString, QStringList>());
            return;
        }
        QString payload {reply->readAll()}; // clazy:exclude=qt4-qstring-from-array
        tidyPayload(payload);
        QXmlQuery xmlQuery;
        xmlQuery.setFocus(payload);

        xmlQuery.setQuery(requestObj.value("query"));
        if (!xmlQuery.isValid()) {
            Q_EMIT scrapReply->finished(QHash<QString, QStringList>());
            return;
        }
        QStringList list;
        xmlQuery.evaluateTo(&list);

        saveToContext(requestObj.value("name"), list);

        Q_EMIT scrapReply->finished(list);
        m_scheduleIndex++;
        scrap();
    });

}

void QScrapEngine::addRequest(QString httpMethod, QString endpoint, QString var, QString query)
{
    QHash<QString, QString> hashObj;

    hashObj.insert("httpMethod", httpMethod);
    hashObj.insert("endpoint", endpoint);
    hashObj.insert("name", var);
    hashObj.insert("query", query);

    m_requestsSchedule.append(hashObj);
}

QString QScrapEngine::fromByteArrayToString(QByteArray html)
{
    return QTextCodec::codecForName("iso-8859-1")->toUnicode(html);
}

void QScrapEngine::saveToContext(QString key, QStringList value)
{
    QScrapEngine::CONTEXT.insert(key, QJsonArray::fromStringList(value));    
}

QString QScrapEngine::evaluateStringToContext(QString value)
{
    QRegularExpression re("%%(.*?)%%");
    QRegularExpressionMatchIterator i = re.globalMatch(value);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        qDebug() << match.captured();
    }
    return value;
}
