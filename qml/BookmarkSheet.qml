import QtQuick 1.1
import com.nokia.meego 1.0
import "strings.js" as S
import "storage.js" as DB

// Edit the name and address of a bookmark.
Sheet {
    id: sheet
    property int bid: -1
    signal saved()

    function edit(id, title, url) {
        bid = id;
        titleField.text = title;
        urlField.text = url;
        open();
    }

    acceptButtonText: S.t(uiLang, "save")
    rejectButtonText: S.t(uiLang, "cancel")

    content: Item {
        anchors.fill: parent
        Image { source: "background.png"; anchors.centerIn: parent; smooth: false }
        Column {
            anchors { left: parent.left; right: parent.right; top: parent.top; margins: 16 }
            spacing: 10
            SectionLabel { text: S.t(uiLang, "title") }
            TextField { id: titleField; width: parent.width }
            SectionLabel { text: S.t(uiLang, "address") }
            TextField {
                id: urlField
                width: parent.width
                inputMethodHints: Qt.ImhUrlCharactersOnly | Qt.ImhNoPredictiveText | Qt.ImhNoAutoUppercase
            }
        }
    }
    onAccepted: {
        if (urlField.text != "")
            DB.update(bid, titleField.text != "" ? titleField.text : urlField.text, urlField.text);
        saved();
    }
}
