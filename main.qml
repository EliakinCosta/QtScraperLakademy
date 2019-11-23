import QtQuick 2.13
import QtQuick.Controls 2.13
import QtQuick.Window 2.13
import com.ifba.scraping 1.0

Window {
    visible: true
    width: 640
    height: 480
    title: qsTr("Hello World")

    QWebScraper {
        id: scraper
        actions: [
            {
                "endpoint": "https://suap.ifba.edu.br/accounts/login/?next=/",
                "method": "GET",
                "scraps": [
                    {
                        "name": "token",
                        "query": "/html/body/div[1]/div[1]/form/input/@value/string()"
                    }
                ]
            },
            {
                "endpoint": "https://suap.ifba.edu.br/accounts/login/?next=/",
                "method": "POST",
                "data": [
                    {"csrfmiddlewaretoken": "%%token%%"},
                    {"username": txtUsername.text},
                    {"password": txtPassword.text},
                    {"this_is_the_login_form": "1"},
                    {"next": "/"}
                ],
                "validator": "Início - SUAP: Sistema Unificado de Administração Pública"
            }
        ]
    }

    Column {

        TextField {
            id: txtUsername
            color: "black"
        }

        TextField {
            id: txtPassword
            color: "black"
        }

        Button {
            text: "Test"
            onClicked: scraper.scrap()
        }
    }
}
