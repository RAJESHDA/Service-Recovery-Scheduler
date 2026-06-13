#include "recovery_scheduler_impl.hpp"

#include <algorithm>
#include <atomic>
#include <stdexcept>
#include <utility>
#include <iostream>
#include <iterator> 

namespace recovery_scheduler {
    namespace {

        std::size_t clamp_action_index(std::size_t current_level, std::size_t action_count) {
            return std::min(current_level, action_count - 1);
        }

    } // namespace

    // ServiceRecord helpers ------------------------------------------------------

    std::size_t recovery_scheduler::Impl::ServiceRecord::action_index_for_failure() const {
        return clamp_action_index(current_level, actions.size());
    }

    void recovery_scheduler::Impl::ServiceRecord::record_failure(const std::shared_ptr<recovery_action>& action) {
        last_action_taken = action->name();
        ++total_failures;
        if (current_level < actions.size()) {
            ++current_level;
        }
    }

    void recovery_scheduler::Impl::ServiceRecord::reset_escalation() {
        current_level = 0;

    }

    void recovery_scheduler::Impl::ServiceRecord::clear_pending_actions() {
        while (!pending_actions.empty()) {
            pending_actions.pop();
        }    
    }

    ServiceState recovery_scheduler::Impl::ServiceRecord::snapshot() const {
        return ServiceState{ current_level, last_action_taken, total_failures };
    }

    // AsyncActionExecutor --------------------------------------------------------

    recovery_scheduler::Impl::AsyncActionExecutor::AsyncActionExecutor(std::size_t worker_count)
        : m_workers(worker_count) {
        for (auto& worker : m_workers) {
            worker = std::thread([this]() { worker_loop(); });
        }
    }

    recovery_scheduler::Impl::AsyncActionExecutor::~AsyncActionExecutor() {
        shutdown();
    }

