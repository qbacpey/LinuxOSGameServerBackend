# 设置CMake版本
cmake_minimum_required(VERSION 3.9.1)

# 创建项目构建目标
project(GameServer VERSION 1.0)

# 设置C++版本
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 设置编译参数
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

# 设置可执行文件输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# 设置库文件输出目录
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 设置可执行文件
set(SOURCES main.cpp)
add_executable(${PROJECT_NAME} ${SOURCES})

# 添加库目标
add_library(room STATIC lib/room/room.cpp)
add_library(semaphore STATIC lib/semaphore/semaphore.cpp)
add_library(websocket STATIC lib/websocket/websocket.cpp)
add_library(server STATIC lib/server/server.cpp)


# 链接库
target_link_libraries(${PROJECT_NAME} PRIVATE room websocket semaphore server)

# 使用系统中的cJSON库
# 由于路径特别，需要在已有CMAKE_MODULE_PATH中添加
set(CMAKE_MODULE_PATH "/usr/local/lib64/cmake/cJSON/;${CMAKE_MODULE_PATH}")
find_package(cJSON REQUIRED)
# 但是这里FOUND变量的名字是一般是直接用的包名
if(${cJSON_FOUND})
    message("CJSON Found")
    # include library headers
    target_include_directories(${PROJECT_NAME} PUBLIC ${CJSON_INCLUDE_DIRS})
    # 这里变量具体是大写（CJSON）还是小写（cJSON）要看配置文件里的定义
    target_link_libraries(${PROJECT_NAME} PUBLIC ${CJSON_LIBRARIES})
else()
    message(FATAL_ERROR "cJSON Not Found")
endif()

# 使用系统中的OpenSSL库
find_package(OpenSSL REQUIRED)
if(${OPENSSL_FOUND})
    message("OpenSSL Found")
    # include library headers
    target_include_directories(${PROJECT_NAME} PUBLIC  ${OPENSSL_INCLUDE_DIR})
    # link OpenSSL library 注意变量的大小写敏感的！
    target_link_libraries(${PROJECT_NAME} PUBLIC ${OPENSSL_LIBRARIES})
else()
    message(FATAL_ERROR "OpenSSL Not Found")
endif()