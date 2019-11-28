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
    m_request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral("application/x-www-form-urlencoded")
    );

    m_request.setRawHeader( "User-Agent" , "Mozilla Firefox" );

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
            qDebug() << "ERRO:" << reply->errorString().toLower();
            return;
        }
        auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode != 200 && statusCode != 302) {
            Q_EMIT scrapReply->finished(QHash<QString, QStringList>());
            return;
        }

        if(statusCode == 302)
        {
            QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            qDebug() << "redirected to " + newUrl.toString();
            QNetworkRequest newRequest(newUrl);
            m_manager.get(newRequest);
            return;
        }
        QString payload {reply->readAll()}; // clazy:exclude=qt4-qstring-from-array        
        tidyPayload(payload);
        qDebug() << "pAYLOAD:" << payload;
        qDebug() << "STATUS CODE:" << statusCode;

        QXmlQuery xmlQuery;
        QStringList list;
        if (requestObj.contains("query")) {
            xmlQuery.setFocus(payload);
            xmlQuery.setQuery(requestObj.value("query"));
            if (!xmlQuery.isValid()) {
                Q_EMIT scrapReply->finished(QHash<QString, QStringList>());
                return;
            }
            xmlQuery.evaluateTo(&list);

            saveToContext(requestObj.value("name"), list);

            Q_EMIT scrapReply->finished(list);
        }
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

void QScrapEngine::addRequest(QString httpMethod, QString endpoint, QJsonArray data)
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
        QString data = requestObj.value("data");
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        QJsonArray obj = doc.array();
        QUrlQuery query;

        for (QJsonArray::const_iterator iter = obj.begin(); iter != obj.end(); ++iter) {
            if (iter->isObject())
            {
                QJsonObject jsonObj = iter->toObject();
                for (QJsonObject::const_iterator it = jsonObj.begin(); it != jsonObj.end(); it++) {
                    query.addQueryItem(it.key(), evaluateStringToContext(it.value().toString()));
                }
            }
        }        
        QByteArray postData = query.toString(QUrl::FullyEncoded).toUtf8();

        qDebug() << postData;

        return m_manager.post(m_request, postData);
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
    QString new_value = value;
    QRegularExpression re("%%(.*?)%%");
    QRegularExpressionMatchIterator i = re.globalMatch(value);
    while (i.hasNext()) {
        // For instance, we can capture only one variable for string.
        QRegularExpressionMatch match = i.next();
        QString templateKey = match.captured(1);
        QString templateValue = QScrapEngine::CONTEXT.value(templateKey).toArray().first().toString();

        new_value = value.replace(QString("%%%1%%").arg(templateKey), templateValue);
    }
    qDebug() << new_value;
    return new_value;
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
