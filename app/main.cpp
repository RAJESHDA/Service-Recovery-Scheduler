// main.cpp : Defines the entry point for the application.
//
#include "recovery_scheduler.h"

#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <vector>



struct ConsoleAction final : recovery_scheduler::recovery_action {
    std::string action_name;

    explicit ConsoleAction(std::string name) : action_name(std::move(name)) {}

    std::string name() const override 
    { 
        return action_name; 
    }

    void execute(const recovery_scheduler::recovery_execution_context& context) override
    {
        std::cout << "Executing " << action_name << " for " << context.m_service_name << '\n';
    }
};

int main() 
{
    recovery_scheduler::recovery_scheduler scheduler;

    
    scheduler.register_service("auth", {
        std::make_shared<ConsoleAction>("RESTART"),
        std::make_shared<ConsoleAction>("STOP"),
        std::make_shared<ConsoleAction>("DISABLE"),
        });

    scheduler.register_service("db", {
        std::make_shared<ConsoleAction>("RESTART"),
        std::make_shared<ConsoleAction>("DISABLE"),
        });

    scheduler.notify_failure("auth");
    scheduler.notify_failure("db");
    scheduler.notify_failure("auth");
    scheduler.wait_for_idle();

    const auto auth = scheduler.query_state("auth");
    const auto db = scheduler.query_state("db");

    std::cout << "auth level=" << auth.m_current_level << " failures=" << auth.m_total_failures << '\n';
    std::cout << "db level=" << db.m_current_level << " failures=" << db.m_total_failures << '\n';
    return 0;
}
