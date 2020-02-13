import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Window 2.13
import com.ifba.scraping 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    Item {
        id: itemLogin
        property string userSigned

        anchors.fill: parent

        QWebScraper {
            id: scraper
            url: "https://talentos.carreirarh.com/"
            actions: [
                {
                    "endpoint": "https://talentos.carreirarh.com/site/user_login/",
                    "method": "GET",
                    "scraps": [
                        {
                            "name": "token",
                            "query": "/html/body/div/div/div[2]/div/form/input/@value/string()"
                        }
                    ]
                },
                {
                    "endpoint": "https://talentos.carreirarh.com/site/user_login/",
                    "method": "POST",
                    "data": [
                        {"csrftoken": "%%token%%"},
                        {"username": usernameText.text},
                        {"password": passwordText.text}
                    ]
                }
            ]
            onStatusChanged: {
                if (scraper.status === QWebScraperStatus.Ready) {
                    console.log(JSON.stringify(scraper.ctx));
                    itemLogin.userSigned = "admin";

                }
            }
        }

        Column {
            anchors.centerIn: parent
            spacing: 10
            visible: !scraperIndicator.running

            Column {
                spacing: 5

                TextInput {
                    id: usernameText
                    cursorVisible: true
                    width: 100
                }

                TextInput {
                    id: passwordText
                    cursorVisible: true
                    width: 100
                }

                Button {
                    id: loginButton
                    text: userLabel.visible ? "OK" : "Login"
                    onClicked: scraper.scrap()
                }
            }

            Label {
                id: userLabel
                text: itemLogin.userSigned ? itemLogin.userSigned : ""
                visible: itemLogin.userSigned
            }
        }

        BusyIndicator {
            id: scraperIndicator
            width: 150
            height: 150
            anchors.centerIn: parent
            running: scraper.status === QWebScraperStatus.Loading
        }
    }
}
