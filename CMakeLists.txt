# Akari Framework - CMake Build System
# Supports standard CMake and ESP-IDF

set(AKARI_SRCS
    "src/akari_core.c"
    "src/akari_event.c"
    "src/akari_http.c"
    "vendor/picohttpparser/picohttpparser.c"
)

set(AKARI_INCLUDES
    "include"
    "vendor/picohttpparser"
    "vendor/jsmn"
)

# ESP-IDF Support
if(IDF_PATH)
    list(APPEND AKARI_SRCS "src/akari_event_poll.c")
    idf_component_register(SRCS ${AKARI_SRCS}
                        INCLUDE_DIRS ${AKARI_INCLUDES}
                        PRIV_REQUIRES esp_timer)
    add_definitions(-DAKARI_USE_POLL)
else()
    # Standard CMake Support
    add_library(akari STATIC ${AKARI_SRCS})
    target_include_directories(akari PUBLIC ${AKARI_INCLUDES})
    
    if(APPLE OR NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
        target_sources(akari PRIVATE "src/akari_event_poll.c")
        target_compile_definitions(akari PUBLIC AKARI_USE_POLL)
    else()
        target_sources(akari PRIVATE "src/akari_event_epoll.c")
    endif()
endif()
