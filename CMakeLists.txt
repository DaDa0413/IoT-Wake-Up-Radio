cmake_minimum_required(VERSION 3.7)
project(IoT_project)

set(CMAKE_CXX_STANDARD 14)

### Server executable
##
add_executable(IoTServer IoTServer.cpp)
### link mysql
##
include_directories(/usr/local/include/mysql)
link_directories(/usr/local/lib/mysql)
target_link_libraries(IoTServer -lmysqlclient)
### link boost library
##
include_directories(/usr/local/include)
link_directories(/usr/local/lib)
target_link_libraries(IoTServer boost_thread boost_system -lpthread)


### Client executable
##
add_executable(IoTClient IoTClient.cpp)