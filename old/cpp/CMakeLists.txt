include_directories(contrib)

add_executable(rio main.cpp)

if(CMAKE_COMPILER_IS_GNUCXX)
  target_link_libraries(rio pthread stdc++fs)
endif(CMAKE_COMPILER_IS_GNUCXX)
