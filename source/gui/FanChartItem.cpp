#include "source/gui/FanChartItem.hpp"
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

    FanChartItem::FanChartItem(QQuickItem* parent)
        : QQuickPaintedItem(parent)
        , own(present::TraceOf({}, {}))
    {
        setImplicitWidth(320);
        setImplicitHeight(200);
    }

    QColor FanChartItem::Band()
    {
        return QColor(196, 216, 240);
    }

    QColor FanChartItem::Median()
    {
        return QColor(0, 0, 160);
    }

    ChartSeries* FanChartItem::Series() const
    {
        return series;
    }

    void FanChartItem::SetSeries(ChartSeries* shown)
    {
        if (series == shown)
            return;

        if (series != nullptr)
            disconnect(series, nullptr, this, nullptr);

        series = shown;

        if (series != nullptr)
            connect(series, &ChartSeries::changed, this, [this]
                {
                    update();
                });

        emit seriesChanged();
        update();
    }

    QColor FanChartItem::Background() const
    {
        return background;
    }

    void FanChartItem::SetBackground(const QColor& colour)
    {
        background = colour;
        emit styleChanged();
        update();
    }

    QColor FanChartItem::Ink() const
    {
        return ink;
    }

    void FanChartItem::SetInk(const QColor& colour)
    {
        ink = colour;
        emit styleChanged();
        update();
    }

    void FanChartItem::Present(const present::FanChartSeries& shown)
    {
        own = shown;
        update();
    }

    const present::FanChartSeries& FanChartItem::Shown() const
    {
        return series != nullptr ? series->Shown() : own;
    }

    QPointF FanChartItem::Frame(const present::ChartPoint& point) const
    {
        const auto& shown = Shown();
        const auto plotWidth = std::max<double>(leastSide, width() - leftMargin - rightMargin);
        const auto plotHeight = std::max<double>(leastSide, height() - topMargin - bottomMargin);
        const auto across = (point.at - shown.horizontal.lowest) / (shown.horizontal.highest - shown.horizontal.lowest);
        const auto up = (point.value - shown.vertical.lowest) / (shown.vertical.highest - shown.vertical.lowest);

        return QPointF(leftMargin + across * plotWidth, topMargin + plotHeight - up * plotHeight);
    }

    void FanChartItem::paint(QPainter* painter)
    {
        const auto& shown = Shown();

        painter->setRenderHint(QPainter::Antialiasing, false);
        painter->fillRect(QRectF(0, 0, width(), height()), background);

        const QRectF plot(QPointF(leftMargin, topMargin), Frame(present::ChartPoint{ shown.horizontal.highest, shown.vertical.lowest }));

        painter->setPen(ink);
        painter->drawRect(plot);

        for (const auto tick : shown.vertical.ticks)
        {
            const auto at = Frame(present::ChartPoint{ shown.horizontal.lowest, tick });

            painter->drawLine(QPointF(at.x() - tickLength, at.y()), at);
            painter->drawText(QRectF(0, at.y() - 8, leftMargin - tickLength - 2, 16), Qt::AlignRight | Qt::AlignVCenter, Rounded(tick));
        }

        for (const auto tick : shown.horizontal.ticks)
        {
            const auto at = Frame(present::ChartPoint{ tick, shown.vertical.lowest });

            painter->drawLine(at, QPointF(at.x(), at.y() + tickLength));
            painter->drawText(QRectF(at.x() - 32, at.y() + tickLength, 64, bottomMargin - tickLength), Qt::AlignHCenter | Qt::AlignTop, Rounded(tick));
        }

        if (present::IsEmpty(shown))
            return;

        QPolygonF band;

        for (const auto& point : shown.upper)
            band << Frame(point);

        for (auto point = shown.lower.rbegin(); point != shown.lower.rend(); ++point)
            band << Frame(*point);

        painter->setPen(Qt::NoPen);
        painter->setBrush(Band());
        painter->drawPolygon(band);

        QPolygonF median;

        for (const auto& point : shown.median)
            median << Frame(point);

        painter->setBrush(Qt::NoBrush);
        painter->setPen(QPen(Median(), 3));
        painter->drawPolyline(median);
    }
}
