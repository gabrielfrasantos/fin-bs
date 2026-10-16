#include "source/io/ScenarioLoader.hpp"
#include "infra/stream/StringOutputStream.hpp"
#include "infra/syntax/JsonFileReader.hpp"
#include "infra/syntax/JsonFormatter.hpp"
#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <variant>

namespace fin::io
{
    namespace
    {
        constexpr std::int32_t basisPointsPerUnit = 10000;
        constexpr std::int32_t mostVolatilityBasisPoints = 100000;
        constexpr std::int32_t mostCycles = 1000;
        constexpr std::int32_t mostDaysPerCycle = 365;
        constexpr std::int32_t mostPaths = 1000000;
        constexpr std::int32_t nonLeapYear = 2001;
        constexpr std::size_t writtenDateSize = 10;
        constexpr std::size_t roomPerScenario = 1024;
        constexpr std::size_t roomPerUnderlying = 512;

        core::Currency CurrencyNamed(const std::string& name)
        {
            if (name == "EUR")
                return core::Currency::Eur;

            if (name == "USD")
                return core::Currency::Usd;

            throw std::runtime_error("Currency " + name + " is not one this project holds");
        }

        const char* NameOf(core::Currency currency)
        {
            return currency == core::Currency::Eur ? "EUR" : "USD";
        }

        core::Money AmountAt(const infra::JsonObjectNavigator& navigator, const std::string& key, core::Currency currency, std::int32_t least)
        {
            const auto cents = navigator / infra::JsonIntegerNavigatorToken{ key };

            if (cents < least)
                throw std::runtime_error("Integer " + key + " is smaller than this field allows");

            return core::Money::FromCents(cents, currency);
        }

        std::int32_t IntegerAt(const infra::JsonObjectNavigator& navigator, const std::string& key, std::int32_t least, std::int32_t most)
        {
            const auto whole = navigator / infra::JsonIntegerNavigatorToken{ key };

            if (whole < least || whole > most)
                throw std::runtime_error("Integer " + key + " is outside the range this field allows");

            return whole;
        }

        double FractionAt(const infra::JsonObjectNavigator& navigator, const std::string& key, std::int32_t least, std::int32_t most)
        {
            return static_cast<double>(IntegerAt(navigator, key, least, most)) / static_cast<double>(basisPointsPerUnit);
        }

        bool IsWrittenInDigits(std::string_view text)
        {
            return !text.empty() && std::all_of(text.begin(), text.end(), [](char character)
                                        {
                                            return character >= '0' && character <= '9';
                                        });
        }

        std::int32_t NumberOf(std::string_view digits)
        {
            std::int32_t number = 0;

            for (const auto digit : digits)
                number = number * 10 + (digit - '0');

            return number;
        }

        core::Date DateAt(const infra::JsonObjectNavigator& navigator, const std::string& key)
        {
            const auto text = navigator / infra::JsonStringNavigatorToken{ key };

            if (text.size() != writtenDateSize || text[4] != '-' || text[7] != '-' || !IsWrittenInDigits(std::string_view(text).substr(0, 4)) || !IsWrittenInDigits(std::string_view(text).substr(5, 2)) || !IsWrittenInDigits(std::string_view(text).substr(8, 2)))
                throw std::runtime_error("String " + key + " is not written as a year, a month and a day");

            const auto year = NumberOf(std::string_view(text).substr(0, 4));
            const auto month = static_cast<std::uint32_t>(NumberOf(std::string_view(text).substr(5, 2)));
            const auto day = static_cast<std::uint32_t>(NumberOf(std::string_view(text).substr(8, 2)));

            if (!core::Date::IsValid(year, month, day))
                throw std::runtime_error("String " + key + " is not a day of the calendar");

            return core::Date::FromYearMonthDay(year, month, day);
        }

        std::string TextOf(const core::Date& date)
        {
            const auto padded = [](std::uint32_t part)
            {
                return part < 10 ? "0" + std::to_string(part) : std::to_string(part);
            };

            return std::to_string(date.Year()) + "-" + padded(date.Month()) + "-" + padded(date.Day());
        }

        std::string TextAt(const infra::JsonObjectNavigator& navigator, const std::string& key)
        {
            auto text = navigator / infra::JsonStringNavigatorToken{ key };

            if (text.empty())
                throw std::runtime_error("String " + key + " names nothing");

            return text;
        }

