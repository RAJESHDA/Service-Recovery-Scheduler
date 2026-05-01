#include "lib/include/recovery_action.h"
#include "lib/include/recovery_execution_context.h"
#include "lib/include/disable_action.h"
#include "lib/include/restart_action.h"
#include "lib/include/stop_action.h"

#include <gtest/gtest.h>

#include <functional>
#include <memory>
#include <string>

using recovery_scheduler::disable_action;
using recovery_scheduler::recovery_action;
using recovery_scheduler::restart_action;
using recovery_scheduler::recovery_execution_context;
using recovery_scheduler::stop_action;

namespace {

    struct ActionCase {
        std::string expected_name;
        std::function<std::unique_ptr<recovery_action>()> factory;
    };

    class RecoveryActionTest : public ::testing::TestWithParam<ActionCase> {};

    TEST(RecoveryActionConcreteTypes, RestartActionReportsItsName) {
        restart_action action;
        EXPECT_EQ(action.name(), "RESTART");
    }

    TEST(RecoveryActionConcreteTypes, StopActionReportsItsName) {
        stop_action action;
        EXPECT_EQ(action.name(), "STOP");
    }

    TEST(RecoveryActionConcreteTypes, DisableActionReportsItsName) {
        disable_action action;
        EXPECT_EQ(action.name(), "DISABLE");
    }

    TEST_P(RecoveryActionTest, ExecuteDoesNotThrowForAValidContext) {
        auto action = GetParam().factory();
        recovery_execution_context context{ "auth-service" };

        EXPECT_NO_THROW(action->execute(context));
    }

    TEST_P(RecoveryActionTest, NameMatchesExpectedValue) {
        auto action = GetParam().factory();
        EXPECT_EQ(action->name(), GetParam().expected_name);
    }

    INSTANTIATE_TEST_SUITE_P(
        ConcreteRecoveryActions,
        RecoveryActionTest,
        ::testing::Values(
            ActionCase{ "RESTART", [] { return std::make_unique<restart_action>(); } },
            ActionCase{ "STOP", [] { return std::make_unique<stop_action>(); } },
            ActionCase{ "DISABLE", [] { return std::make_unique<disable_action>(); } }));

} // namespace
