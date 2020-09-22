cmake_minimum_required(VERSION 3.6)
project(tzspd)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -pedantic -D_GNU_SOURCE")

set(SOURCE_FILES
        addr-list.c
        addr-list.h
        tzspd.c
        tzsp-decap.c
        tzsp-decap.h
        utils.c
        utils.h)

set(SOURCE_FILES_MINGW
        win32.c
        win32.h)
        
set(LIBRARIES_MINGW
        ws2_32)
        
if(MINGW)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mconsole")
    add_executable(tzspd ${SOURCE_FILES} ${SOURCE_FILES_MINGW})
    target_link_libraries(tzspd ${LIBRARIES} ${LIBRARIES_MINGW})
ELSE()
    add_executable(tzspd ${SOURCE_FILES})
    install(TARGETS tzspd DESTINATION bin)
ENDIF()

