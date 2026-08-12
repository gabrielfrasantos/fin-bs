#include "source/gui/FanChartWidget.hpp"
#include <QPainter>
#include <QPen>
#include <QPolygonF>
#include <QRectF>
#include <QString>
#include <algorithm>
#include <cmath>

namespace fin::gui
{
    namespace
    {
        constexpr int leftMargin = 84;
        constexpr int rightMargin = 12;
        constexpr int topMargin = 12;
        constexpr int bottomMargin = 28;
        constexpr int tickLength = 4;
        constexpr int leastSide = 32;

        QString Rounded(double value)
        {
            return QString::number(std::llround(value));
        }
    }

    FanChartWidget::FanChartWidget(QWidget* parent)
        : QWidget(parent)
        , series(present::TraceOf({}, {}))
    {
        setMinimumSize(320, 200);
        setAutoFillBackground(true);
    }

    QColor FanChartWidget::Band()
    {
        return QColor(196, 216, 240);
    }

    QColor FanChartWidget::Median()
    {
        return QColor(0, 0, 160);
    }

    void FanChartWidget::Present(const present::FanChartSeries& shown)
    {
        series = shown;
        update();
    }

    QPointF FanChartWidget::Frame(const present::ChartPoint& point) const
    {
        const auto plotWidth = std::max(leastSide, width() - leftMargin - rightMargin);
        const auto plotHeight = std::max(leastSide, height() - topMargin - bottomMargin);
        const auto across = (point.at - series.horizontal.lowest) / (series.horizontal.highest - series.horizontal.lowest);
        const auto up = (point.value - series.vertical.lowest) / (series.vertical.highest - series.vertical.lowest);

        return QPointF(leftMargin + across * plotWidth, topMargin + plotHeight - up * plotHeight);
    }

    void FanChartWidget::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.fillRect(rect(), Qt::white);

        const QRectF plot(QPointF(leftMargin, topMargin), Frame(present::ChartPoint{ series.horizontal.highest, series.vertical.lowest }));

        painter.setPen(QColor(120, 120, 120));
        painter.drawRect(plot);

        for (const auto tick : series.vertical.ticks)
        {
            const auto at = Frame(present::ChartPoint{ series.horizontal.lowest, tick });

            painter.drawLine(QPointF(at.x() - tickLength, at.y()), at);
            painter.drawText(QRectF(0, at.y() - 8, leftMargin - tickLength - 2, 16), Qt::AlignRight | Qt::AlignVCenter, Rounded(tick));
        }

        for (const auto tick : series.horizontal.ticks)
        {
            const auto at = Frame(present::ChartPoint{ tick, series.vertical.lowest });

            painter.drawLine(at, QPointF(at.x(), at.y() + tickLength));
            painter.drawText(QRectF(at.x() - 32, at.y() + tickLength, 64, bottomMargin - tickLength), Qt::AlignHCenter | Qt::AlignTop, Rounded(tick));
        }

        if (present::IsEmpty(series))
            return;

        QPolygonF band;

        for (const auto& point : series.upper)
            band << Frame(point);

        for (auto point = series.lower.rbegin(); point != series.lower.rend(); ++point)
            band << Frame(*point);

        painter.setPen(Qt::NoPen);
        painter.setBrush(Band());
        painter.drawPolygon(band);

        QPolygonF median;

        for (const auto& point : series.median)
            median << Frame(point);

        painter.setBrush(Qt::NoBrush);
        painter.setPen(QPen(Median(), 3));
        painter.drawPolyline(median);
    }
}
