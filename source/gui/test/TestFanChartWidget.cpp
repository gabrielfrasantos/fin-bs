#include "source/gui/FanChartWidget.hpp"
#include "source/present/FanChartSeries.hpp"
#include <QImage>
#include <QPainter>
#include <QTest>
#include <vector>

namespace
{
    using fin::core::Currency;
    using fin::core::Date;
    using fin::core::Money;

    Money Euro(double amount)
    {
        return Money::FromAmount(amount, Currency::Eur);
    }

    std::vector<Date> Boundaries()
    {
        return { Date::FromYearMonthDay(2026, 1, 5), Date::FromYearMonthDay(2026, 7, 6), Date::FromYearMonthDay(2027, 1, 4) };
    }

    std::vector<Money> Walked()
    {
        return { Euro(200000.00), Euro(210000.00), Euro(230000.00) };
    }

    QImage Painted(fin::gui::FanChartWidget& widget)
    {
        QImage frame(widget.size(), QImage::Format_ARGB32);
        frame.fill(Qt::transparent);
        widget.render(&frame);

        return frame;
    }
}

class TestFanChartWidget
    : public QObject
{
    Q_OBJECT

private slots:

    void draws_the_median_where_the_geometry_says_it_falls()
    {
        fin::gui::FanChartWidget widget;
        widget.resize(640, 400);
        const auto series = fin::present::TraceOf(Walked(), Boundaries());
        widget.Present(series);

        const auto frame = Painted(widget);
        const auto at = widget.Frame(series.median[1]);

        QCOMPARE(frame.pixelColor(qRound(at.x()), qRound(at.y())), fin::gui::FanChartWidget::Median());
    }

    void paints_an_empty_frame_when_nothing_has_been_run()
    {
        fin::gui::FanChartWidget widget;
        widget.resize(640, 400);
        widget.Present(fin::present::TraceOf({}, {}));

        const auto frame = Painted(widget);

        QVERIFY(!frame.isNull());
        QCOMPARE(frame.pixelColor(320, 200), QColor(Qt::white));
    }

    void keeps_drawing_after_it_is_resized()
    {
        fin::gui::FanChartWidget widget;
        widget.resize(640, 400);
        const auto series = fin::present::TraceOf(Walked(), Boundaries());
        widget.Present(series);

        widget.resize(400, 300);

        const auto frame = Painted(widget);
        const auto at = widget.Frame(series.median[1]);

        QVERIFY(at.x() < 400.0);
        QCOMPARE(frame.pixelColor(qRound(at.x()), qRound(at.y())), fin::gui::FanChartWidget::Median());
    }
};

QTEST_MAIN(TestFanChartWidget)

#include "TestFanChartWidget.moc"
