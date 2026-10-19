import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ScrollView {
    id: pane

    signal estimateRequested(int index)

    readonly property var scenario: workspace.scenario
    readonly property var numbers: Qt.locale("en_US")

    function money(cents) {
        return scenario.currency + " " + Number(cents / 100).toLocaleString(numbers, "f", 2)
    }

    function percent(basisPoints) {
        return Number(basisPoints / 100).toLocaleString(numbers, "f", 2) + " %"
    }

    function offends(name) {
        return scenario.offendingField === name
    }

    clip: true
    contentWidth: availableWidth

    ColumnLayout {
        width: pane.availableWidth
        spacing: 10

        Rectangle {
            Layout.fillWidth: true
            Layout.margins: 8
            visible: !scenario.wellFormed
            implicitHeight: complaint.implicitHeight + 16
            radius: 4
            color: Qt.rgba(0.9, 0.2, 0.2, 0.12)
            border.color: Material.color(Material.Red)

            Label {
                id: complaint
                objectName: "complaint"
                anchors.fill: parent
                anchors.margins: 8
                wrapMode: Text.WordWrap
                text: scenario.complaint
                color: Material.color(Material.Red)
            }
        }

        Section {
            Layout.margins: 8
            Layout.topMargin: 0
            title: qsTr("Capital and horizon")
            blurb: qsTr("What is put to work, from when, and for how many cycles of how many days.")

            RowLayout {
                Layout.fillWidth: true
                Label { Layout.preferredWidth: 190; text: qsTr("Name") }
                TextField {
                    objectName: "scenario"
                    Layout.fillWidth: true
                    text: scenario.name
                    placeholderText: qsTr("What this scenario is about")
                    onTextChanged: if (!activeFocus) cursorPosition = 0
                    onEditingFinished: scenario.name = text
                }
            }

            Field {
                caption: qsTr("Capital")
                fieldName: "capital_cents"
                unit: qsTr("cents")
                value: scenario.capitalCents
                hint: "= " + pane.money(scenario.capitalCents)
                offending: pane.offends("capital_cents")
                onEdited: function(value) { scenario.capitalCents = value }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { Layout.preferredWidth: 190; text: qsTr("Start date") }
                TextField {
                    objectName: "start_date"
                    Layout.preferredWidth: 170
                    text: scenario.start.toLocaleDateString(pane.numbers, "yyyy-MM-dd")
                    inputMask: "9999-99-99"
                    onEditingFinished: {
                        const parsed = Date.fromLocaleDateString(pane.numbers, text, "yyyy-MM-dd")
                        if (!isNaN(parsed.getTime()))
                            scenario.start = parsed
                    }
                }
                Label { text: qsTr("year-month-day"); opacity: 0.7 }
            }

            Field {
                caption: qsTr("Cycles")
                fieldName: "cycles"
                unit: ""
                to: 10000
                value: scenario.cycles
                hint: qsTr("%1 days in all").arg(scenario.cycles * scenario.daysPerCycle)
                offending: pane.offends("cycles")
                onEdited: function(value) { scenario.cycles = value }
            }

            Field {
                caption: qsTr("Days a cycle")
                fieldName: "days_per_cycle"
                unit: qsTr("days")
                to: 10000
                value: scenario.daysPerCycle
                hint: qsTr("about %1 weeks").arg(Math.round(scenario.daysPerCycle / 7))
                offending: pane.offends("days_per_cycle")
                onEdited: function(value) { scenario.daysPerCycle = value }
            }

            Field {
                caption: qsTr("Observation before expiry")
                fieldName: "observation_days_before_expiry"
                unit: qsTr("days")
                to: 10000
                value: scenario.observationDaysBeforeExpiry
                hint: qsTr("the price is read this long before expiry")
                offending: pane.offends("observation_days_before_expiry")
                onEdited: function(value) { scenario.observationDaysBeforeExpiry = value }
            }
        }

        Section {
            Layout.margins: 8
            Layout.topMargin: 0
            title: qsTr("Strategy levers")
            blurb: qsTr("How far out of the money to write, how much cash to keep back, and when a position is rolled instead of left to expire.")

            Field {
                caption: qsTr("Target delta")
                fieldName: "target_delta_basis_points"
                unit: qsTr("bp")
                to: 1000000
                stepSize: 100
                value: scenario.targetDeltaBasisPoints
                hint: "= " + pane.percent(scenario.targetDeltaBasisPoints)
                offending: pane.offends("target_delta_basis_points")
                onEdited: function(value) { scenario.targetDeltaBasisPoints = value }
            }

            Field {
                caption: qsTr("Cash buffer")
                fieldName: "cash_buffer_basis_points"
                unit: qsTr("bp")
                to: 1000000
                stepSize: 100
                value: scenario.cashBufferBasisPoints
                hint: "= " + pane.percent(scenario.cashBufferBasisPoints) + qsTr(" of capital kept back")
                offending: pane.offends("cash_buffer_basis_points")
                onEdited: function(value) { scenario.cashBufferBasisPoints = value }
            }

            Field {
                caption: qsTr("Roll when delta reaches")
                fieldName: "roll_delta_limit_basis_points"
                unit: qsTr("bp")
                to: 1000000
                stepSize: 100
                value: scenario.rollDeltaLimitBasisPoints
                hint: "= " + pane.percent(scenario.rollDeltaLimitBasisPoints)
                offending: pane.offends("roll_delta_limit_basis_points")
                onEdited: function(value) { scenario.rollDeltaLimitBasisPoints = value }
            }

            Field {
                caption: qsTr("Roll this close to expiry")
                fieldName: "roll_days_to_expiry"
                unit: qsTr("days")
                to: 10000
                value: scenario.rollDaysToExpiry
                hint: qsTr("time trigger")
                offending: pane.offends("roll_days_to_expiry")
                onEdited: function(value) { scenario.rollDaysToExpiry = value }
            }

            Field {
                caption: qsTr("Roll once this much is earned")
                fieldName: "roll_profit_capture_basis_points"
                unit: qsTr("bp")
                to: 1000000
                stepSize: 100
                value: scenario.rollProfitCaptureBasisPoints
                hint: "= " + pane.percent(scenario.rollProfitCaptureBasisPoints) + qsTr(" of the premium")
                offending: pane.offends("roll_profit_capture_basis_points")
                onEdited: function(value) { scenario.rollProfitCaptureBasisPoints = value }
            }

            Field {
                caption: qsTr("Risk-free rate")
                fieldName: "risk_free_rate_basis_points"
                unit: qsTr("bp a year")
                from: -1000000
                to: 1000000
                stepSize: 25
                value: scenario.riskFreeRateBasisPoints
                hint: "= " + pane.percent(scenario.riskFreeRateBasisPoints) + qsTr(" a year")
                offending: pane.offends("risk_free_rate_basis_points")
                onEdited: function(value) { scenario.riskFreeRateBasisPoints = value }
            }
        }

        Section {
            Layout.margins: 8
            Layout.topMargin: 0
            title: qsTr("Wealth tax and simulation")
            blurb: qsTr("The day each year the holdings are assessed, how many futures a simulation draws, and the seed that makes a run repeatable.")
            collapsible: true

            RowLayout {
                Layout.fillWidth: true
                Label { Layout.preferredWidth: 190; text: qsTr("Assessed every year") }
                Switch {
                    objectName: "assessment"
                    checked: scenario.assessed
                    onToggled: scenario.assessed = checked
                }
                Label {
                    Layout.fillWidth: true
                    text: scenario.assessed ? qsTr("on the reference date below") : qsTr("no wealth tax in this run")
                    opacity: 0.85
                    font.italic: true
                }
            }

            Field {
                caption: qsTr("Assessment month")
                fieldName: "assessment_month"
                enabled: scenario.assessed
                from: 1
                to: 12
                value: scenario.assessmentMonth
                offending: pane.offends("assessment_month")
                onEdited: function(value) { scenario.assessmentMonth = value }
            }

            Field {
                caption: qsTr("Assessment day")
                fieldName: "assessment_day"
                enabled: scenario.assessed
                from: 1
                to: 31
                value: scenario.assessmentDay
                offending: pane.offends("assessment_day")
                onEdited: function(value) { scenario.assessmentDay = value }
            }

            Field {
                caption: qsTr("Paths")
                fieldName: "paths"
                unit: qsTr("futures")
                to: 1000000
                stepSize: 100
                value: scenario.paths
                hint: scenario.paths < 1000 ? qsTr("quick look; percentiles are rough") : qsTr("percentiles settle from a few thousand")
                offending: pane.offends("paths")
                onEdited: function(value) { scenario.paths = value }
            }

            Field {
                caption: qsTr("Seed")
                fieldName: "seed"
                value: scenario.seed
                hint: qsTr("the same seed repeats the same futures")
                offending: pane.offends("seed")
                onEdited: function(value) { scenario.seed = value }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { Layout.preferredWidth: 190; text: qsTr("Tax rules file") }
                TextField {
                    objectName: "tax_configuration"
                    Layout.fillWidth: true
                    text: scenario.taxConfiguration
                    onEditingFinished: scenario.taxConfiguration = text
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Label { Layout.preferredWidth: 190; text: qsTr("Broker fees file") }
                TextField {
                    objectName: "broker_configuration"
                    Layout.fillWidth: true
                    text: scenario.brokerConfiguration
                    onEditingFinished: scenario.brokerConfiguration = text
                }
            }
        }

        Section {
            Layout.margins: 8
            Layout.topMargin: 0
            title: qsTr("Underlyings")
            blurb: qsTr("The names in the order collateral is offered to them. Spot and strike step in cents; rates in basis points a year.")

            UnderlyingsEditor {
                Layout.fillWidth: true
                onEstimateRequested: function(index) { pane.estimateRequested(index) }
            }
        }
    }
}
