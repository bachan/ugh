CMAKE_MINIMUM_REQUIRED (VERSION 3.0)
PROJECT (ugh VERSION 0.1.15)
INCLUDE (CMakeMacro.cmake)

USE_PACKAGE (ev ev.h)
USE_PACKAGE (Judy Judy.h)


IF (NOT LIB_INSTALL_DIR)
    SET (LIB_INSTALL_DIR lib/)
ENDIF (NOT LIB_INSTALL_DIR)

SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -export-dynamic -no-pie")
SET (EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
AUX_SOURCE_DIRECTORY (ugh/aux SRC_AUX)
ADD_LIBRARY (aux STATIC ${SRC_AUX})
SET_PROPERTY (TARGET aux PROPERTY POSITION_INDEPENDENT_CODE ON)

#ADD_DEFINITIONS (-DCORO_UCONTEXT)
#ADD_LIBRARY (coro ugh/coro/coro.c)

ADD_LIBRARY (coro_ucontext STATIC ugh/coro_ucontext/coro_ucontext.c)
SET_PROPERTY (TARGET coro_ucontext PROPERTY POSITION_INDEPENDENT_CODE ON)

#ADD_EXECUTABLE (test_coro tmp/coro.c)
#TARGET_LINK_LIBRARIES (test_coro coro)
#INSTALL (TARGETS test_coro DESTINATION bin)

ADD_EXECUTABLE (ugh_test_resolver tmp/test_resolver.c)
TARGET_LINK_LIBRARIES (ugh_test_resolver aux)
INSTALL (TARGETS ugh_test_resolver DESTINATION bin)

ADD_EXECUTABLE (test_pool tmp/test_pool.c)
TARGET_LINK_LIBRARIES (test_pool aux)

INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR}/auto)

SET (UGH_VERSION ${PROJECT_VERSION})

AUX_SOURCE_DIRECTORY (ugh SRC_ugh)
ADD_EXECUTABLE (ugh ${SRC_ugh})
SET_PROPERTY (TARGET ugh PROPERTY POSITION_INDEPENDENT_CODE TRUE)
TARGET_LINK_LIBRARIES (ugh aux coro_ucontext ${LIB_Judy} ${LIB_ev} pthread m)

IF (CMAKE_SYSTEM_NAME STREQUAL Linux)
  TARGET_LINK_LIBRARIES (ugh dl)
ENDIF ()

INSTALL (TARGETS ugh DESTINATION bin)
INSTALL_TEMPLATE (ugh/ugh.cfg.in DESTINATION etc)
INSTALL (DIRECTORY ugh DESTINATION include FILES_MATCHING PATTERN "*.h")
INSTALL_TEMPLATE (ugh/autoconf.h.in DESTINATION include/ugh)

# modules

AUX_SOURCE_DIRECTORY (ugh_example SRC_UGH_EXAMPLE)
ADD_LIBRARY (ugh_example MODULE ${SRC_UGH_EXAMPLE})
SET_PROPERTY (TARGET ugh_example PROPERTY POSITION_INDEPENDENT_CODE ON)
INSTALL (TARGETS ugh_example DESTINATION lib/ugh)
INSTALL_TEMPLATE (ugh_example/config.cfg.in DESTINATION etc/ugh_example)

IF (NOT CPACK_GENERATOR)
    SET (CPACK_GENERATOR "RPM" "DEB" "ZIP" "TGZ")
ENDIF (NOT CPACK_GENERATOR)
SET (CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
SET (CPACK_PACKAGE_NAME "ugh")
SET (CPACK_PACKAGE_CONTACT "bachan@yandex.ru")
SET (CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/bachan/ugh/")
SET (CPACK_RPM_PACKAGE_LICENSE "GPL")
SET (CPACK_PACKAGE_RELEASE 1)
SET (CPACK_RPM_COMPONENT_INSTALL 1)
SET (CPACK_RPM_PACKAGE_AUTOREQ "no")
SET (CPACK_RPM_PACKAGE_AUTOREQPROV "no")
SET (CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
SET (CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_PACKAGE_RELEASE}.${CMAKE_SYSTEM_PROCESSOR}")
SET (CPACK_PACKAGE_VENDOR "UGH Team")
INCLUDE (CPack)
