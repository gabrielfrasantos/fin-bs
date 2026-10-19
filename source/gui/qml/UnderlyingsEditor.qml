import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    id: editor

    signal estimateRequested(int index)

    readonly property var scenario: workspace.scenario
    readonly property var underlyings: scenario.underlyings
    readonly property var numbers: Qt.locale("en_US")
    readonly property var widths: [80, 78, 62, 74, 74, 74, 70, 70]
    property int current: -1

    function money(cents) {
        return scenario.currency + " " + Number(cents / 100).toLocaleString(numbers, "f", 2)
    }

    function percent(basisPoints) {
        return Number(basisPoints / 100).toLocaleString(numbers, "f", 2) + " %"
    }

    spacing: 6

    Row {
        id: heading
        spacing: 2

        Repeater {
            model: 8

            Label {
                width: editor.widths[index]
                text: editor.underlyings.heading(index)
                font.weight: Font.DemiBold
                font.pixelSize: 12
                elide: Text.ElideRight
                ToolTip.visible: hovered
                ToolTip.text: editor.underlyings.unit(index)
                property bool hovered: mouse.containsMouse

                MouseArea {
                    id: mouse
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }
        }
    }

    ListView {
        id: rows
        objectName: "underlyings"
        Layout.fillWidth: true
        Layout.preferredHeight: Math.min(320, Math.max(40, contentHeight))
        clip: true
        model: editor.underlyings
        currentIndex: editor.current
        spacing: 2

        delegate: Row {
            id: line
            required property int index
            required property var model
            spacing: 2

            Repeater {
                model: 8

                TextField {
                    required property int index
                    readonly property int column: index
                    width: editor.widths[column]
                    height: 38
                    topPadding: 10
                    bottomPadding: 6
                    leftPadding: 6
                    rightPadding: 6
                    font.pixelSize: 13
                    horizontalAlignment: column === 0 ? Text.AlignLeft : Text.AlignRight
                    text: editor.underlyings.cell(line.index, column)
                    validator: column === 0 ? null : intValidator
                    background: Rectangle {
                        color: line.index === editor.current ? Qt.rgba(0.247, 0.318, 0.71, 0.14) : "transparent"
                        border.color: line.index === editor.current ? Material.accent : Qt.rgba(0.5, 0.5, 0.5, 0.35)
                        radius: 3
                    }
                    onActiveFocusChanged: if (activeFocus) editor.current = line.index
                    onEditingFinished: {
                        if (!editor.underlyings.setCell(line.index, column, text))
                            text = editor.underlyings.cell(line.index, column)
                    }

                    Connections {
                        target: editor.underlyings
                        function onDataChanged() { text = editor.underlyings.cell(line.index, column) }
                        function onModelReset() { text = editor.underlyings.cell(line.index, column) }
                    }
                }
            }
        }
    }

    IntValidator {
        id: intValidator
        bottom: -1000000000
        top: 2147483647
    }

    Label {
        Layout.fillWidth: true
        visible: editor.current >= 0 && editor.current < editor.underlyings.count
        wrapMode: Text.WordWrap
        font.italic: true
        opacity: 0.85
        text: {
            if (editor.current < 0 || editor.current >= editor.underlyings.count)
                return ""
            const u = editor.underlyings
            const i = editor.current
            const spot = u.cell(i, 1)
            const collateral = spot * 100
            const share = scenario.capitalCents > 0 ? Number(collateral / scenario.capitalCents * 100).toLocaleString(editor.numbers, "f", 1) : "-"
            return u.cell(i, 0) + ": spot " + editor.money(spot) + ", one contract about " + editor.money(collateral) + " (" + share + " % of capital), drift " + editor.percent(u.cell(i, 2)) + ", volatility " + editor.percent(u.cell(i, 3)) + " realised, " + editor.percent(u.cell(i, 4)) + " implied, yield " + editor.percent(u.cell(i, 5)) + " withheld at " + editor.percent(u.cell(i, 6)) + ", strikes every " + editor.money(u.cell(i, 7))
        }
    }

    RowLayout {
        Layout.fillWidth: true
        spacing: 4

        Button {
            objectName: "add"
            text: qsTr("Add")
            flat: true
            onClicked: { scenario.add(); editor.current = editor.underlyings.count - 1 }
        }
        Button {
            objectName: "remove"
            text: qsTr("Remove")
            flat: true
            enabled: editor.current >= 0 && editor.current < editor.underlyings.count
            onClicked: { scenario.remove(editor.current); editor.current = Math.min(editor.current, editor.underlyings.count - 1) }
        }
        Button {
            objectName: "up"
            text: qsTr("Up")
            flat: true
            enabled: editor.current > 0
            onClicked: { scenario.move(editor.current, -1); editor.current = editor.current - 1 }
        }
        Button {
            objectName: "down"
            text: qsTr("Down")
            flat: true
            enabled: editor.current >= 0 && editor.current < editor.underlyings.count - 1
            onClicked: { scenario.move(editor.current, 1); editor.current = editor.current + 1 }
        }
        Item { Layout.fillWidth: true }
        Button {
            objectName: "estimate"
            text: qsTr("Estimate...")
            flat: true
            enabled: editor.current >= 0 && editor.current < editor.underlyings.count
            onClicked: editor.estimateRequested(editor.current)
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Read a date,close history and adopt the volatility and drift it implies for the selected name")
        }
    }
}
