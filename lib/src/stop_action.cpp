#include "disable_action.h"
#include <iostream>

namespace recovery_scheduler 
{

    std::string disable_action::name() const 
    {
        return "DISABLE";
    }

    void disable_action::execute(const recovery_execution_context& ctx)
    {
        std::cout
            << "[RECOVERY] "
            << "service=" << ctx.m_service_name
            << ", action=DISABLE"
            << ", failure_count=" << ctx.m_failure_count
            << ", escalation_level=" << ctx.m_escalation_level
            << '\n';
    }
}