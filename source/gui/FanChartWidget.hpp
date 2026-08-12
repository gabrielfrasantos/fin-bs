#pragma once

#include "source/present/FanChartSeries.hpp"
#include <QColor>
#include <QPointF>
#include <QWidget>

namespace fin::gui
{
    class FanChartWidget
        : public QWidget
    {
        Q_OBJECT

    public:
        explicit FanChartWidget(QWidget* parent = nullptr);

        void Present(const present::FanChartSeries& shown);
        QPointF Frame(const present::ChartPoint& point) const;

        static QColor Band();
        static QColor Median();

    protected:
        void paintEvent(QPaintEvent* event) override;

    private:
        present::FanChartSeries series;
    };
}
