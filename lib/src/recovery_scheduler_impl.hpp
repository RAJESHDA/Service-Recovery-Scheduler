#pragma once

#include "recovery_scheduler.h"

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector> 

namespace recovery_scheduler {

    class recovery_scheduler::Impl final {
    public:
        Impl();
        ~Impl();

        void register_service(std::string service_name, ActionSequence actions);
        void notify_failure(const std::string& service_name);
        void notify_healthy(const std::string& service_name);
        ServiceState query_state(const std::string& service_name) const;
        bool is_registered(const std::string& service_name) const noexcept;
        void wait_for_idle();

    private:
        struct PendingAction final {
            std::shared_ptr<recovery_action> action;
            std::string service_name;
        };

        struct ServiceRecord final {
            mutable std::mutex mutex;
            std::string name;
            ActionSequence actions;
            std::queue<PendingAction> pending_actions;
            bool action_running{ false };
            std::size_t current_level{ 0 };
            std::optional<std::string> last_action_taken{};
            std::size_t total_failures{ 0 };

            std::size_t action_index_for_failure() const;
            void record_failure(const std::shared_ptr<recovery_action>& action);
            void reset_escalation();
            ServiceState snapshot() const;
        };

        class AsyncActionExecutor final {
        public:
            explicit AsyncActionExecutor(std::size_t worker_count);
            ~AsyncActionExecutor();

            void submit(std::function<void()> task);
            void wait_idle();

        private:
            void shutdown();
            void worker_loop();

            std::mutex m_mutex;
            std::condition_variable m_cv;
            std::condition_variable m_idle_cv;
            std::queue<std::function<void()>> m_tasks;
            std::vector<std::thread> m_workers;
            std::size_t m_active_workers{ 0 };
            bool m_stopping{ false };
        };

        static bool has_null_action(const ActionSequence& actions);
        static std::size_t default_worker_count();

        std::shared_ptr<ServiceRecord> find_record(const std::string& service_name) const;
        void enqueue_action(const std::shared_ptr<ServiceRecord>& record,
            std::shared_ptr<recovery_action> action,
            std::string service_name);
        void schedule_next_action(const std::shared_ptr<ServiceRecord>& record);

        mutable std::shared_mutex m_registry_mutex;
        std::unordered_map<std::string, std::shared_ptr<ServiceRecord>> m_services;
        std::unique_ptr<AsyncActionExecutor> m_executor;
    };

} // namespace recovery_scheduler
