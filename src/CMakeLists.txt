cmake_minimum_required(VERSION 2.8)

project (cool)
set(CMAKE_BUILD_TYPE Release)

set(cool_INCLUDE_DIRS
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/internal/
        ${CMAKE_SOURCE_DIR}/internal/ccn/
        ${CMAKE_SOURCE_DIR}/internal/encoding/
        ${CMAKE_SOURCE_DIR}/internal/test/
        ${CMAKE_SOURCE_DIR}/test/
    )

include_directories(
        /usr/local/include/
        /Users/cwood/PARC/CCNx_Distillery/usr/include/
        ${cool_INCLUDE_DIRS}
    )

set(cool_SOURCES
        ${CMAKE_SOURCE_DIR}/internal/actor.c
        ${CMAKE_SOURCE_DIR}/internal/actor_message.c
        ${CMAKE_SOURCE_DIR}/internal/buffer.c
        ${CMAKE_SOURCE_DIR}/internal/ccn/ccn_common.c
        ${CMAKE_SOURCE_DIR}/internal/ccn/ccn_fetcher.c
        ${CMAKE_SOURCE_DIR}/internal/ccn/ccn_producer.c
        ${CMAKE_SOURCE_DIR}/internal/channel.c
        ${CMAKE_SOURCE_DIR}/internal/encoding/cJSON.c
        ${CMAKE_SOURCE_DIR}/internal/signal.c
        ${CMAKE_SOURCE_DIR}/mpc.c
        ${CMAKE_SOURCE_DIR}/cool.c
        ${CMAKE_SOURCE_DIR}/cooli.c
    )


link_directories(
        /Users/cwood/PARC/CCNx_Distillery/usr/lib
        /usr/local/lib
    )

add_executable(cool ${cool_SOURCES})

# edit = readline
# gmp = math
# rest = CCNx stuff
target_link_libraries(cool longbow longbow-ansiterm parc ccnx_common ccnx_api_portal ccnx_api_notify ccnx_api_control ccnx_transport_rta gmp edit )

install(TARGETS cool DESTINATION bin)
