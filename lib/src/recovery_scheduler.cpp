#include "recovery_scheduler.h"
#include "recovery_scheduler_impl.hpp"

#include <memory>
#include <string>
#include <utility>

namespace recovery_scheduler {

    recovery_scheduler::recovery_scheduler()
        : m_impl(std::make_unique<Impl>()) {
    }

    recovery_scheduler::~recovery_scheduler() = default;

    void recovery_scheduler::register_service(std::string service_name, ActionSequence actions) {
        m_impl->register_service(std::move(service_name), std::move(actions));
    }

    void recovery_scheduler::notify_failure(const std::string& service_name) {
        m_impl->notify_failure(service_name);
    }

    void recovery_scheduler::notify_healthy(const std::string& service_name) {
        m_impl->notify_healthy(service_name);
    }

    ServiceState recovery_scheduler::query_state(const std::string& service_name) const {
        return m_impl->query_state(service_name);
    }

    bool recovery_scheduler::is_registered(const std::string& service_name) const noexcept {
        return m_impl->is_registered(service_name);
    }

    void recovery_scheduler::wait_for_idle() {
        m_impl->wait_for_idle();
    }

} // namespace recovery_scheduler
