import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S
import "storage.js" as DB

// Saved pages, laid out like the conversation list in HAL 2027:
// name (two lines) and address. Tap opens it, hold for edit / remove.
// Pull the list down at its top to search (as in MeeGo Touch lists).
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait
    property bool searching: false
    property int selected: -1
    property string selectedTitle: ""
    property string selectedUrl: ""
    property bool barAnimated: true

    function reload() {
        model.clear();
        var rows = DB.list(searching ? search.text : "");
        for (var i = 0; i < rows.length; ++i)
            model.append(rows[i]);
    }
    function openSearch() {
        if (searching)
            return;
        closeAnim.stop();
        barAnimated = false;
        searching = true;
        barAnimated = true;
        openAnim.restart();
        search.forceActiveFocus();
        search.platformOpenSoftwareInputPanel();
        idle.restart();
    }
    function closeSearch() {
        if (!searching)
            return;
        idle.stop();
        search.platformCloseSoftwareInputPanel();
        openAnim.stop();
        closeAnim.restart();
    }
    Component.onCompleted: reload()

    Image { source: "background.png"; y: -36; smooth: false }
    ListModel { id: model }
    Header { id: header; title: S.t(uiLang, "bookmarks") }

    Item {
        id: searchBar
        anchors { top: header.bottom; left: parent.left; right: parent.right }
        height: page.searching ? 76 : 0
        clip: true
        Behavior on height {
            enabled: page.barAnimated
            NumberAnimation { duration: 300; easing.type: Easing.OutCubic }
        }
        Item {
            id: searchContent
            width: parent.width
            height: 76
            Rectangle {
                anchors { fill: parent; leftMargin: 16; rightMargin: 16; topMargin: 10; bottomMargin: 10 }
                radius: 12
                color: "#0c0c0c"
            }
            Image {
                id: lens
                anchors { left: parent.left; leftMargin: 28; verticalCenter: parent.verticalCenter }
                source: "image://theme/icon-m-common-search-inverse"
                opacity: 0.7
            }
            TextField {
                id: search
                anchors { left: lens.right; right: clearIcon.left; verticalCenter: parent.verticalCenter }
                inputMethodHints: Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
                platformStyle: TextFieldStyle {
                    background: ""; backgroundSelected: ""; backgroundDisabled: ""; backgroundError: ""
                    textColor: "#ffffff"
                    paddingLeft: 8
                }
                onTextChanged: { page.reload(); idle.restart(); }
                Keys.onReturnPressed: search.platformCloseSoftwareInputPanel()
            }
            Image {
                id: clearIcon
                anchors { right: parent.right; rightMargin: 24; verticalCenter: parent.verticalCenter }
                source: "image://theme/icon-m-input-clear"
                visible: search.text != ""
                width: visible ? implicitWidth : 0
                MouseArea {
                    anchors { fill: parent; margins: -12 }
                    onClicked: { search.text = ""; search.forceActiveFocus(); }
                }
            }
        }
    }
    ParallelAnimation {
        id: openAnim
        NumberAnimation { target: searchContent; property: "opacity"; from: 0; to: 1; duration: 300; easing.type: Easing.OutQuad }
        NumberAnimation { target: searchContent; property: "y"; from: 30; to: 0; duration: 300; easing.type: Easing.OutCubic }
    }
    SequentialAnimation {
        id: closeAnim
        NumberAnimation { target: searchContent; property: "opacity"; to: 0; duration: 200 }
        ScriptAction { script: { search.text = ""; page.searching = false; page.reload(); } }
    }
    Timer {
        id: idle
        interval: 5000
        onTriggered: if (search.text == "") page.closeSearch()
    }

    ListView {
        id: list
        anchors { top: searchBar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        clip: true
        model: model
        property real pullStart: 0
        property bool pulling: false
        onMovementStarted: {
            pullStart = contentY;
            pulling = !page.searching && contentY <= 0;
            if (page.searching)
                idle.restart();
        }
        onMovementEnded: pulling = false
        onContentYChanged: {
            if (!pulling)
                return;
            var distance = pullStart - contentY;
            if (distance >= 50) {
                pulling = false;
                page.openSearch();
                contentY += 76;
            } else if (distance < 0) {
                pulling = false;
            }
        }
        delegate: Item {
            width: list.width
            height: 104
            Rectangle {
                anchors.fill: parent
                color: "#ffffff"
                opacity: mouse.pressed ? 0.12 : 0
            }
            Column {
                anchors { left: parent.left; right: parent.right; leftMargin: 16; rightMargin: 16; verticalCenter: parent.verticalCenter }
                Label {
                    width: parent.width
                    text: model.title
                    wrapMode: Text.Wrap
                    elide: Text.ElideRight
                    maximumLineCount: 2
                    font.pixelSize: 24
                    color: "white"
                }
                Label {
                    width: parent.width
                    text: model.url.replace(/^https?:\/\//, "").replace(/\/$/, "")
                    elide: Text.ElideRight
                    font.pixelSize: 20
                    color: "#a0a0a0"
                }
            }
            MouseArea {
                id: mouse
                anchors.fill: parent
                onClicked: appWindow.open(model.url)
                onPressAndHold: {
                    page.selected = model.bid;
                    page.selectedTitle = model.title;
                    page.selectedUrl = model.url;
                    itemMenu.open();
                }
            }
        }
    }
    ScrollDecorator { flickableItem: list }

    Label {
        anchors { centerIn: parent; verticalCenterOffset: -40 }
        width: parent.width - 80
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.Wrap
        visible: model.count == 0 && !page.searching
        text: S.t(uiLang, "noBookmarks")
        color: "#808080"
        font.pixelSize: 26
    }

    ContextMenu {
        id: itemMenu
        MenuLayout {
            MenuItem { text: S.t(uiLang, "open"); onClicked: appWindow.open(page.selectedUrl) }
            MenuItem { text: S.t(uiLang, "edit"); onClicked: editSheet.edit(page.selected, page.selectedTitle, page.selectedUrl) }
            MenuItem { text: S.t(uiLang, "remove"); onClicked: { DB.remove(page.selected); page.reload(); } }
        }
    }

    BookmarkSheet {
        id: editSheet
        onSaved: page.reload()
    }

    tools: ToolBarLayout {
        ToolIcon {
            iconId: "toolbar-back"
            onClicked: pageStack.pop()
        }
    }
}
