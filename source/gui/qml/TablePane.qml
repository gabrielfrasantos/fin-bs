import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

// A result table: the headings the presentation layer named, the strings it formatted, nothing
// recomputed on the way to the screen.
Item {
    id: pane

    property var table
    property string emptyText: qsTr("Nothing to show until something has been run.")

    function widthOf(column) {
        const heading = pane.table.headings[column]
        if (heading === "underlying" || heading === "measure" || heading === "kind")
            return 170
        if (heading === "date")
            return 110
        return 130
    }

    // A Flickable adopts its children, so the hint for an empty table lives beside the layout.
    Label {
        anchors.centerIn: parent
        anchors.verticalCenterOffset: 16
        width: parent.width - 48
        z: 1
        visible: pane.table.rows === 0
        text: pane.emptyText
        wrapMode: Text.WordWrap
        horizontalAlignment: Text.AlignHCenter
        opacity: 0.6
        font.italic: true
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        HorizontalHeaderView {
            id: header
            Layout.fillWidth: true
            syncView: grid
            clip: true
            delegate: Rectangle {
                implicitWidth: pane.widthOf(column)
                implicitHeight: 32
                color: Material.theme === Material.Dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.05)
                required property int column
                required property string display

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    text: display
                    font.weight: Font.DemiBold
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }

        TableView {
            id: grid
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: pane.table
            columnWidthProvider: function(column) { return pane.widthOf(column) }
            rowHeightProvider: function() { return 28 }
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {}
            ScrollBar.horizontal: ScrollBar {}

            delegate: Rectangle {
                required property int row
                required property int column
                required property string display
                implicitWidth: pane.widthOf(column)
                implicitHeight: 28
                color: row % 2 === 0 ? "transparent" : (Material.theme === Material.Dark ? Qt.rgba(1, 1, 1, 0.03) : Qt.rgba(0, 0, 0, 0.025))

                Label {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 8
                    text: display
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: isNaN(Number(display.replace(/,/g, ""))) || display.length === 0 ? Text.AlignLeft : Text.AlignRight
                    elide: Text.ElideRight
                    font.family: "monospace"
                }
            }
        }
    }
}