    void recovery_scheduler::Impl::AsyncActionExecutor::submit(std::function<void()> task) {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_stopping) {
                throw std::runtime_error("executor is shutting down");
            }
            m_tasks.push(std::move(task));
        }
        m_cv.notify_one();
    }

    void recovery_scheduler::Impl::AsyncActionExecutor::wait_idle() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_idle_cv.wait(lock, [this]() { return m_tasks.empty() && m_active_workers == 0; });
    }

    void recovery_scheduler::Impl::AsyncActionExecutor::shutdown() {
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_stopping = true;
        }
        m_cv.notify_all();

        for (auto& worker : m_workers) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void recovery_scheduler::Impl::AsyncActionExecutor::worker_loop() {
        for (;;) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                m_cv.wait(lock, [this]() { return m_stopping || !m_tasks.empty(); });
                if (m_stopping && m_tasks.empty()) {
                    return;
                }
                task = std::move(m_tasks.front());
                m_tasks.pop();
                ++m_active_workers;
            }

            try {
                task();
            }
            catch (...) {
                // Dummy recovery actions are not expected to throw.
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                --m_active_workers;
                if (m_tasks.empty() && m_active_workers == 0) {
                    m_idle_cv.notify_all();
                }
            }
        }
    }

    // Impl helpers ---------------------------------------------------------------

    bool recovery_scheduler::Impl::has_null_action(const ActionSequence& actions) {
        return std::any_of(actions.begin(), actions.end(),
            [](const std::shared_ptr<recovery_action>& action) { return action == nullptr; });
    }

    std::size_t recovery_scheduler::Impl::default_worker_count() {
        const auto hw = std::thread::hardware_concurrency();
        return hw == 0 ? 2U : std::max<std::size_t>(2U, hw);
    }

    std::shared_ptr<recovery_scheduler::Impl::ServiceRecord>
        recovery_scheduler::Impl::find_record(const std::string& service_name) const {
        std::shared_lock<std::shared_mutex> lock(m_registry_mutex);
        const auto it = m_services.find(service_name);
        if (it == m_services.end()) {
            throw std::out_of_range("unknown service: " + service_name);
        }
        return it->second;
    }

    void recovery_scheduler::Impl::enqueue_action(const std::shared_ptr<ServiceRecord>& record,
        std::shared_ptr<recovery_action> action,
        std::string service_name) {
        bool should_schedule = false;
        {
            std::lock_guard<std::mutex> lock(record->mutex);
            record->pending_actions.push(PendingAction{ std::move(action), std::move(service_name) });
            if (!record->action_running) {
                record->action_running = true;
                should_schedule = true;
            }
        }

        if (should_schedule) {
            schedule_next_action(record);
        }
    }

    void recovery_scheduler::Impl::schedule_next_action(const std::shared_ptr<ServiceRecord>& record) 
    {
        PendingAction pending;
        bool has_action = false;

        {
            std::lock_guard<std::mutex> lock(record->mutex);
            if (!record->pending_actions.empty()) {
                pending = std::move(record->pending_actions.front());
                record->pending_actions.pop();
                has_action = true;
            }
            else {
                record->action_running = false;
            }
        }

        if (!has_action) {
            return;
        }

        m_executor->submit([this, record, pending = std::move(pending)]() mutable 
            {
            try {
                pending.action->execute(recovery_execution_context{ pending.service_name });
            }
            catch (...) {
                // Keep the scheduler alive even if a dummy action misbehaves.
            }
            schedule_next_action(record);
            });
    }

    // Impl public methods --------------------------------------------------------

    recovery_scheduler::Impl::Impl()
        : m_executor(std::make_unique<AsyncActionExecutor>(default_worker_count())) {
    }

    recovery_scheduler::Impl::~Impl() = default;

    void recovery_scheduler::Impl::register_service(std::string service_name, ActionSequence actions) {
        
        if (service_name.empty()) {
            throw std::invalid_argument("service_name must not be empty");
        }
        if (actions.empty()) {
            throw std::invalid_argument("actions must not be empty");
        }
        if (has_null_action(actions)) {
            throw std::invalid_argument("actions must not contain null entries");
        }

        std::cout << "Registering " << service_name << " service with actions ";  std::for_each(actions.begin(), actions.end(), [](const std::shared_ptr<recovery_action>& action_ptr) {
            std::cout << action_ptr->name() << " ";
            }); std::cout << std::endl;
        auto record = std::make_shared<ServiceRecord>();
        record->name = std::move(service_name);
        record->actions = std::move(actions);

        std::unique_lock<std::shared_mutex> lock(m_registry_mutex);
        if (m_services.find(record->name) != m_services.end()) {
            throw std::invalid_argument("service already registered");
        }
        m_services.emplace(record->name, std::move(record));
    }

    void recovery_scheduler::Impl::notify_failure(const std::string& service_name) {
        const auto record = find_record(service_name);
        std::cout << "$$$Failure occured for service " << service_name << std::endl;
        std::shared_ptr<recovery_action> action_to_queue;
        std::string service_for_action;

        {
            std::lock_guard<std::mutex> lock(record->mutex);
            const std::size_t action_index = record->action_index_for_failure();
            action_to_queue = record->actions.at(action_index);
            service_for_action = record->name;

            record->record_failure(action_to_queue);
        }

        enqueue_action(record, std::move(action_to_queue), std::move(service_for_action));
    }

    void recovery_scheduler::Impl::notify_healthy(const std::string& service_name) {
        const auto record = find_record(service_name);
        std::lock_guard<std::mutex> lock(record->mutex);
        record->reset_escalation();
        record->clear_pending_actions();
    }

    ServiceState recovery_scheduler::Impl::query_state(const std::string& service_name) const {
        const auto record = find_record(service_name);
        std::lock_guard<std::mutex> lock(record->mutex);
        return record->snapshot();
    }

    bool recovery_scheduler::Impl::is_registered(const std::string& service_name) const noexcept {
        std::shared_lock<std::shared_mutex> lock(m_registry_mutex);
        return m_services.find(service_name) != m_services.end();
    }

    void recovery_scheduler::Impl::wait_for_idle() {
        m_executor->wait_idle();
    }


} // namespace recovery_scheduler
