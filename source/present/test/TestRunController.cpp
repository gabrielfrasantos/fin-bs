#include "source/present/RunController.hpp"
#include "source/present/test/RunScenario.hpp"
#include <cstdint>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{
    using fin::present::RunController;
    using fin::present::RunKind;
    using fin::present::RunState;
    using fin::present::test::Calibrated;
    using fin::present::test::Inputs;
    using ::testing::HasSubstr;

    class StoppingController
        : public RunController
    {
    public:
        explicit StoppingController(std::int32_t after)
            : after(after)
        {}

        void Completed(std::int32_t path, std::int32_t of) override
        {
            RunController::Completed(path, of);

            if (path == after)
                Cancel();
        }

    private:
        std::int32_t after;
    };

    TEST(TestRunController, a_projection_writes_a_row_for_every_underlying_of_every_cycle)
    {
        RunController controller;
        controller.Start(Inputs(), RunKind::projection);

        ASSERT_EQ(controller.State(), RunState::completed);
        ASSERT_TRUE(controller.Result().has_value());
        EXPECT_EQ(controller.Result()->rows.size(), 4u);
        EXPECT_EQ(controller.Result()->boundaries.size(), 3u);
        EXPECT_FALSE(controller.Result()->ledger.Entries().empty());
        EXPECT_EQ(controller.Progress(), (fin::present::RunProgress{ 2, 2 }));
    }

    TEST(TestRunController, a_simulation_reports_progress_that_reaches_exactly_the_paths_it_was_asked_for)
    {
        RunController controller;
        controller.Start(Inputs(), RunKind::simulation);

        ASSERT_EQ(controller.State(), RunState::completed);
        ASSERT_TRUE(controller.Result()->distribution.has_value());
        EXPECT_EQ(controller.Result()->distribution->Paths(), 8u);
        EXPECT_EQ(controller.Progress(), (fin::present::RunProgress{ 8, 8 }));
    }

    TEST(TestRunController, a_cancelled_simulation_keeps_the_paths_it_had_already_walked)
    {
        StoppingController controller(3);
        controller.Start(Inputs(), RunKind::simulation);

        EXPECT_EQ(controller.State(), RunState::cancelled);
        ASSERT_TRUE(controller.Result()->distribution.has_value());
        EXPECT_EQ(controller.Result()->distribution->Paths(), 3u);
    }

    TEST(TestRunController, walks_one_path_before_it_stops_so_a_cancelled_run_still_reports_something)
    {
        StoppingController controller(1);
        controller.Start(Inputs(), RunKind::simulation);

        EXPECT_EQ(controller.State(), RunState::cancelled);
        EXPECT_EQ(controller.Result()->distribution->Paths(), 1u);
    }

    TEST(TestRunController, a_cancelled_projection_stops_at_the_end_of_a_cycle)
    {
        StoppingController controller(1);
        controller.Start(Inputs(), RunKind::projection);

        EXPECT_EQ(controller.State(), RunState::cancelled);
        EXPECT_EQ(controller.Result()->rows.size(), 2u);
    }

    TEST(TestRunController, reports_a_scenario_it_cannot_run_rather_than_throwing_at_the_caller)
    {
        auto inputs = Inputs();
        inputs.scenario.paths = 0;

        RunController controller;
        controller.Start(inputs, RunKind::simulation);

        EXPECT_EQ(controller.State(), RunState::failed);
        EXPECT_FALSE(controller.Result().has_value());
        EXPECT_THAT(controller.Failure(), HasSubstr("scenario"));
    }

    TEST(TestRunController, reports_a_broker_configuration_of_another_currency_rather_than_throwing_at_the_caller)
    {
        auto inputs = Inputs();
        inputs.fees.optionFeePerContract = fin::core::Money::FromCents(75, fin::core::Currency::Usd);
        inputs.fees.shareCommissionPerTrade = fin::core::Money::FromCents(200, fin::core::Currency::Usd);
        inputs.fees.handlingFeePerTrade = fin::core::Money::FromCents(100, fin::core::Currency::Usd);
        inputs.fees.connectivityFeeCapPerExchangePerYear = fin::core::Money::FromCents(250, fin::core::Currency::Usd);

        RunController controller;
        controller.Start(inputs, RunKind::simulation);

        EXPECT_EQ(controller.State(), RunState::failed);
        EXPECT_THAT(controller.Failure(), HasSubstr("currency"));
    }

    TEST(TestRunController, a_search_reports_the_setting_it_settled_on)
    {
        RunController controller;
        controller.Start(Inputs(), RunKind::search);

        ASSERT_EQ(controller.State(), RunState::completed);
        ASSERT_TRUE(controller.Result()->recommendation.has_value());

        const auto& recommendation = *controller.Result()->recommendation;

        EXPECT_GT(recommendation.setting.targetDelta.Value(), 0.0);
        EXPECT_LT(recommendation.setting.targetDelta.Value(), 1.0);
        EXPECT_EQ(controller.Progress(), (fin::present::RunProgress{ 1, 1 }));
    }

    TEST(TestRunController, runs_the_same_scenario_twice_to_the_same_answer)
    {
        RunController controller;
        controller.Start(Inputs(), RunKind::projection);
        const auto first = controller.Result()->rows;

        controller.Start(Inputs(), RunKind::projection);

        EXPECT_EQ(controller.Result()->rows, first);
    }
}

namespace
{
    using fin::core::Date;
    using fin::estimation::PriceSeries;
    using fin::present::CalibrationRow;

