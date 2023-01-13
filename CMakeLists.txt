cmake_minimum_required(VERSION 3.0)
project(routn)

##g++ -g -fno-strict-aliasing -O3 -Wall -export-dynamic -Wall -pipe -D_GNU_SOURCE -D_REENTRANT -fPIC -Wno-deprecated -m64 -O example_echosvr.cpp co*.cpp  -lpthread -ldl -L ./lib -lcolib

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -fno-strict-aliasing -export-dynamic -o3 -pipe -g -D_GNU_SOURCE -D_REENTRANT -m64 -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-dev -lssl -lcrypto -L ./lib")

set(PROJECT_SOURCE_DIR /home/rotun-li/routn/)

include_directories(.)
include_directories(/usr/local/include)
link_directories(/usr/local/lib)

ENABLE_LANGUAGE(ASM)
option(BUILD_TEST "ON for complile test" OFF)

find_package(Boost REQUIRED)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif()

find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    include_directories(${ZLIB_INCLUDE_DIR})
endif()

find_package(Protobuf REQUIRED)
if(Protobuf_FOUND)
    include_directories(${Protobuf_INCLUDE_DIR})
endif()

find_library(YAMLCPP yamlcpp)
find_library(PTHREAD pthread)
find_library(OPENSSL openssl)
find_library(CRYPTO crypto)
find_library(Protobuf protobuf)

set(LIB_SRC
    src/rpc/RockProtocol.cpp
    src/rpc/RockStream.cpp
    src/rpc/RockServer.cpp
    src/ns/NameServerProtocol.cpp
    src/ns/NameServerClient.cpp
    src/ns/ns_protobuf.pb.cpp
    src/ns/NameServerModule.cpp
    src/plugins/libco/coctx.cpp
    src/plugins/libco/coctx_swap.S
    src/db/Sqlite.cpp
    src/TcpServer.cpp
    src/Log.cpp
    src/Util.cpp
    src/Config.cpp
    src/Fiber.cpp
    src/FiberSync.cpp
    src/Thread.cpp
    src/Scheduler.cpp
    src/IoManager.cpp
    src/Timer.cpp
    src/Hook.cpp
    src/FdManager.cpp
    src/Address.cpp
    src/Socket.cpp
    src/ByteArray.cpp
    src/Uri.rl.cpp
    src/Daemon.cpp
    src/Env.cpp
    src/Worker.cpp
    src/Library.cpp
    src/Module.cpp
    src/Dns.cpp
    src/JsonUtil.cpp
    src/Protocol.cpp
    src/Application.cpp
    src/Zk_client.cpp
    src/streams/Stream.cpp
    src/streams/SocketStream.cpp
    src/streams/AsyncSocketStream.cpp
    src/streams/ZLIBStream.cpp
    src/streams/ServiceDiscovery.cpp
    src/streams/LoadBalance.cpp
    src/http/HttpSession.cpp
    src/http/HttpConnection.cpp
    src/http/HttpServer.cpp
    src/http/HttpServlet.cpp
    src/http/HTTP.cpp
    src/http/httpclient_parser.rl.cpp
    src/http/http11_parser.rl.cpp 
    src/http/Parser.cpp
    src/http/WSServer.cpp
    src/http/WSSession.cpp
    src/http/WSServlet.cpp
    src/http/WSConnection.cpp
    )

add_library(routn SHARED ${LIB_SRC})
target_link_libraries(routn ssl crypto)

set(LIBS
        routn
        pthread
        yaml-cpp
        dl
        colib
        tbb
        ${ZLIB_LIBRARIES}
        sqlite3
        protobuf
        zookeeper_mt
    )


add_executable(routn-svr src/main.cpp )
add_dependencies(routn-svr routn)
target_link_libraries(routn-svr ${LIBS})

if(BUILD_TEST)

add_executable(test test/test.cpp )
add_dependencies(test routn)
target_link_libraries(test ${LIBS})


add_executable(test_config test/test_config.cpp)
add_dependencies(test_config routn)
target_link_libraries(test_config routn ${LIBS})


add_executable(test_thread test/test_thread.cpp)
add_dependencies(test_thread routn)
target_link_libraries(test_thread routn ${LIBS})

add_executable(test_util test/test_util.cpp)
add_dependencies(test_util routn)
target_link_libraries(test_util routn ${LIBS})

add_executable(test_fiber test/test_fiber.cpp)
add_dependencies(test_fiber routn)
target_link_libraries(test_fiber routn ${LIBS})

add_executable(test_scheduler test/test_scheduler.cpp)
add_dependencies(test_scheduler routn)
target_link_libraries(test_scheduler routn ${LIBS})

