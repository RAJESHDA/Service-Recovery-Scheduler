
#pragma once

#include "recovery_execution_context.h"

#include <string>

namespace recovery_scheduler 
{

    class recovery_action 
    {
    public:
        virtual ~recovery_action() = default;

        virtual std::string name() const = 0;
        virtual void execute(const recovery_execution_context& context) = 0;
    };
}