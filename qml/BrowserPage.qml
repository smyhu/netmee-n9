import QtQuick 1.1
import com.nokia.meego 1.0
import NetMee 1.0
import "strings.js" as S
import "storage.js" as DB

// Main screen: address bar, the page, and the toolbar
// (back, home, star, bookmarks, menu).
Page {
    id: page
    orientationLock: PageOrientation.LockPortrait
    property int bookmarkId: -1
    property real zoom: parseFloat(web.text("zoom", "1.5"))
    property variant engines: { "frogfind": "http://frogfind.com/?q=",
                                "duckduckgo": "https://lite.duckduckgo.com/lite/?q=",
                                "wiby": "http://wiby.me/?q=" }
    // search address for the words: "%s" marks where they go, else they are added at the end
    function searchUrl(words) {
        var e = web.text("engine", "duckduckgo");
        var base = e == "custom" ? web.text("customEngine", "") : engines[e];
        if (!base)
            base = engines["duckduckgo"];
        var q = encodeURIComponent(words);
        return base.indexOf("%s") >= 0 ? base.replace("%s", q) : base + q;
    }

    function homeUrl() { return web.text("home", "https://lite.duckduckgo.com/lite/"); }

    // address or search words -> URL
    function go(text) {
        var t = text.replace(/^\s+|\s+$/g, "");
        if (t == "")
            return;
        var url;
        if (/^[a-z]+:/i.test(t))
            url = t;
        else if (t.indexOf(" ") < 0 && /\.[a-z]{2,}(\/|:|$)/i.test(t))
            url = "http://" + t;
        else
            url = searchUrl(t);
        address.platformCloseSoftwareInputPanel();
        page.forceActiveFocus();
        web.load(url);
    }
    function shortUrl(u) {
        return u.replace(/^http:\/\//, "").replace(/\/$/, "");
    }
    function refreshStar() { bookmarkId = DB.find(web.url); }
    function showFailure(url) {
        var reader = "http://frogfind.com/read.php?a=" + encodeURIComponent(url);
        web.showMessage("<html><head><meta name='viewport' content='width=device-width'></head>" +
            "<body style='background:#000;color:#ddd;font-family:Nokia Pure Text,sans-serif;font-size:18px;margin:16px'>" +
            "<p style='font-size:22px;color:#fff'>" + S.t(uiLang, "failTitle") + "</p>" +
            "<p style='color:#999;word-wrap:break-word'>" + url + "</p>" +
            "<p>" + S.t(uiLang, "failText") + "</p>" +
            "<p><a style='color:" + appWindow.accent + "' href='" + reader + "'>" + S.t(uiLang, "failReader") + "</a></p></body></html>");
    }

    Image { source: "background.png"; y: -36; smooth: false }

    // address bar
    Item {
        id: bar
        anchors { top: parent.top; left: parent.left; right: parent.right }
        height: 80
        Rectangle {
            anchors { fill: parent; leftMargin: 12; rightMargin: 12; topMargin: 12; bottomMargin: 8 }
            radius: 12
            color: "#0c0c0c"
        }
        TextField {
            id: address
            anchors { left: parent.left; leftMargin: 20; right: action.left; verticalCenter: parent.verticalCenter; verticalCenterOffset: 2 }
            placeholderText: S.t(uiLang, "addressHint")
            inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            platformStyle: TextFieldStyle {
                background: ""; backgroundSelected: ""; backgroundDisabled: ""; backgroundError: ""
                // while not editing, the label below shows the address
                textColor: address.activeFocus ? "#ffffff" : "transparent"
                paddingLeft: 4
            }
            onActiveFocusChanged: {
                if (activeFocus) {
                    text = web.url;
                    selectAll();
                } else {
                    text = page.shortUrl(web.url);
                }
            }
            Keys.onReturnPressed: page.go(text)
            Keys.onEnterPressed: page.go(text)
        }
        // the address while not editing: start of it, cut at the end
        Label {
            anchors { left: address.left; leftMargin: 4; right: address.right; verticalCenter: address.verticalCenter }
            visible: !address.activeFocus
            text: address.text == "" ? "" : page.shortUrl(web.url)
            elide: Text.ElideRight
            color: "#ffffff"
            font.pixelSize: 24
        }
        // reload / stop
        Image {
            id: action
            anchors { right: parent.right; rightMargin: 22; verticalCenter: parent.verticalCenter; verticalCenterOffset: 2 }
            source: web.loading ? "image://theme/icon-m-toolbar-stop-white" : "image://theme/icon-m-toolbar-refresh-white"
            opacity: actionMouse.pressed ? 0.5 : 0.85
            MouseArea {
                id: actionMouse
                anchors { fill: parent; margins: -12 }
                onClicked: web.loading ? web.stop() : web.reload()
            }
        }
        // loading progress along the bottom of the field
        Rectangle {
            anchors { left: parent.left; leftMargin: 18; bottom: parent.bottom; bottomMargin: 9 }
            height: 3
            radius: 1
            width: (parent.width - 36) * web.progress / 100
            color: appWindow.accent
            visible: web.loading
        }
    }

    PinchArea {
        id: pinch
        anchors { top: bar.bottom; left: parent.left; right: parent.right; bottom: parent.bottom }
        pinch.target: zoomFeedback
        pinch.minimumScale: 0.5
        pinch.maximumScale: 3.0
        onPinchFinished: {
            var z = Math.max(0.75, Math.min(3.0, page.zoom * zoomFeedback.scale));
            zoomFeedback.scale = 1;
            page.zoom = Math.round(z * 4) / 4;
            web.setText("zoom", "" + page.zoom);
        }
        Item { id: zoomFeedback }

        Flickable {
            id: flick
            anchors.fill: parent
            clip: true
            contentWidth: Math.max(width, web.contentWidth)
            contentHeight: Math.max(height, web.contentHeight)
            boundsBehavior: Flickable.DragOverBounds
            pressDelay: 0
            transform: Scale {
                origin.x: flick.width / 2
                origin.y: flick.height / 2
                xScale: zoomFeedback.scale
                yScale: zoomFeedback.scale
            }
            WebItem {
                id: web
                width: Math.max(flick.width, contentWidth)
                height: Math.max(flick.height, contentHeight)
                zoom: page.zoom
                viewportWidth: Math.round(flick.width / page.zoom)
                onUrlChanged: {
                    page.refreshStar();
                    if (!address.activeFocus) {
                        address.text = page.shortUrl(url);
                    }
                    web.setText("last", url);
                }
                onLoadingChanged: if (loading) { flick.contentX = 0; flick.contentY = 0; }
                onLoadFailed: page.showFailure(url)
            }
        }
        ScrollDecorator { flickableItem: flick }
    }

    Menu {
        id: menu
        MenuLayout {
            MenuItem {
                text: S.t(uiLang, "reader")
                onClicked: web.load("http://frogfind.com/read.php?a=" + encodeURIComponent(web.url))
            }
            MenuItem {
                text: S.t(uiLang, web.jsEnabled ? "jsOn" : "jsOff")
                onClicked: {
                    web.setJsAllowed(web.hostOf(web.url), !web.jsEnabled);
                    web.reload();
                }
            }
            MenuItem {
                text: S.t(uiLang, "look") + ": " + S.t(uiLang, ["styleOriginal", "styleDark", "styleSepia"][web.pageStyle])
                onClicked: web.pageStyle = (web.pageStyle + 1) % 3
            }
            MenuItem {
                text: S.t(uiLang, "setHome")
                onClicked: { web.setText("home", web.url); appWindow.showBanner(S.t(uiLang, "homeSet")); }
            }
            MenuItem {
                text: S.t(uiLang, "settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), { "web": web })
            }
            MenuItem {
                text: S.t(uiLang, "about")
                onClicked: aboutDialog.open()
            }
        }
    }

    QueryDialog {
        id: aboutDialog
        titleText: "NetMee " + appVersion
        message: S.t(uiLang, "aboutText") + "<br><br>" + S.t(uiLang, "credit") +
                 "<br><br>MIT · <a href=\"https://github.com/smyhu/netmee-n9\" style=\"color:" + appWindow.accent +
                 "; text-decoration:underline\">github.com/smyhu/netmee-n9</a>"
        acceptButtonText: S.t(uiLang, "close")
        onLinkActivated: { aboutDialog.close(); page.go(link); }
    }

    tools: ToolBarLayout {
        ToolIcon {
            iconId: "toolbar-back"
            opacity: web.canGoBack ? 1 : 0.4
            onClicked: if (web.canGoBack) web.back()
        }
        ToolIcon {
            iconId: "toolbar-home"
            onClicked: web.load(page.homeUrl())
        }
        ToolIcon {
            iconId: page.bookmarkId >= 0 ? "toolbar-favorite-mark" : "toolbar-favorite-unmark"
            onClicked: {
                if (page.bookmarkId >= 0) {
                    DB.remove(page.bookmarkId);
                    appWindow.showBanner(S.t(uiLang, "removed"));
                } else if (web.url != "") {
                    DB.add(web.title, web.url);
                    appWindow.showBanner(S.t(uiLang, "added"));
                }
                page.refreshStar();
            }
        }
        ToolIcon {
            iconId: "toolbar-list"
            onClicked: pageStack.push(Qt.resolvedUrl("BookmarksPage.qml"))
        }
        ToolIcon {
            iconId: "toolbar-view-menu"
            onClicked: menu.status == DialogStatus.Closed ? menu.open() : menu.close()
        }
    }

    onStatusChanged: if (status == PageStatus.Active) refreshStar()
    Component.onCompleted: {
        var start = web.text("last", "");
        web.load(start != "" ? start : homeUrl());
    }
}
