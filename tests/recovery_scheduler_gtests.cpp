#include "recovery_scheduler.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


namespace {

    struct RecordingAction final : recovery_scheduler::recovery_action {
        std::string action_name;
        std::vector<std::string>* log;
        std::mutex* log_mutex;

        RecordingAction(std::string name, std::vector<std::string>* sink, std::mutex* mutex)
            : action_name(std::move(name)), log(sink), log_mutex(mutex) {
        }

        std::string name() const override { return action_name; }

        void execute(const recovery_scheduler::recovery_execution_context& context) override {
            std::lock_guard<std::mutex> lock(*log_mutex);
            log->push_back(action_name + ":" + context.m_service_name);
        }
    };

    struct BlockingAction final : recovery_scheduler::recovery_action {
        std::string action_name;
        std::vector<std::string>* log;
        std::mutex* log_mutex;
        std::mutex* sync_mutex;
        std::condition_variable* sync_cv;
        std::atomic<int>* started_count;
        std::shared_future<void> release;

        BlockingAction(std::string name,
            std::vector<std::string>* sink,
            std::mutex* sink_mutex,
            std::mutex* start_mutex,
            std::condition_variable* start_cv,
            std::atomic<int>* started,
            std::shared_future<void> release_signal)
            : action_name(std::move(name))
            , log(sink)
            , log_mutex(sink_mutex)
            , sync_mutex(start_mutex)
            , sync_cv(start_cv)
            , started_count(started)
            , release(std::move(release_signal)) {
        }

        std::string name() const override { return action_name; }

        void execute(const recovery_scheduler::recovery_execution_context& context) override {
            {
                std::lock_guard<std::mutex> lock(*log_mutex);
                log->push_back(action_name + ":" + context.m_service_name);
            }

            {
                std::lock_guard<std::mutex> lock(*sync_mutex);
                ++(*started_count);
            }
            sync_cv->notify_all();

            release.wait();
        }
    };

    std::shared_ptr<recovery_scheduler::recovery_action> make_action(const std::string& name,
        std::vector<std::string>* log,
        std::mutex* log_mutex) {
        return std::make_shared<RecordingAction>(name, log, log_mutex);
    }

} // namespace

TEST(RecoverySchedulerTest, RegistrationAndInitialState) {
    recovery_scheduler::recovery_scheduler scheduler;
    std::vector<std::string> log;
    std::mutex log_mutex;

    scheduler.register_service(
        "svc",
        { make_action("RESTART", &log, &log_mutex),
         make_action("STOP", &log, &log_mutex),
         make_action("DISABLE", &log, &log_mutex) });

    EXPECT_TRUE(scheduler.is_registered("svc"));

    const auto state = scheduler.query_state("svc");
    EXPECT_EQ(state.m_current_level, std::size_t{ 0 });
    EXPECT_EQ(state.m_total_failures, std::size_t{ 0 });
    EXPECT_FALSE(state.m_last_action_taken.has_value());
    EXPECT_TRUE(log.empty());
}

TEST(RecoverySchedulerTest, FailureEscalatesThroughActions) {
    recovery_scheduler::recovery_scheduler scheduler;
    std::vector<std::string> log;
    std::mutex log_mutex;

    scheduler.register_service(
        "svc",
        { make_action("RESTART", &log, &log_mutex),
         make_action("RESTART", &log, &log_mutex),
         make_action("STOP", &log, &log_mutex),
         make_action("DISABLE", &log, &log_mutex) });

    scheduler.notify_failure("svc");
    auto state = scheduler.query_state("svc");
    EXPECT_EQ(state.m_current_level, std::size_t{ 1 });
    EXPECT_EQ(state.m_total_failures, std::size_t{ 1 });
    ASSERT_TRUE(state.m_last_action_taken.has_value());
    EXPECT_EQ(*state.m_last_action_taken, "RESTART");

    scheduler.notify_failure("svc");
    state = scheduler.query_state("svc");
    EXPECT_EQ(state.m_current_level, std::size_t{ 2 });
    EXPECT_EQ(state.m_total_failures, std::size_t{ 2 });
    ASSERT_TRUE(state.m_last_action_taken.has_value());
    EXPECT_EQ(*state.m_last_action_taken, "RESTART");

    scheduler.notify_failure("svc");
    state = scheduler.query_state("svc");
    EXPECT_EQ(state.m_current_level, std::size_t{ 3 });
    EXPECT_EQ(state.m_total_failures, std::size_t{ 3 });
    ASSERT_TRUE(state.m_last_action_taken.has_value());
    EXPECT_EQ(*state.m_last_action_taken, "STOP");

    scheduler.notify_failure("svc");
    state = scheduler.query_state("svc");
    EXPECT_EQ(state.m_current_level, std::size_t{ 4 });
    EXPECT_EQ(state.m_total_failures, std::size_t{ 4 });
    ASSERT_TRUE(state.m_last_action_taken.has_value());
    EXPECT_EQ(*state.m_last_action_taken, "DISABLE");

    scheduler.wait_for_idle();

    const std::vector<std::string> expected{
        "RESTART:svc",
        "RESTART:svc",
        "STOP:svc",
        "DISABLE:svc",
    };
    EXPECT_EQ(log, expected);
}