add_executable(test_iomanager test/test_iomanager.cpp)
add_dependencies(test_iomanager routn)
target_link_libraries(test_iomanager routn ${LIBS})

add_executable(test_hook test/test_hook.cpp)
add_dependencies(test_hook routn)
target_link_libraries(test_hook routn ${LIBS})

add_executable(test_address test/test_address.cpp)
add_dependencies(test_address routn)
target_link_libraries(test_address routn ${LIBS})

add_executable(test_socket test/test_socket.cpp)
add_dependencies(test_socket routn)
target_link_libraries(test_socket routn ${LIBS})

add_executable(test_byte test/test_byte.cpp)
add_dependencies(test_byte routn)
target_link_libraries(test_byte routn ${LIBS})

add_executable(test_http test/test_http.cpp)
add_dependencies(test_http routn)
target_link_libraries(test_http routn ${LIBS})

add_executable(test_http_parser test/test_http_parser.cpp)
add_dependencies(test_http_parser routn)
target_link_libraries(test_http_parser routn ${LIBS})

add_executable(test_Tcp_server test/test_Tcp_server.cpp)
add_dependencies(test_Tcp_server routn)
target_link_libraries(test_Tcp_server routn ${LIBS})

add_executable(test_http_server test/test_http_server.cpp)
add_dependencies(test_http_server routn)
target_link_libraries(test_http_server routn ${LIBS})

add_executable(test_http_connection test/test_http_connection.cpp)
add_dependencies(test_http_connection routn)
target_link_libraries(test_http_connection routn ${LIBS})

add_executable(test_uri test/test_uri.cpp)
add_dependencies(test_uri routn)
target_link_libraries(test_uri routn ${LIBS})

add_executable(test_daemon test/test_daemon.cpp)
add_dependencies(test_daemon routn)
target_link_libraries(test_daemon routn ${LIBS})

add_executable(test_env test/test_env.cpp)
add_dependencies(test_env routn)
target_link_libraries(test_env routn ${LIBS})

add_executable(test_application test/test_application.cpp)
add_dependencies(test_application routn)
target_link_libraries(test_application routn ${LIBS})

add_executable(test_ws_server test/test_ws_server.cpp)
add_dependencies(test_ws_server routn)
target_link_libraries(test_ws_server routn ${LIBS})

add_executable(test_ws_client test/test_ws_client.cpp)
add_dependencies(test_ws_client routn)
target_link_libraries(test_ws_client routn ${LIBS})

add_executable(ex_echoServer examples/ex_echoServer.cpp)
add_dependencies(ex_echoServer routn)
target_link_libraries(ex_echoServer routn ${LIBS})

add_executable(http_server_demo examples/http_server_demo.cpp)
add_dependencies(http_server_demo routn)
target_link_libraries(http_server_demo routn ${LIBS})

add_executable(test_lru test/test_lru.cpp)
add_dependencies(test_lru routn)
target_link_libraries(test_lru routn ${LIBS})

add_executable(test_timed_cache test/test_timed_cache.cpp)
add_dependencies(test_timed_cache routn)
target_link_libraries(test_timed_cache routn ${LIBS})

add_library(heart SHARED modules/heart/heart_module.cpp)
add_library(chat SHARED modules/chat_room/chat_servlet.cpp modules/chat_room/chat_module.cpp modules/chat_room/chat_protocol.cpp)
add_library(test SHARED test/test_module.cpp)

add_executable(test_sqlite test/test_sqlite.cpp)
add_dependencies(test_sqlite routn)
target_link_libraries(test_sqlite routn ${LIBS})

add_executable(test_nameserver test/test_nameserver.cpp)
add_dependencies(test_nameserver routn)
target_link_libraries(test_nameserver routn ${LIBS})

add_executable(test_rock test/test_rock.cpp)
add_dependencies(test_rock routn)
target_link_libraries(test_rock routn ${LIBS})

add_executable(test_zookeeper test/test_zookeeper.cpp)
add_dependencies(test_zookeeper routn)
target_link_libraries(test_zookeeper routn ${LIBS})

endif()


add_executable(test_service_discovery test/test_service_discovery.cpp)
add_dependencies(test_service_discovery routn)
target_link_libraries(test_service_discovery routn ${LIBS})

add_library(heart SHARED modules/heart/heart_module.cpp)
add_library(chat SHARED modules/chat_room/chat_servlet.cpp modules/chat_room/chat_module.cpp modules/chat_room/chat_protocol.cpp)
add_library(test SHARED test/test_module.cpp)


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
