cmake_minimum_required(VERSION 3.5)

project(MyProject)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  logger
  sth
)

add_library(logger SHARED logger/logger.cpp)
add_library(sth SHARED sth/sth.cpp)
add_library(badguy SHARED badguy/badguy.cpp)

# uncomment to see the difference
# target_link_libraries(sth logger)
# target_link_libraries(badguy logger sth)

add_library(main OBJECT main.cpp)

# all singleton initializations are triggered from 
# dynamic library badguy's _dl_init
add_executable(wrong)
target_link_libraries(wrong main badguy logger sth)

# all singleton initializations are triggered from
# main function
add_executable(right)
target_link_libraries(right main logger sth)

# same code, but changed order of linking, will change the 
# order of destruction
add_executable(right1)
target_link_libraries(right1 main badguy sth logger)

# if initialization is triggered from main function,
# order of linking does not matter
add_executable(right2)
target_link_libraries(right2 main sth logger)