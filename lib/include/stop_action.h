
#pragma once

#include "recovery_execution_context.h"
#include "recovery_action.h"
#include <string>

namespace recovery_scheduler 
{

    class stop_action final : public recovery_action
    {
    public:
        std::string name() const override;
        void execute(const recovery_execution_context& context) override;
    };
}