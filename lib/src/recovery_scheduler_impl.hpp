#pragma once

#include "recovery_scheduler.h"
#include <string>

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
        // to be implemented
    };

} // namespace recovery_scheduler
