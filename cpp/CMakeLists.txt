include_directories(contrib)

add_executable(rio main.cpp)

if(CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(rio stdc++fs)
endif(CMAKE_COMPILER_IS_GNUCXX)
