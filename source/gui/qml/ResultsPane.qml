import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import Fin 1.0

ColumnLayout {
    id: pane

    readonly property var run: workspace.run

    spacing: 8

    Flow {
        Layout.fillWidth: true
        Layout.margins: 8
        Layout.bottomMargin: 0
        spacing: 8
        visible: run.highlights.length > 0

        Repeater {
            model: run.highlights

            Frame {
                required property var modelData
                Material.elevation: 1
                padding: 10

                ColumnLayout {
                    spacing: 2

                    Label {
                        text: modelData.value
                        font.pixelSize: 20
                        font.weight: Font.DemiBold
                    }

                    Label {
                        text: modelData.label
                        font.pixelSize: 12
                        opacity: 0.7
                    }
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.margins: 8
        Layout.bottomMargin: 0
        visible: run.highlights.length === 0 && !run.running
        text: qsTr("Project for one path on the stated drifts, simulate for the spread of outcomes, or search for the levers that do best under the risk limits.")
        wrapMode: Text.WordWrap
        opacity: 0.6
    }

    TabBar {
        id: tabs
        objectName: "resultsTabs"
        Layout.fillWidth: true

        TabButton { text: qsTr("Summary") }
        TabButton { text: qsTr("Fan chart") }
        TabButton { text: qsTr("Cycles") }
        TabButton { text: qsTr("Ledger") }
    }

    StackLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        currentIndex: tabs.currentIndex

        TablePane {
            objectName: "summary"
            table: run.summary
            emptyText: run.kind === "projection" && run.hasResult ? qsTr("A projection has no distribution; see the fan chart and the cycles.") : qsTr("Nothing to show until a simulation or a search has been run.")
        }

        FanChart {
            objectName: "chart"
            series: run.chart
            background: Material.background
            ink: Material.foreground

            Label {
                anchors.centerIn: parent
                visible: run.chart.empty
                text: qsTr("The chart shows the 5th to 95th percentile band and the median of equity at every cycle boundary.")
                opacity: 0.6
            }
        }

        TablePane {
            objectName: "cycles"
            table: run.cycles
        }

        TablePane {
            objectName: "ledger"
            table: run.ledger
        }
    }

    Label {
        objectName: "disclaimer"
        Layout.fillWidth: true
        Layout.margins: 8
        Layout.topMargin: 0
        wrapMode: Text.WordWrap
        font.pixelSize: 12
        opacity: 0.75
        text: qsTr("These are estimates, not forecasts. The underlyings are drawn independently of one another, so every drawdown and assignment rate shown here is optimistic; prices are drawn from a lognormal model that understates what a market does at its worst.")
    }
}
