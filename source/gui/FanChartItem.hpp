#pragma once

#include "source/gui/ChartSeries.hpp"
#include "source/present/FanChartSeries.hpp"
#include <QColor>
#include <QPointF>
#include <QQuickPaintedItem>

namespace fin::gui
{
    // Paints the percentile band and the median of a chart series over geometry that the Qt-free
    // layer computed: the item only maps model coordinates onto its own pixels.
    class FanChartItem
        : public QQuickPaintedItem
    {
        Q_OBJECT
        Q_PROPERTY(fin::gui::ChartSeries* series READ Series WRITE SetSeries NOTIFY seriesChanged)
        Q_PROPERTY(QColor background READ Background WRITE SetBackground NOTIFY styleChanged)
        Q_PROPERTY(QColor ink READ Ink WRITE SetInk NOTIFY styleChanged)

    public:
        explicit FanChartItem(QQuickItem* parent = nullptr);

        ChartSeries* Series() const;
        void SetSeries(ChartSeries* series);
        QColor Background() const;
        void SetBackground(const QColor& colour);
        QColor Ink() const;
        void SetInk(const QColor& colour);

        void Present(const present::FanChartSeries& shown);
        QPointF Frame(const present::ChartPoint& point) const;

        void paint(QPainter* painter) override;

        static QColor Band();
        static QColor Median();

    signals:
        void seriesChanged();
        void styleChanged();

    private:
        const present::FanChartSeries& Shown() const;

        ChartSeries* series{ nullptr };
        present::FanChartSeries own;
        QColor background{ Qt::white };
        QColor ink{ 120, 120, 120 };
    };
}
