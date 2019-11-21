#include "qscrapengine.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTextCodec>
#include <QXmlQuery>
#include <QRegularExpression>
#include <QStringLiteral>
#include <QUrlQuery>

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

    auto reply = doHttpRequest(requestObj);
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
        qDebug() << payload;
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

void QScrapEngine::addRequest(QString httpMethod, QString endpoint, QJsonObject data)
{
    QHash<QString, QString> hashObj;

    hashObj.insert("httpMethod", httpMethod);
    hashObj.insert("endpoint", endpoint);

    // Convert QJsonObject to QString
    QJsonDocument doc(data);
    QByteArray docByteArray = doc.toJson(QJsonDocument::Compact);
    QString strJson = QLatin1String(docByteArray);
    hashObj.insert("data", strJson);

    m_requestsSchedule.append(hashObj);
}

QNetworkReply *QScrapEngine::doHttpRequest(QHash<QString, QString> requestObj)
{
    QString httpMethod = requestObj.value("httpMethod");
    QString endpoint = requestObj.value("endpoint");

    m_request.setUrl(endpoint);
    if(httpMethod ==  "GET") {
       return m_manager.get(m_request);
    } else if (httpMethod ==  "POST"){
        QStringList queryStringList;
        QString data = requestObj.value("data");
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonObject obj = doc.object();

        for (QJsonObject::const_iterator iter = obj.begin(); iter != obj.end(); ++iter) {
            queryStringList.append(QString("%1=%2").arg(iter.key(), evaluateStringToContext(iter.value().toString())));
        }

        QString queryString = QStringLiteral("?") + queryStringList.join("&");

        return m_manager.post(m_request, QUrlQuery {queryString}.toString(QUrl::FullyEncoded).toUtf8());
    }

    return nullptr;
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
        // For instance, we can capture only one variable for string.
        QRegularExpressionMatch match = i.next();
        QString templateKey = match.captured();
        QString templateValue = QScrapEngine::CONTEXT.value(templateKey).toString();

        value.replace(QString("%%%1%%").arg(templateKey), templateValue);
    }
    qDebug() << value;
    return value;
}

QJsonObject QScrapEngine::objectFromString(const QString& in)
{
    QJsonObject obj;

    QJsonDocument doc = QJsonDocument::fromJson(in.toUtf8());

    // check validity of the document
    if(!doc.isNull())
    {
        if(doc.isObject())
        {
            obj = doc.object();
        }
        else
        {
            qDebug() << "Document is not an object" << endl;
        }
    }
    else
    {
        qDebug() << "Invalid JSON...\n" << in << endl;
    }

    return obj;
}
