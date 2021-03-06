#include "qscrapengine.h"

#include <QDebug>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QTextCodec>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QRegularExpression>
#include <QStringLiteral>
#include <QUrlQuery>
#include <QNetworkCookie>
#include <QNetworkCookieJar>

#include <tidy.h>
#include <tidybuffio.h>

#include "qwebscraperstatus.h"


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

    this->m_manager.setCookieJar(&m_cookieJar);

    QObject::connect(&m_manager, &QNetworkAccessManager::finished,
                         this, &QScrapEngine::replyFinished);
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
        setStatus(QWebScraperStatus::Ready);
        return;
    }

    QHash<QString, QString> requestObj;
    requestObj = m_requestsSchedule.at(m_scheduleIndex);

    doHttpRequest(requestObj);
    setStatus(QWebScraperStatus::Loading);
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

    if (!endpoint.toLower().startsWith("http"))
        endpoint = m_baseUrl + endpoint;

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

        QList<QNetworkCookie> cookies = m_manager.cookieJar()->cookiesForUrl(m_urlLogin);
        for(auto it = cookies.begin(); it != cookies.end(); ++it){
            qDebug() << cookies.toStdList();
            m_request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(*it));
        }


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

        QJsonValue test = QScrapEngine::CONTEXT.value(templateKey);
        qDebug() << "test" << test.toString();

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

void QScrapEngine::setBaseUrl(QString baseUrl)
{
    m_baseUrl = baseUrl;
}

void QScrapEngine::replyFinished(QNetworkReply *reply)
{
    reply->deleteLater();

    QHash<QString, QString> requestObj;
    requestObj = m_requestsSchedule.at(m_scheduleIndex);

    if (reply->error() != QNetworkReply::NetworkError::NoError) {
        qDebug() << "ERRO:" << reply->errorString().toLower();
        setStatus(QWebScraperStatus::Error);
        return;
    }

    m_urlLogin.setUrl(requestObj.value("endpoint"));
    QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie>>(reply->header(QNetworkRequest::SetCookieHeader));
    if(cookies.count() != 0){
        //you must tell which cookie goes with which url
        qDebug() << cookies.toStdList();
        m_manager.cookieJar()->setCookiesFromUrl(cookies, m_urlLogin);
    }

    auto statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode != 200 && statusCode != 302) {
        setStatus(QWebScraperStatus::Error);
        return;
    }

    if(statusCode == 302)
    {
        QUrl newUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
        qDebug() << "redirected to " + newUrl.toString();
        QHash<QString, QString> hashObj;

        hashObj.insert("httpMethod", "GET");
        hashObj.insert("endpoint", newUrl.toString());

        auto replyRedirect = doHttpRequest(hashObj);

        connect (replyRedirect, &QNetworkReply::finished, this, [=]() {
            replyRedirect->deleteLater();

            auto statusCode = replyRedirect->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            QString payload {replyRedirect->readAll()}; // clazy:exclude=qt4-qstring-from-array
            tidyPayload(payload);

        });
        return;
    }
    QString payload {reply->readAll()}; // clazy:exclude=qt4-qstring-from-array
    tidyPayload(payload);
    qDebug() << "pAYLOAD:" << payload.mid(2000);
    qDebug() << "STATUS CODE:" << statusCode;

    QXmlQuery xmlQuery;
    QString result;
    QStringList list;    
    xmlQuery.setFocus(payload);
    xmlQuery.setQuery(requestObj.value("query"));
    if (xmlQuery.isValid()) {
        xmlQuery.evaluateTo(&list);
        saveToContext(requestObj.value("name"), list);
    }

    m_scheduleIndex++;
    scrap();
}

QWebScraperStatus::Status QScrapEngine::status() const
{
    return m_status;
}

void QScrapEngine::setStatus(QWebScraperStatus::Status status)
{
    if (m_status!=status)
    {
        m_status = status;
        Q_EMIT statusChanged(m_status);
    }
}

void QScrapEngine::setCookies(const QString &cookies)
{
    auto cookieJar = m_manager.cookieJar();
    for (const auto &cookie : cookieJar->cookiesForUrl(QUrl("https://talentos.carreirarh.com/site/user_login/"))) {
        if (cookie.name() == "csrftoken" || cookie.name() == "sessionid")
            cookieJar->deleteCookie(cookie);
    }
    for (const auto &newCookie : cookies.split(';')) {
        auto newCookieValues = newCookie.split('=');
        if (newCookieValues.size() > 1)
        {
            qDebug() << newCookieValues.first().trimmed().toLatin1() << newCookieValues.last().trimmed().toLatin1();
            QNetworkCookie newCookieObject(newCookieValues.first().trimmed().toLatin1(), newCookieValues.last().trimmed().toLatin1());
            newCookieObject.setDomain("talentos.carreirarh.com");
            newCookieObject.setPath("/site/user_login/");
            cookieJar->insertCookie(newCookieObject);
        }
    }

    QList<QNetworkCookie> cookies2 = m_manager.cookieJar()->cookiesForUrl(QUrl("https://www.fifaindex.com/accounts/login/"));
    qDebug() << "Size: " << cookies2.size();
    for(auto it = cookies2.begin(); it != cookies2.end(); ++it){
       m_request.setHeader(QNetworkRequest::CookieHeader, QVariant::fromValue(*it));
    }
}

