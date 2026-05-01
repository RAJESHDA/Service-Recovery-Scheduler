add_library(lib STATIC 
    "include/recovery_execution_context.h"
    "include/recovery_action.h"
    "include/disable_action.h"
    "include/restart_action.h"
    "include/stop_action.h"
    "src/stop_action.cpp"
    "src/restart_action.cpp"
    "src/disable_action.cpp" 
    "include/recovery_scheduler.h")

# Configure include directories 
target_include_directories(lib 
    PUBLIC 
        # Exposes the 'include' directory to any target linking this library
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE 
        # Only accessible when compiling the library itself
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