        std::optional<sim::AnnualReference> AssessmentIn(const infra::JsonObjectNavigator& navigator)
        {
            const auto assessment = navigator / infra::JsonOptionalObjectNavigatorToken{ "assessment" };
            const auto month = assessment / infra::JsonIntegerNavigatorToken{ "month" };
            const auto day = assessment / infra::JsonIntegerNavigatorToken{ "day" };

            if (!month.has_value())
                return std::nullopt;

            if (!core::Date::IsValid(nonLeapYear, static_cast<std::uint32_t>(*month), static_cast<std::uint32_t>(*day)))
                throw std::runtime_error("Object assessment does not name a day that every year holds");

            return sim::AnnualReference{ static_cast<std::uint32_t>(*month), static_cast<std::uint32_t>(*day) };
        }

        std::vector<sim::UnderlyingTerms> UnderlyingsIn(const infra::JsonObjectNavigator& navigator, core::Currency currency)
        {
            const auto listed = navigator / infra::JsonArrayNavigatorToken{ "underlyings" };
            std::vector<sim::UnderlyingTerms> underlyings;

            for (auto& value : listed.array)
            {
                auto* row = std::get_if<infra::JsonObject>(&value);

                if (row == nullptr)
                    throw std::runtime_error("Array underlyings holds an entry that does not describe an underlying");

                const infra::JsonObjectNavigator entry(*row);
                auto ticker = TextAt(entry, "ticker");

                if (std::any_of(underlyings.begin(), underlyings.end(), [&ticker](const sim::UnderlyingTerms& other)
                        {
                            return other.underlying == ticker;
                        }))
                    throw std::runtime_error("String ticker " + ticker + " names two underlyings of the same book");

                underlyings.push_back({ .underlying = std::move(ticker),
                    .initialPrice = AmountAt(entry, "spot_cents", currency, 1),
                    .drift = core::Rate::FromContinuous(FractionAt(entry, "drift_basis_points", -basisPointsPerUnit, basisPointsPerUnit)),
                    .volatility = core::Volatility::FromAnnualized(FractionAt(entry, "volatility_basis_points", 1, mostVolatilityBasisPoints)),
                    .dividendYield = core::Rate::FromContinuous(FractionAt(entry, "dividend_yield_basis_points", 0, basisPointsPerUnit)),
                    .strikeIncrement = AmountAt(entry, "strike_increment_cents", currency, 1) });
            }

            if (underlyings.empty())
                throw std::runtime_error("Array underlyings names no underlying to wheel");

            return underlyings;
        }

        Scenario ScenarioIn(const infra::JsonObjectNavigator& navigator)
        {
            const auto currency = CurrencyNamed(navigator / infra::JsonStringNavigatorToken{ "currency" });
            const auto daysPerCycle = IntegerAt(navigator, "days_per_cycle", 1, mostDaysPerCycle);
            const auto observation = IntegerAt(navigator, "observation_days_before_expiry", 0, mostDaysPerCycle);

            if (observation >= daysPerCycle)
                throw std::runtime_error("Integer observation_days_before_expiry falls on or before the day a cycle opens");

            return Scenario{
                .name = TextAt(navigator, "scenario"),
                .capital = AmountAt(navigator, "capital_cents", currency, 1),
                .cashBufferBasisPoints = IntegerAt(navigator, "cash_buffer_basis_points", 0, basisPointsPerUnit),
                .triggers = strategy::RollTriggers{
                    .daysToExpiry = IntegerAt(navigator, "roll_days_to_expiry", 0, mostDaysPerCycle),
                    .profitCaptureBasisPoints = IntegerAt(navigator, "roll_profit_capture_basis_points", 1, basisPointsPerUnit),
                    .deltaLimit = core::Delta::FromValue(FractionAt(navigator, "roll_delta_limit_basis_points", 1, basisPointsPerUnit)) },
                .terms = sim::BookTerms{ .start = DateAt(navigator, "start_date"), .cycles = IntegerAt(navigator, "cycles", 1, mostCycles), .daysPerCycle = daysPerCycle, .observationDaysBeforeExpiry = observation, .targetDelta = core::Delta::FromValue(FractionAt(navigator, "target_delta_basis_points", 1, basisPointsPerUnit - 1)), .riskFreeRate = core::Rate::FromContinuous(FractionAt(navigator, "risk_free_rate_basis_points", -basisPointsPerUnit, basisPointsPerUnit)), .assessment = AssessmentIn(navigator), .underlyings = UnderlyingsIn(navigator, currency) },
                .paths = IntegerAt(navigator, "paths", 1, mostPaths),
                .seed = static_cast<std::uint64_t>(IntegerAt(navigator, "seed", 0, std::numeric_limits<std::int32_t>::max())),
                .taxConfiguration = TextAt(navigator, "tax_configuration"),
                .brokerConfiguration = TextAt(navigator, "broker_configuration")
            };
        }

        std::int32_t BasisPointsOf(double fraction)
        {
            return static_cast<std::int32_t>(std::llround(fraction * basisPointsPerUnit));
        }
    }

