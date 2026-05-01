#pragma once

#include <string>

namespace recovery_scheduler 
{

    class recovery_execution_context final 
    {
    public:
        std::string m_service_name;
        std::size_t m_failure_count;
        std::size_t m_escalation_level;
    };

} 
// namespace recovery_scheduler
