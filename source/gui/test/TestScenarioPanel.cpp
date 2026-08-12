#include "source/gui/ScenarioPanel.hpp"
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>
#include <QTest>
#include <string>
#include <vector>

namespace
{
    std::vector<std::string> Closes()
    {
        return { "date,close",
            "2025-01-02,100.00",
            "2025-01-03,101.50",
            "2025-01-06,99.25",
            "2025-01-07,102.75",
            "2025-01-08,101.00",
            "2025-01-09,103.50",
            "2025-01-10,102.25" };
    }

    fin::present::UnderlyingRow Row(std::string ticker)
    {
        return fin::present::UnderlyingRow{ .ticker = std::move(ticker), .spotCents = 9500, .driftBasisPoints = 600, .volatilityBasisPoints = 2200, .dividendYieldBasisPoints = 180, .strikeIncrementCents = 100 };
    }
}

class TestScenarioPanel
    : public QObject
{
    Q_OBJECT

private slots:

    void typing_into_a_field_reaches_the_document()
    {
        fin::gui::ScenarioPanel panel;

        panel.findChild<QLineEdit*>("scenario")->setText("A book of my own");
        panel.findChild<QSpinBox*>("capital_cents")->setValue(12345600);

        QCOMPARE(QString::fromStdString(panel.Document().Fields().name), QString("A book of my own"));
        QCOMPARE(panel.Document().Fields().capitalCents, 12345600);
    }

    void a_field_outside_its_range_is_named_rather_than_silently_clamped()
    {
        fin::gui::ScenarioPanel panel;

        panel.findChild<QSpinBox*>("target_delta_basis_points")->setValue(15000);

        QVERIFY(panel.Document().Offending().has_value());
        QCOMPARE(QString::fromStdString(panel.Document().Offending()->field), QString("target_delta_basis_points"));
    }

    void adding_an_underlying_puts_a_row_in_the_table()
    {
        fin::gui::ScenarioPanel panel;
        auto* table = panel.findChild<QTableWidget*>("underlyings");

        QCOMPARE(table->rowCount(), 0);

        panel.Add();

        QCOMPARE(table->rowCount(), 1);
        QCOMPARE(panel.Document().Underlyings().size(), std::size_t(1));
    }

    void editing_a_cell_reaches_the_row_it_belongs_to()
    {
        fin::gui::ScenarioPanel panel;
        panel.Add();

        auto* table = panel.findChild<QTableWidget*>("underlyings");
        table->item(0, 0)->setText("ACME");
        table->item(0, 1)->setText("6789");

        QCOMPARE(QString::fromStdString(panel.Document().Underlyings()[0].ticker), QString("ACME"));
        QCOMPARE(panel.Document().Underlyings()[0].spotCents, 6789);
    }

    void moving_a_row_reorders_the_book_the_collateral_is_offered_to()
    {
        fin::gui::ScenarioPanel panel;
        auto document = panel.Document();
        document.Append(Row("ACME"));
        document.Append(Row("BETA"));
        panel.Present(document);

        panel.findChild<QTableWidget*>("underlyings")->selectRow(1);
        panel.MoveBy(-1);

        QCOMPARE(QString::fromStdString(panel.Document().Underlyings()[0].ticker), QString("BETA"));
        QCOMPARE(QString::fromStdString(panel.Document().Underlyings()[1].ticker), QString("ACME"));
    }

    void adopting_an_estimate_changes_exactly_the_volatility_and_the_drift()
    {
        fin::gui::ScenarioPanel panel;
        auto document = panel.Document();
        document.Append(Row("ACME"));
        panel.Present(document);

        const auto before = panel.Document().Underlyings()[0];

        QVERIFY(panel.Adopt(0, Closes()));

        const auto after = panel.Document().Underlyings()[0];

        QVERIFY(after.volatilityBasisPoints != before.volatilityBasisPoints);
        QCOMPARE(after.spotCents, before.spotCents);
        QCOMPARE(after.dividendYieldBasisPoints, before.dividendYieldBasisPoints);
        QCOMPARE(after.strikeIncrementCents, before.strikeIncrementCents);
        QCOMPARE(panel.findChild<QTableWidget*>("underlyings")->item(0, 3)->text().toInt(), after.volatilityBasisPoints);
    }

    void reports_a_price_history_it_cannot_read_rather_than_adopting_a_guess()
    {
        fin::gui::ScenarioPanel panel;
        auto document = panel.Document();
        document.Append(Row("ACME"));
        panel.Present(document);

        QVERIFY(!panel.Adopt(0, { "date,close", "2025-01-02,not a price" }));
        QVERIFY(!panel.Complaint().isEmpty());
    }
};

QTEST_MAIN(TestScenarioPanel)

#include "TestScenarioPanel.moc"