    Date On(std::int32_t year, std::uint32_t month, std::uint32_t day)
    {
        return Date::FromYearMonthDay(year, month, day);
    }

    PriceSeries Truncated(const PriceSeries& history, std::size_t closes)
    {
        const auto dates = history.Dates().subspan(0, closes);
        const auto prices = history.Closes().subspan(0, closes);

        return PriceSeries::From(std::vector<Date>(dates.begin(), dates.end()), std::vector<fin::core::Money>(prices.begin(), prices.end()), history.Convention());
    }

    TEST(TestRunController, a_backtest_writes_a_row_and_an_estimate_for_every_underlying_of_every_cycle)
    {
        RunController controller;
        controller.Start(Calibrated(), RunKind::backtest);

        ASSERT_EQ(controller.State(), RunState::completed) << controller.Failure();
        EXPECT_EQ(controller.Result()->kind, RunKind::backtest);
        EXPECT_EQ(controller.Result()->rows.size(), 4u);
        EXPECT_EQ(controller.Result()->boundaries.size(), 3u);
        EXPECT_EQ(controller.Result()->equity.size(), 3u);
        ASSERT_EQ(controller.Result()->calibration.size(), 4u);
        EXPECT_EQ(controller.Result()->calibration[0].cycle, 0);
        EXPECT_EQ(controller.Result()->calibration[0].underlying, "ACME");
        EXPECT_EQ(controller.Result()->calibration[1].underlying, "BETA");
        EXPECT_EQ(controller.Result()->calibration[2].cycle, 1);
        EXPECT_EQ(controller.Progress(), (fin::present::RunProgress{ 2, 2 }));
    }

    TEST(TestRunController, a_backtest_estimates_each_cycle_from_the_closes_before_its_opening)
    {
        RunController controller;
        controller.Start(Calibrated(), RunKind::backtest);

        ASSERT_EQ(controller.State(), RunState::completed) << controller.Failure();
        EXPECT_EQ(controller.Result()->calibration[0].estimate.windowEnd, On(2026, 1, 2));
        EXPECT_EQ(controller.Result()->calibration[2].estimate.windowEnd, On(2026, 7, 3));
        EXPECT_EQ(controller.Result()->calibration[0].estimate.observations, 252);
        EXPECT_NEAR(controller.Result()->calibration[0].estimate.impliedVolatility.Annualized(), controller.Result()->calibration[0].estimate.volatility.Annualized() + 0.02, 1e-12);
    }

    TEST(TestRunController, a_backtest_runs_twice_to_the_same_answer)
    {
        RunController controller;
        controller.Start(Calibrated(), RunKind::backtest);
        const auto first = controller.Result()->rows;

        controller.Start(Calibrated(), RunKind::backtest);

        EXPECT_EQ(controller.Result()->rows, first);
    }

    TEST(TestRunController, a_backtest_without_a_calibration_block_is_refused)
    {
        RunController controller;
        controller.Start(Inputs(), RunKind::backtest);

        EXPECT_EQ(controller.State(), RunState::failed);
        EXPECT_THAT(controller.Failure(), HasSubstr("calibration"));
    }

    TEST(TestRunController, a_backtest_refuses_a_history_that_stops_short_of_the_schedule_by_name)
    {
        auto inputs = Calibrated();
        inputs.histories[1].history = Truncated(inputs.histories[1].history, 800);

        RunController controller;
        controller.Start(inputs, RunKind::backtest);

        EXPECT_EQ(controller.State(), RunState::failed);
        EXPECT_THAT(controller.Failure(), HasSubstr("BETA"));
        EXPECT_THAT(controller.Failure(), HasSubstr("ends on"));
    }

    TEST(TestRunController, a_backtest_refuses_a_window_the_history_cannot_fill_by_name)
    {
        auto inputs = Calibrated();
        inputs.scenario.calibration->rule.windowObservations = 5000;

        RunController controller;
        controller.Start(inputs, RunKind::backtest);

        EXPECT_EQ(controller.State(), RunState::failed);
        EXPECT_THAT(controller.Failure(), HasSubstr("ACME"));
        EXPECT_THAT(controller.Failure(), HasSubstr("needs 5000"));
    }

    TEST(TestRunController, a_backtest_refuses_an_underlying_without_a_history_by_name)
    {
        auto inputs = Calibrated();
        inputs.histories.pop_back();

        RunController controller;
        controller.Start(inputs, RunKind::backtest);

        EXPECT_EQ(controller.State(), RunState::failed);
        EXPECT_THAT(controller.Failure(), HasSubstr("No price history was read for BETA"));
    }

    TEST(TestRunController, a_forward_run_with_a_calibration_block_takes_its_terms_at_the_start_from_the_rule)
    {
        RunController projected;
        projected.Start(Calibrated(), RunKind::projection);

        ASSERT_EQ(projected.State(), RunState::completed) << projected.Failure();
        ASSERT_EQ(projected.Result()->calibration.size(), 2u);
        EXPECT_EQ(projected.Result()->calibration[0].cycle, 0);
        EXPECT_EQ(projected.Result()->calibration[1].cycle, 0);
        EXPECT_EQ(projected.Result()->calibration[0].estimate.windowEnd, On(2026, 1, 2));

        RunController stated;
        stated.Start(Inputs(), RunKind::projection);

        EXPECT_NE(projected.Result()->rows, stated.Result()->rows);

        RunController simulated;
        simulated.Start(Calibrated(), RunKind::simulation);

        ASSERT_EQ(simulated.State(), RunState::completed) << simulated.Failure();
        EXPECT_EQ(simulated.Result()->calibration.size(), 2u);
    }
}