    ScenarioLoader::ScenarioLoader(const std::vector<std::string>& document)
    {
        try
        {
            infra::JsonFileReader reader(document);
            auto scenario = ScenarioIn(reader.GetNavigator());

            if (!IsWellFormed(scenario))
                throw std::runtime_error("The fields together do not describe a run that can be made");

            loaded = std::move(scenario);
        }
        catch (const std::exception& error)
        {
            loaded.reset();
            failure = error.what();
        }
    }

    const std::optional<Scenario>& ScenarioLoader::Loaded() const
    {
        return loaded;
    }

    const std::string& ScenarioLoader::Failure() const
    {
        return failure;
    }

    bool IsWellFormed(const Scenario& scenario)
    {
        if (scenario.name.empty() || scenario.taxConfiguration.empty() || scenario.brokerConfiguration.empty())
            return false;

        if (scenario.capital.Cents() <= 0 || scenario.paths <= 0)
            return false;

        if (scenario.cashBufferBasisPoints < 0 || scenario.cashBufferBasisPoints > basisPointsPerUnit)
            return false;

        for (const auto& underlying : scenario.terms.underlyings)
            if (underlying.initialPrice.GetCurrency() != scenario.capital.GetCurrency() || underlying.strikeIncrement.GetCurrency() != scenario.capital.GetCurrency())
                return false;

        return strategy::IsWellFormed(scenario.triggers) && sim::IsWellFormed(scenario.terms);
    }

    core::Currency CurrencyOf(const Scenario& scenario)
    {
        return scenario.capital.GetCurrency();
    }

    std::string Formatted(const Scenario& scenario)
    {
        std::vector<char> room(roomPerScenario + roomPerUnderlying * scenario.terms.underlyings.size());
        infra::BoundedString text(infra::MemoryRange<char>(room.data(), room.data() + room.size()));
        infra::StringOutputStream stream(text, infra::noFail);

        {
            infra::JsonObjectFormatter formatter(stream);

            formatter.Add("scenario", infra::BoundedConstString(scenario.name));
            formatter.Add("currency", NameOf(CurrencyOf(scenario)));
            formatter.Add("capital_cents", scenario.capital.Cents());
            formatter.Add("start_date", infra::BoundedConstString(TextOf(scenario.terms.start)));
            formatter.Add("cycles", scenario.terms.cycles);
            formatter.Add("days_per_cycle", scenario.terms.daysPerCycle);
            formatter.Add("observation_days_before_expiry", scenario.terms.observationDaysBeforeExpiry);
            formatter.Add("target_delta_basis_points", BasisPointsOf(scenario.terms.targetDelta.Value()));
            formatter.Add("cash_buffer_basis_points", scenario.cashBufferBasisPoints);
            formatter.Add("roll_delta_limit_basis_points", BasisPointsOf(scenario.triggers.deltaLimit.Value()));
            formatter.Add("roll_days_to_expiry", scenario.triggers.daysToExpiry);
            formatter.Add("roll_profit_capture_basis_points", scenario.triggers.profitCaptureBasisPoints);
            formatter.Add("risk_free_rate_basis_points", BasisPointsOf(scenario.terms.riskFreeRate.Continuous()));

            if (scenario.terms.assessment.has_value())
            {
                auto assessment = formatter.SubObject("assessment");
                assessment.Add("month", static_cast<std::int32_t>(scenario.terms.assessment->month));
                assessment.Add("day", static_cast<std::int32_t>(scenario.terms.assessment->day));
            }

            formatter.Add("paths", scenario.paths);
            formatter.Add("seed", static_cast<std::int64_t>(scenario.seed));
            formatter.Add("tax_configuration", infra::BoundedConstString(scenario.taxConfiguration));
            formatter.Add("broker_configuration", infra::BoundedConstString(scenario.brokerConfiguration));

            auto underlyings = formatter.SubArray("underlyings");

            for (const auto& underlying : scenario.terms.underlyings)
            {
                auto entry = underlyings.SubObject();
                entry.Add("ticker", infra::BoundedConstString(underlying.underlying));
                entry.Add("spot_cents", underlying.initialPrice.Cents());
                entry.Add("drift_basis_points", BasisPointsOf(underlying.drift.Continuous()));
                entry.Add("volatility_basis_points", BasisPointsOf(underlying.volatility.Annualized()));
                entry.Add("dividend_yield_basis_points", BasisPointsOf(underlying.dividendYield.Continuous()));
                entry.Add("strike_increment_cents", underlying.strikeIncrement.Cents());
            }
        }

        return std::string(text.begin(), text.end());
    }
}
