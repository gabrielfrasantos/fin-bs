#include "source/gui/ScenarioModel.hpp"
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
        return fin::present::UnderlyingRow{ .ticker = std::move(ticker), .spotCents = 9500, .driftBasisPoints = 600, .volatilityBasisPoints = 2200, .impliedVolatilityBasisPoints = 2400, .dividendYieldBasisPoints = 180, .dividendWithholdingBasisPoints = 1500, .strikeIncrementCents = 100 };
    }
}

class TestScenarioModel
    : public QObject
{
    Q_OBJECT

private slots:

    void writing_a_property_reaches_the_document()
    {
        fin::gui::ScenarioModel model;

        model.SetName("A book of my own");
        model.SetCapitalCents(12345600);

        QCOMPARE(QString::fromStdString(model.Document().Fields().name), QString("A book of my own"));
        QCOMPARE(model.Document().Fields().capitalCents, 12345600);
    }

    void a_field_outside_its_range_is_named_rather_than_silently_clamped()
    {
        fin::gui::ScenarioModel model;

        model.SetTargetDeltaBasisPoints(15000);

        QCOMPARE(model.WellFormed(), false);
        QCOMPARE(model.OffendingField(), QString("target_delta_basis_points"));
        QVERIFY(model.Complaint().contains("target_delta_basis_points"));
    }

    void adding_an_underlying_puts_a_row_in_the_model()
    {
        fin::gui::ScenarioModel model;

        QCOMPARE(model.Underlyings()->Count(), 0);

        model.add();

        QCOMPARE(model.Underlyings()->Count(), 1);
        QCOMPARE(model.Document().Underlyings().size(), std::size_t(1));
    }

    void editing_a_cell_reaches_the_row_it_belongs_to()
    {
        fin::gui::ScenarioModel model;
        model.add();

        QVERIFY(model.Underlyings()->setCell(0, 0, "ACME"));
        QVERIFY(model.Underlyings()->setCell(0, 1, "6789"));
        QVERIFY(!model.Underlyings()->setCell(0, 1, "not a number"));

        QCOMPARE(QString::fromStdString(model.Document().Underlyings()[0].ticker), QString("ACME"));
        QCOMPARE(model.Document().Underlyings()[0].spotCents, 6789);
        QCOMPARE(model.Underlyings()->cell(0, 1).toInt(), 6789);
    }

    void moving_a_row_reorders_the_book_the_collateral_is_offered_to()
    {
        fin::gui::ScenarioModel model;
        auto document = model.Document();
        document.Append(Row("ACME"));
        document.Append(Row("BETA"));
        model.Present(document);

        model.move(1, -1);

        QCOMPARE(QString::fromStdString(model.Document().Underlyings()[0].ticker), QString("BETA"));
        QCOMPARE(QString::fromStdString(model.Document().Underlyings()[1].ticker), QString("ACME"));
    }

    void adopting_an_estimate_changes_exactly_the_volatility_and_the_drift()
    {
        fin::gui::ScenarioModel model;
        auto document = model.Document();
        document.Append(Row("ACME"));
        model.Present(document);

        const auto before = model.Document().Underlyings()[0];

        QVERIFY(model.Adopt(0, Closes()));

        const auto after = model.Document().Underlyings()[0];

        QVERIFY(after.volatilityBasisPoints != before.volatilityBasisPoints);
        QCOMPARE(after.spotCents, before.spotCents);
        QCOMPARE(after.dividendYieldBasisPoints, before.dividendYieldBasisPoints);
        QCOMPARE(after.strikeIncrementCents, before.strikeIncrementCents);
        QCOMPARE(model.Underlyings()->cell(0, 3).toInt(), after.volatilityBasisPoints);
    }

    void reports_a_price_history_it_cannot_read_rather_than_adopting_a_guess()
    {
        fin::gui::ScenarioModel model;
        auto document = model.Document();
        document.Append(Row("ACME"));
        model.Present(document);

        QVERIFY(!model.Adopt(0, { "date,close", "2025-01-02,not a price" }));
        QVERIFY(!model.PriceComplaint().isEmpty());
    }
};

QTEST_MAIN(TestScenarioModel)

#include "TestScenarioModel.moc"
