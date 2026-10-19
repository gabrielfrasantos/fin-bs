#include "source/core/Date.hpp"
#include "source/core/Money.hpp"
#include "source/gui/FanChartItem.hpp"
#include "source/present/FanChartSeries.hpp"
#include <QImage>
#include <QPainter>
#include <QTest>
#include <vector>

namespace
{
    fin::core::Money Euro(double amount)
    {
        return fin::core::Money::FromAmount(amount, fin::core::Currency::Eur);
    }

    fin::present::FanChartSeries Trace()
    {
        const std::vector<fin::core::Date> boundaries{ fin::core::Date::FromYearMonthDay(2025, 1, 2), fin::core::Date::FromYearMonthDay(2025, 7, 3), fin::core::Date::FromYearMonthDay(2026, 1, 1) };
        const std::vector<fin::core::Money> equity{ Euro(100000.00), Euro(110000.00), Euro(105000.00) };

        return fin::present::TraceOf(equity, boundaries);
    }

    QImage Painted(fin::gui::FanChartItem& item)
    {
        QImage frame(static_cast<int>(item.width()), static_cast<int>(item.height()), QImage::Format_ARGB32);
        QPainter painter(&frame);
        item.paint(&painter);

        return frame;
    }
}

class TestFanChartItem
    : public QObject
{
    Q_OBJECT

private slots:

    void paints_the_median_where_the_geometry_says_it_is()
    {
        fin::gui::FanChartItem item;
        item.setWidth(640);
        item.setHeight(400);

        const auto series = Trace();
        item.Present(series);

        const auto frame = Painted(item);
        const auto at = item.Frame(series.median[1]);

        QCOMPARE(frame.pixelColor(qRound(at.x()), qRound(at.y())), fin::gui::FanChartItem::Median());
    }

    void an_empty_series_paints_an_empty_frame_rather_than_crashing()
    {
        fin::gui::FanChartItem item;
        item.setWidth(640);
        item.setHeight(400);
        item.Present(fin::present::TraceOf({}, {}));

        const auto frame = Painted(item);

        QVERIFY(!frame.isNull());
        QCOMPARE(frame.pixelColor(320, 200), QColor(Qt::white));
    }

    void follows_the_series_it_is_bound_to()
    {
        fin::gui::ChartSeries series;
        fin::gui::FanChartItem item;
        item.setWidth(640);
        item.setHeight(400);
        item.SetSeries(&series);

        QCOMPARE(series.Empty(), true);

        series.Present(Trace());

        QCOMPARE(series.Empty(), false);
        QCOMPARE(Painted(item).pixelColor(qRound(item.Frame(Trace().median[1]).x()), qRound(item.Frame(Trace().median[1]).y())), fin::gui::FanChartItem::Median());
    }
};

QTEST_MAIN(TestFanChartItem)

#include "TestFanChartItem.moc"
