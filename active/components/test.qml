import QtQuick 1.1
import org.kde.okular 0.1

Item {
    width: 400
    height: 200
    DocumentItem {
        id: documentItem
        path: "/home/diau/Books/Fox Trapping A Book of Instruction Telling How to Trap, Snare, Poison and Shoot - A Valuable Book for Trappers - 29483.epub"
    }
    Flickable {
        anchors {
            left: thumbnailsView.right
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }
        contentWidth: page.width
        contentHeight: page.height
        PageItem {
            id: page
            document: documentItem
            width: implicitWidth
            height: implicitHeight
        }
    }
    ListView {
        id: thumbnailsView
        width: 100
        height: parent.height
        model: documentItem.pageCount
        delegate: Rectangle {
            color: "red"
            width: parent.width
            height: width
            ThumbnailItem {
                id: thumbnail
                document: documentItem
                pageNumber: modelData
                width: parent.width
                height: parent.width / (implicitWidth/implicitHeight)
            }
            Text {
                text: modelData+1
            }
            Text {
                text: thumbnail.implicitWidth+"x"+thumbnail.implicitHeight
            }
            MouseArea {
                anchors.fill: parent
                onClicked: page.pageNumber = modelData
            }
        }
    }
}
