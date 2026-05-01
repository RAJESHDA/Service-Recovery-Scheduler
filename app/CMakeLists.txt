# Add source to this project's executable.
add_executable(my_app main.cpp)

# Link the static library target. 
# This automatically handles both compiled library linking and include flags.
target_link_libraries(my_app PRIVATE lib)
