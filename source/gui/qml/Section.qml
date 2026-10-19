import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

Frame {
    id: section

    property string title
    property string blurb
    property bool collapsible: false
    property bool expanded: true
    default property alias content: body.data

    Layout.fillWidth: true
    padding: 12
    Material.elevation: 1

    ColumnLayout {
        anchors.left: parent.left
        anchors.right: parent.right
        spacing: 8

        RowLayout {
            Layout.fillWidth: true

            Label {
                Layout.fillWidth: true
                text: section.title
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }

            ToolButton {
                visible: section.collapsible
                text: section.expanded ? qsTr("Hide") : qsTr("Show")
                onClicked: section.expanded = !section.expanded
            }
        }

        Label {
            Layout.fillWidth: true
            visible: section.blurb.length > 0
            text: section.blurb
            wrapMode: Text.WordWrap
            opacity: 0.7
            font.pixelSize: 12
        }

        ColumnLayout {
            id: body
            Layout.fillWidth: true
            visible: section.expanded
            spacing: 6
        }
    }
}
