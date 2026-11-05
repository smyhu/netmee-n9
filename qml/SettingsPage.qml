import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S

// Filters, home page and search service.
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait
    property variant web

    Image { source: "background.png"; y: -36; smooth: false }
    Header { id: header; title: S.t(uiLang, "settings") }

    Flickable {
        id: flick
        anchors { top: header.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        contentHeight: col.height + 32
        clip: true
        Column {
            id: col
            anchors { left: parent.left; right: parent.right; margins: 16 }
            spacing: 14

            SectionLabel { text: S.t(uiLang, "filters") }
            Item {
                width: parent.width; height: 64
                Label { anchors { left: parent.left; right: sw1.left; verticalCenter: parent.verticalCenter }
                        text: S.t(uiLang, "thirdParty"); wrapMode: Text.Wrap; color: "white" }
                Switch { id: sw1; anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                         checked: web.blockThirdPartyScripts
                         onCheckedChanged: web.blockThirdPartyScripts = checked }
            }
            Item {
                width: parent.width; height: 64
                Label { anchors { left: parent.left; right: sw2.left; verticalCenter: parent.verticalCenter }
                        text: S.t(uiLang, "jsDefault"); wrapMode: Text.Wrap; color: "white" }
                Switch { id: sw2; anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                         checked: web.jsDefault
                         onCheckedChanged: web.jsDefault = checked }
            }
            Label { text: S.t(uiLang, "maxScript"); color: "white" }
            ButtonRow {
                width: parent.width
                Button { text: "100 KB"; checked: web.maxScriptKB == 100; onClicked: web.maxScriptKB = 100 }
                Button { text: "200 KB"; checked: web.maxScriptKB == 200; onClicked: web.maxScriptKB = 200 }
                Button { text: "500 KB"; checked: web.maxScriptKB == 500; onClicked: web.maxScriptKB = 500 }
                Button { text: "∞"; checked: web.maxScriptKB == 0; onClicked: web.maxScriptKB = 0 }
            }

            SectionLabel { text: S.t(uiLang, "look") }
            ButtonRow {
                width: parent.width
                Button { text: S.t(uiLang, "styleOriginal"); checked: web.pageStyle == 0; onClicked: web.pageStyle = 0 }
                Button { text: S.t(uiLang, "styleDark"); checked: web.pageStyle == 1; onClicked: web.pageStyle = 1 }
                Button { text: S.t(uiLang, "styleSepia"); checked: web.pageStyle == 2; onClicked: web.pageStyle = 2 }
            }

            SectionLabel { text: S.t(uiLang, "homePage") }
            TextField {
                id: home
                width: parent.width
                text: web.text("home", "https://lite.duckduckgo.com/lite/")
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                onTextChanged: if (text != "") web.setText("home", text)
            }

            SectionLabel { text: S.t(uiLang, "searchWith") }
            ButtonColumn {
                width: parent.width
                property string engine: web.text("engine", "duckduckgo")
                Button { text: "FrogFind"; checked: parent.engine == "frogfind"; onClicked: web.setText("engine", "frogfind") }
                Button { text: "DuckDuckGo Lite"; checked: parent.engine == "duckduckgo"; onClicked: web.setText("engine", "duckduckgo") }
                Button { text: "wiby.me"; checked: parent.engine == "wiby"; onClicked: web.setText("engine", "wiby") }
                Button { text: S.t(uiLang, "customEngine"); checked: parent.engine == "custom"; onClicked: web.setText("engine", "custom") }
            }
            TextField {
                width: parent.width
                text: web.text("customEngine", "")
                placeholderText: "https://example.com/search?q=%s"
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                onTextChanged: web.setText("customEngine", text)
            }
            Label {
                width: parent.width
                wrapMode: Text.Wrap
                text: S.t(uiLang, "customHint")
                color: "#909090"
                font.pixelSize: 20
            }
        }
    }
    ScrollDecorator { flickableItem: flick }

    tools: ToolBarLayout {
        ToolIcon { iconId: "toolbar-back"; onClicked: pageStack.pop() }
    }
}
