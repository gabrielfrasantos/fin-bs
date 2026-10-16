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
