#pragma once

#include "source/present/FanChartSeries.hpp"
#include <QObject>

namespace fin::gui
{
    class ChartSeries
        : public QObject
    {
        Q_OBJECT
        Q_PROPERTY(bool empty READ Empty NOTIFY changed)

    public:
        explicit ChartSeries(QObject* parent = nullptr);

        const present::FanChartSeries& Shown() const;
        void Present(const present::FanChartSeries& series);
        bool Empty() const;

    signals:
        void changed();

    private:
        present::FanChartSeries series;
    };
}
