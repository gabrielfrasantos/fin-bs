import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    objectName: "window"

    // The theme is a property so that a tour, a test or a user can flip it.
    property bool dark: false
    readonly property var scenario: workspace.scenario
    readonly property var run: workspace.run

    width: 1440
    height: 900
    minimumWidth: 1024
    minimumHeight: 640
    visible: true
    title: qsTr("Options income and compounding estimator")

    Material.theme: dark ? Material.Dark : Material.Light
    Material.primary: Material.Indigo
    Material.accent: Material.Indigo

    header: ToolBar {
        // The brand bar keeps the same colours in both themes, so its controls always read.
        Material.theme: Material.Dark
        Material.elevation: 2
        background: Rectangle { color: "#3f51b5" }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 4

            ToolButton {
                objectName: "open"
                text: qsTr("Open")
                icon.name: "document-open"
                onClicked: openDialog.open()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Open a scenario file (Ctrl+O)")
            }

            ToolButton {
                objectName: "save"
                text: qsTr("Save as")
                enabled: scenario.wellFormed
                onClicked: saveDialog.open()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Save the scenario as a file (Ctrl+S)")
            }

            ToolButton {
                objectName: "export"
                text: qsTr("Export results")
                enabled: run.hasResult
                onClicked: exportDialog.open()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Write the cycle table and the ledger as comma separated values")
            }

            ToolSeparator {}

            Label {
                Layout.fillWidth: true
                elide: Text.ElideMiddle
                text: scenario.name.length > 0 ? scenario.name : qsTr("Untitled scenario")
                font.pixelSize: 16
                font.weight: Font.Medium
            }

            Button {
                objectName: "project"
                text: qsTr("Project")
                highlighted: true
                enabled: run.runnable
                onClicked: run.project()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("One run on the stated drifts: every cycle, every cashflow (Ctrl+R)")
            }

            Button {
                objectName: "simulate"
                text: qsTr("Simulate")
                enabled: run.runnable
                onClicked: run.simulate()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Many random futures: the spread of outcomes, not one path")
            }

            Button {
                objectName: "search"
                text: qsTr("Search")
                enabled: run.runnable
                onClicked: run.search()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Look for the delta, buffer and sizing that do best under the risk limits")
            }

            Button {
                objectName: "cancel"
                text: qsTr("Cancel")
                enabled: run.running
                onClicked: run.cancel()
            }

            ToolSeparator {}

            Switch {
                objectName: "theme"
                text: qsTr("Dark")
                checked: window.dark
                onToggled: window.dark = checked
            }
        }
    }

    footer: ToolBar {
        Material.theme: Material.Dark
        Material.elevation: 0
        background: Rectangle { color: "#3f51b5" }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 12

            Label {
                objectName: "status"
                Layout.fillWidth: true
                elide: Text.ElideRight
                text: !scenario.wellFormed ? scenario.complaint : (workspace.message.length > 0 ? workspace.message : run.status)
                color: !scenario.wellFormed || workspace.message.length > 0 ? Material.color(Material.Red, Material.Shade200) : "white"
            }

            Label {
                visible: run.running && run.total > 0
                text: run.completed + " / " + run.total
            }

            ProgressBar {
                objectName: "progress"
                Layout.preferredWidth: 240
                visible: run.running
                indeterminate: run.total <= 0
                from: 0
                to: Math.max(1, run.total)
                value: run.completed
            }
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        ScenarioPane {
            objectName: "scenarioPane"
            SplitView.preferredWidth: 640
            SplitView.minimumWidth: 420
            onEstimateRequested: function(index) {
                pricesDialog.row = index
                pricesDialog.open()
            }
        }

        ResultsPane {
            SplitView.fillWidth: true
            SplitView.minimumWidth: 480
        }
    }

    FileDialog {
        id: openDialog
        title: qsTr("Open a scenario")
        currentFolder: workspace.scenariosFolder()
        nameFilters: [qsTr("Scenarios (*.json)")]
        onAccepted: workspace.open(selectedFile)
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Save the scenario")
        fileMode: FileDialog.SaveFile
        currentFolder: workspace.scenariosFolder()
        nameFilters: [qsTr("Scenarios (*.json)")]
        onAccepted: workspace.save(selectedFile)
    }

    FileDialog {
        id: exportDialog
        title: qsTr("Export the results")
        fileMode: FileDialog.SaveFile
        nameFilters: [qsTr("Comma separated values (*.csv)")]
        onAccepted: workspace.exportResults(selectedFile)
    }

    FileDialog {
        id: pricesDialog
        property int row: -1
        title: qsTr("Open a price history")
        nameFilters: [qsTr("Comma separated values (*.csv)")]
        onAccepted: workspace.estimate(row, selectedFile)
    }

    Shortcut { sequence: StandardKey.Open; onActivated: openDialog.open() }
    Shortcut { sequence: StandardKey.SaveAs; onActivated: if (scenario.wellFormed) saveDialog.open() }
    Shortcut { sequence: "Ctrl+R"; onActivated: if (run.runnable) run.project() }
    Shortcut { sequence: StandardKey.Cancel; onActivated: if (run.running) run.cancel() }
}
