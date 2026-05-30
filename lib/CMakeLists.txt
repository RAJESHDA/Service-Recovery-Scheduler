add_library(lib STATIC 
    "include/recovery_execution_context.h"
    "include/recovery_action.h"
    "include/disable_action.h"
    "include/restart_action.h"
    "include/stop_action.h"
    "src/stop_action.cpp"
    "src/restart_action.cpp"
    "src/disable_action.cpp" 
    "include/recovery_scheduler.h"
    "src/recovery_scheduler_impl.hpp"
    "src/recovery_scheduler_impl.cpp"
    "src/recovery_scheduler.cpp")

# Configure include directories 
target_include_directories(lib 
    PUBLIC 
        # Exposes the 'include' directory to any target linking this library
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PUBLIC 
        # Only accessible when compiling the library itself
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
