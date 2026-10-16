#include "source/io/PriceCsvLoader.hpp"
#include <algorithm>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace fin::io
{
    namespace
    {
        constexpr std::string_view columnNames = "date,close";
        constexpr std::size_t writtenDateSize = 10;
        constexpr std::size_t mostWholeDigits = 15;
        constexpr std::int64_t centsPerUnit = 100;

        std::string_view WithoutLineEnding(const std::string& line)
        {
            std::string_view text(line);

            while (!text.empty() && (text.back() == '\r' || text.back() == '\n'))
                text.remove_suffix(1);

            return text;
        }

        bool IsWrittenInDigits(std::string_view text)
        {
            return !text.empty() && std::all_of(text.begin(), text.end(), [](char character)
                                        {
                                            return character >= '0' && character <= '9';
                                        });
        }

        std::int64_t NumberOf(std::string_view digits)
        {
            std::int64_t number = 0;

            for (const auto digit : digits)
                number = number * 10 + (digit - '0');

            return number;
        }

        core::Date DateFrom(std::string_view text)
        {
            if (text.size() != writtenDateSize || text[4] != '-' || text[7] != '-' || !IsWrittenInDigits(text.substr(0, 4)) || !IsWrittenInDigits(text.substr(5, 2)) || !IsWrittenInDigits(text.substr(8, 2)))
                throw std::runtime_error("Date " + std::string(text) + " is not written as a year, a month and a day");

            const auto year = static_cast<std::int32_t>(NumberOf(text.substr(0, 4)));
            const auto month = static_cast<std::uint32_t>(NumberOf(text.substr(5, 2)));
            const auto day = static_cast<std::uint32_t>(NumberOf(text.substr(8, 2)));

            if (!core::Date::IsValid(year, month, day))
                throw std::runtime_error("Date " + std::string(text) + " is not a day of the calendar");

            return core::Date::FromYearMonthDay(year, month, day);
        }

        bool RoundsUp(std::string_view beyondTheCent, std::int64_t cents)
        {
            if (beyondTheCent.empty() || beyondTheCent.front() < '5')
                return false;

            if (beyondTheCent.front() > '5')
                return true;

            const auto beyondTheHalf = beyondTheCent.substr(1);

            return std::any_of(beyondTheHalf.begin(), beyondTheHalf.end(), [](char digit)
                       {
                           return digit != '0';
                       }) ||
                   cents % 2 != 0;
        }

        std::int64_t CentsFrom(std::string_view text)
        {
            const auto point = text.find('.');
            const auto whole = text.substr(0, point);
            const auto fraction = point == std::string_view::npos ? std::string_view() : text.substr(point + 1);

            if (!IsWrittenInDigits(whole) || (point != std::string_view::npos && !IsWrittenInDigits(fraction)))
                throw std::runtime_error("Close " + std::string(text) + " is not an amount");

            if (whole.size() > mostWholeDigits)
                throw std::runtime_error("Close " + std::string(text) + " is larger than an amount this project holds");

            const auto tenths = fraction.size() > 0 ? fraction[0] - '0' : 0;
            const auto hundredths = fraction.size() > 1 ? fraction[1] - '0' : 0;
            const auto cents = NumberOf(whole) * centsPerUnit + tenths * 10 + hundredths;

            return RoundsUp(fraction.size() > 2 ? fraction.substr(2) : std::string_view(), cents) ? cents + 1 : cents;
        }

        struct Closes
        {
            std::vector<core::Date> dates;
            std::vector<core::Money> closes;
        };

        Closes ClosesIn(const std::vector<std::string>& document, core::Currency currency)
        {
            Closes read;
            bool columnsNamed = false;
            std::size_t row = 0;

            for (const auto& line : document)
            {
                ++row;
                const auto text = WithoutLineEnding(line);

                if (text.empty())
                    continue;

                try
                {
                    if (!columnsNamed)
                    {
                        if (text != columnNames)
                            throw std::runtime_error("Columns are named " + std::string(text) + " rather than " + std::string(columnNames));

                        columnsNamed = true;
                        continue;
                    }

                    const auto separator = text.find(',');

                    if (separator == std::string_view::npos || text.find(',', separator + 1) != std::string_view::npos)
                        throw std::runtime_error("Row " + std::string(text) + " does not hold a date and a close");

                    const auto date = DateFrom(text.substr(0, separator));
                    const auto close = core::Money::FromCents(CentsFrom(text.substr(separator + 1)), currency);

                    if (close.Cents() <= 0)
                        throw std::runtime_error("Close " + std::string(text.substr(separator + 1)) + " is not a positive amount");

                    if (!read.dates.empty() && date == read.dates.back())
                        throw std::runtime_error("Date " + std::string(text.substr(0, separator)) + " is closed twice");

                    if (!read.dates.empty() && date < read.dates.back())
                        throw std::runtime_error("Date " + std::string(text.substr(0, separator)) + " comes before the date above it");

                    read.dates.push_back(date);
                    read.closes.push_back(close);
                }
                catch (const std::exception& error)
                {
                    throw std::runtime_error("Line " + std::to_string(row) + ": " + error.what());
                }
            }

            if (!columnsNamed)
                throw std::runtime_error("The file names no columns");

            return read;
        }
    }

    PriceCsvLoader::PriceCsvLoader(const std::vector<std::string>& document, core::Currency currency, core::DayCountConvention convention)
    {
        try
        {
            auto read = ClosesIn(document, currency);

            if (read.dates.size() < 2)
                throw std::runtime_error("A series is read from at least two closes, and this one holds " + std::to_string(read.dates.size()));

            if (core::YearFraction(read.dates.front(), read.dates.back(), convention) <= 0.0)
                throw std::runtime_error("The closes span no time under the day count they are read with");

            series = estimation::PriceSeries::From(std::move(read.dates), std::move(read.closes), convention);
        }
        catch (const std::exception& error)
        {
            series.reset();
            failure = error.what();
        }
    }

    const std::optional<estimation::PriceSeries>& PriceCsvLoader::Series() const
    {
        return series;
    }

    const std::string& PriceCsvLoader::Failure() const
    {
        return failure;
    }
}