TEST(RecoverySchedulerTest, HealthyResetsLevelButKeepsHistory) {
    recovery_scheduler::recovery_scheduler scheduler;
    std::vector<std::string> log;
    std::mutex log_mutex;

    scheduler.register_service(
        "svc",
        { make_action("RESTART", &log, &log_mutex), make_action("STOP", &log, &log_mutex) });

    scheduler.notify_failure("svc");
    scheduler.notify_failure("svc");
    scheduler.notify_healthy("svc");

    auto state = scheduler.query_state("svc");
    EXPECT_EQ(state.m_current_level, std::size_t{ 0 });
    EXPECT_EQ(state.m_total_failures, std::size_t{ 2 });
    ASSERT_TRUE(state.m_last_action_taken.has_value());
    EXPECT_EQ(*state.m_last_action_taken, "STOP");

    scheduler.notify_failure("svc");
    state = scheduler.query_state("svc");
    EXPECT_EQ(state.m_current_level, std::size_t{ 1 });
    EXPECT_EQ(state.m_total_failures, std::size_t{ 3 });
    ASSERT_TRUE(state.m_last_action_taken.has_value());
    EXPECT_EQ(*state.m_last_action_taken, "RESTART");

    scheduler.wait_for_idle();

    const std::vector<std::string> expected{
        "RESTART:svc",
        "STOP:svc",
        "RESTART:svc",
    };
    EXPECT_EQ(log, expected);
}

TEST(RecoverySchedulerTest, ParallelExecutionAcrossServices) {
    recovery_scheduler::recovery_scheduler scheduler;
    std::vector<std::string> log;
    std::mutex log_mutex;
    std::mutex start_mutex;
    std::condition_variable start_cv;
    std::atomic<int> started{ 0 };
    std::promise<void> release_promise;
    const auto release = release_promise.get_future().share();

    scheduler.register_service(
        "svc-a",
        { std::make_shared<BlockingAction>("RESTART", &log, &log_mutex, &start_mutex, &start_cv, &started, release) });
    scheduler.register_service(
        "svc-b",
        { std::make_shared<BlockingAction>("RESTART", &log, &log_mutex, &start_mutex, &start_cv, &started, release) });

    scheduler.notify_failure("svc-a");
    scheduler.notify_failure("svc-b");

    {
        std::unique_lock<std::mutex> lock(start_mutex);
        const bool both_started = start_cv.wait_for(lock, std::chrono::seconds(2), [&]() {
            return started.load() == 2;
            });
        EXPECT_TRUE(both_started);
    }

    release_promise.set_value();
    scheduler.wait_for_idle();

    EXPECT_EQ(log.size(), std::size_t{ 2 });
    EXPECT_NE(std::find(log.begin(), log.end(), "RESTART:svc-a"), log.end());
    EXPECT_NE(std::find(log.begin(), log.end(), "RESTART:svc-b"), log.end());
}

TEST(RecoverySchedulerTest, UnknownServiceThrows) {
    recovery_scheduler::recovery_scheduler scheduler;

    EXPECT_THROW(scheduler.notify_failure("missing"), std::out_of_range);
}

TEST(RecoverySchedulerTest, InvalidRegistrationIsRejected) {
    recovery_scheduler::recovery_scheduler scheduler;
    std::vector<std::string> log;
    std::mutex log_mutex;

    EXPECT_THROW(scheduler.register_service("svc", {}), std::invalid_argument);

    scheduler.register_service("svc", { make_action("RESTART", &log, &log_mutex) });
    EXPECT_THROW(scheduler.register_service("svc", { make_action("STOP", &log, &log_mutex) }),
        std::invalid_argument);
}
