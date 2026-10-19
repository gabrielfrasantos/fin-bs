import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ColumnLayout {
    id: field

    property string caption
    property string unit
    property string hint
    property int value: 0
    property int from: 0
    property int to: 2147483647
    property int stepSize: 1
    property bool offending: false
    property string fieldName

    signal edited(int value)

    Layout.fillWidth: true
    spacing: 0

    RowLayout {
        Layout.fillWidth: true
        spacing: 8

        Label {
            Layout.preferredWidth: 170
            text: field.caption
            wrapMode: Text.WordWrap
            color: field.offending ? Material.color(Material.Red) : Material.foreground
        }

        SpinBox {
            id: box
            objectName: field.fieldName
            Layout.preferredWidth: 210
            editable: true
            from: field.from
            to: field.to
            stepSize: field.stepSize
            value: field.value
            font.pixelSize: 14
            onValueModified: field.edited(value)
            Material.foreground: field.offending ? Material.color(Material.Red) : undefined
        }

        Label {
            Layout.fillWidth: true
            text: field.unit
            opacity: 0.7
            elide: Text.ElideRight
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.leftMargin: 178
        visible: field.hint.length > 0
        text: field.hint
        elide: Text.ElideRight
        opacity: 0.8
        font.italic: true
        font.pixelSize: 12
    }
}
