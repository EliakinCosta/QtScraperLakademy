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
        url: "https://dribbble.com/"
        actions: [
            {
                "endpoint": "https://dribbble.com/shots/8952865-Budget-planner-Mobile-concept",
                "method": "GET",
                "scraps": [
                    {
                        "name": "title",
                        "query": "/html/head/title/string()"
                    },
                    {
                        "name": "description",
                        "query": "/html/head/meta[9]/@content/string()"
                    },
                    {
                        "name": "images",
                        "query": "/html/body/div[4]/div[2]/div/div[2]/section/div/div[1]/div/div[1]/div/a/div/div/picture/@srcset/string()"
                    }
                ]
            }
        ]
    }

    Column {

        Label {

        }

        Label {

        }


        Button {
            text: "Scrap"
            onClicked: scraper.scrap()
        }
    }
}
