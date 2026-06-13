#pragma once

#include "recovery_action.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace recovery_scheduler {

    struct ServiceState final {
        std::size_t m_current_level{ 0 };
        std::optional<std::string> m_last_action_taken{};
        std::size_t m_total_failures{ 0 };
        std::vector<std::string> m_pending_actions{};
    };

    class recovery_scheduler final {
    public:
        using ActionSequence = std::vector<std::shared_ptr<recovery_action>>;

        recovery_scheduler();
        ~recovery_scheduler();

        recovery_scheduler(const recovery_scheduler&) = delete;
        recovery_scheduler& operator=(const recovery_scheduler&) = delete;
        recovery_scheduler(recovery_scheduler&&) = delete;
        recovery_scheduler& operator=(recovery_scheduler&&) = delete;

        void register_service(std::string service_name, ActionSequence actions);

        void notify_failure(const std::string& service_name);
        void notify_healthy(const std::string& service_name);

        [[nodiscard]] ServiceState query_state(const std::string& service_name) const;
        [[nodiscard]] bool is_registered(const std::string& service_name) const noexcept;

        void wait_for_idle();

    private:
        struct Impl;
        std::unique_ptr<Impl> m_impl;
    };

} // namespace recovery_scheduler
