add_library(lib STATIC 
    src/dummy_later_renamed.cpp
    include/dummy_later_renamed.h
)

# Configure include directories 
target_include_directories(lib 
    PUBLIC 
        # Exposes the 'include' directory to any target linking this library
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE 
        # Only accessible when compiling the library itself
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)
