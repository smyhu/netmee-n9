import QtQuick 1.1
import com.nokia.meego 1.0
import com.nokia.extras 1.1

PageStackWindow {
    id: appWindow
    showStatusBar: true
    initialPage: BrowserPage { id: browser }

    property color accent: "#eaffd0"
    property color tintHeader: "#373e2e"
    property color tintBorder: "#5a6650"

    function showBanner(text) {
        banner.text = text;
        banner.show();
    }
    function open(url) {
        pageStack.pop(browser);
        browser.go(url);
    }

    InfoBanner {
        id: banner
        y: 36 + 8
    }

    Component.onCompleted: {
        theme.inverted = true;
        theme.colorScheme = 4;
    }
}
