
include_directories(${PROJECT_SOURCE_DIR}
                    ${PROJECT_SOURCE_DIR}/third-party/googletest-main/googletest/include)


add_executable(recovery_scheduler_tests
    recovery_action_gtests.cpp
    recovery_scheduler_gtests.cpp
)

target_link_libraries(recovery_scheduler_tests
    PRIVATE
        librecoveryscheduler
        gtest
        gmock
        gtest_main
        gmock_main
)
