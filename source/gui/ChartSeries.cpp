#include "source/gui/ChartSeries.hpp"

namespace fin::gui
{
    ChartSeries::ChartSeries(QObject* parent)
        : QObject(parent)
        , series(present::TraceOf({}, {}))
    {}

    const present::FanChartSeries& ChartSeries::Shown() const
    {
        return series;
    }

    void ChartSeries::Present(const present::FanChartSeries& shown)
    {
        series = shown;

        emit changed();
    }

    bool ChartSeries::Empty() const
    {
        return present::IsEmpty(series);
    }
}
