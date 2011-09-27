PROJECT (ugh)
CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
INCLUDE (CMakeMacro.cmake)

USE_PACKAGE (ev ev.h) 
USE_PACKAGE (m math.h)
USE_PACKAGE (Judy Judy.h)

SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -export-dynamic")

AUX_SOURCE_DIRECTORY (ugh/aux SRC_AUX)
ADD_LIBRARY (aux STATIC ${SRC_AUX})

#ADD_DEFINITIONS (-DCORO_UCONTEXT)
#ADD_LIBRARY (coro ugh/coro/coro.c)

ADD_LIBRARY (coro_ucontext ugh/coro_ucontext/coro_ucontext.c)

#ADD_EXECUTABLE (test_coro tmp/coro.c)
#TARGET_LINK_LIBRARIES (test_coro coro)
#INSTALL (TARGETS test_coro DESTINATION bin)

ADD_EXECUTABLE (test_resolver tmp/test_resolver.c)
TARGET_LINK_LIBRARIES (test_resolver aux)
INSTALL (TARGETS test_resolver DESTINATION bin)

ADD_EXECUTABLE (test_pool tmp/test_pool.c)
TARGET_LINK_LIBRARIES (test_pool aux)

INCLUDE_DIRECTORIES (${PROJECT_BINARY_DIR}/auto)

SET (UGH_VERSION "0.0.1")

AUX_SOURCE_DIRECTORY (ugh SRC_ugh)
ADD_EXECUTABLE (ugh ${SRC_ugh})
TARGET_LINK_LIBRARIES (ugh aux coro_ucontext ${LIB_Judy} ${LIB_ev} ${LIB_m} pthread)

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
INSTALL (TARGETS ugh_example DESTINATION ${LIBDIR}/ugh)
INSTALL_TEMPLATE (ugh_example/config.cfg.in DESTINATION etc/ugh_example)

