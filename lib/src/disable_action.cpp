#include "stop_action.h"
#include <iostream>

namespace recovery_scheduler 
{

    std::string stop_action::name() const 
    {
        return "STOP";
    }

    void stop_action::execute(const recovery_execution_context& ctx)
    {
        std::cout
            << "[RECOVERY] "
            << "service=" << ctx.m_service_name
            << ", action=STOP"
            << ", failure_count=" << ctx.m_failure_count
            << ", escalation_level=" << ctx.m_escalation_level
            << '\n';
    }

}