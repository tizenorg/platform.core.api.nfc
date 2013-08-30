CMAKE_MINIMUM_REQUIRED(VERSION 2.6)

SET(Services
        "application"
        "base"
        "content"
        "location"
        "media"
        "messaging"
        "network"
        "social"
        "telephony"
        "system"
)

# project
SET(project_prefix "capi")
SET(service "network")
SET(submodule "nfc")

SET(fw_name "${project_prefix}-${service}-${submodule}")
PROJECT(${fw_name} C)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED dlog glib-2.0 gio-2.0 nfc-common nfc capi-base-common
	vconf ecore-x)
FOREACH(flag ${pkgs_CFLAGS})
    SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} -fPIC -Wall -Werror")
SET(CMAKE_C_FLAGS_DEBUG "-O0 -g")

IF("${ARCH}" STREQUAL "arm")
    ADD_DEFINITIONS("-DTARGET")
ENDIF("${ARCH}" STREQUAL "arm")

ADD_DEFINITIONS("-DPREFIX=\"${CMAKE_INSTALL_PREFIX}\"")

SET(CMAKE_EXE_LINKER_FLAGS "-Wl,--as-needed -Wl,--rpath=${LIB_INSTALL_DIR}")

SET(SOURCES src/nfc.c)

ADD_LIBRARY(${fw_name} SHARED ${SOURCES})
TARGET_LINK_LIBRARIES(${fw_name} ${pkgs_LDFLAGS})
SET_TARGET_PROPERTIES(${fw_name} PROPERTIES VERSION ${FULLVER} SOVERSION ${MAJORVER})

INSTALL(TARGETS ${fw_name} DESTINATION ${LIB_INSTALL_DIR})
INSTALL(
        DIRECTORY include/ DESTINATION include/${service}
        FILES_MATCHING
        PATTERN "*_internal.h" EXCLUDE
        PATTERN "include/*.h"
        )

SET(PC_NAME ${fw_name})
SET(PC_REQUIRED "capi-base-common")
SET(PC_LDFLAGS -l${fw_name})
SET(PREFIX ${CMAKE_INSTALL_PREFIX})
SET(VERSION ${FULLVER})

CONFIGURE_FILE(${fw_name}.pc.in ${fw_name}.pc @ONLY)
INSTALL(FILES ${fw_name}.pc DESTINATION ${LIB_INSTALL_DIR}/pkgconfig)

#ADD_SUBDIRECTORY(test)

IF(UNIX)

ADD_CUSTOM_TARGET (distclean @echo cleaning for source distribution)
ADD_CUSTOM_COMMAND(
        DEPENDS clean
        COMMENT "distribution clean"
        COMMAND find
        ARGS    .
        -not -name config.cmake -and \(
        -name tester.c -or
        -name Testing -or
        -name CMakeFiles -or
        -name cmake.depends -or
        -name cmake.check_depends -or
        -name CMakeCache.txt -or
        -name cmake.check_cache -or
        -name *.cmake -or
        -name Makefile -or
        -name core -or
        -name core.* -or
        -name gmon.out -or
        -name install_manifest.txt -or
        -name *.pc -or
        -name *~ \)
        | grep -v TC | xargs rm -rf
        TARGET  distclean
        VERBATIM
)

ENDIF(UNIX)
