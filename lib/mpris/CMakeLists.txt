cmake_minimum_required(VERSION 3.0)
project(mpris C)

add_library(${PROJECT_NAME} STATIC
  ${PROJECT_SOURCE_DIR}/mpris.h
  ${PROJECT_SOURCE_DIR}/mpris.c
  )

find_package(PkgConfig)
pkg_check_modules(GLIB2 REQUIRED glib-2.0)
pkg_check_modules(GIO_UNIX REQUIRED gio-unix-2.0)

find_program(GDBUS_CODEGEN gdbus-codegen)
if (${GDBUS_CODEGEN} STREQUAL GDBUS_CODEGEN-NOTFOUND)
  message(SEND_ERROR "Could not find gdbus-codegen program")
endif()

add_custom_command(
  OUTPUT ${PROJECT_SOURCE_DIR}/mpris.c ${PROJECT_SOURCE_DIR}/mpris.h
  COMMAND ${GDBUS_CODEGEN} --generate-c-code ${PROJECT_SOURCE_DIR}/mpris
#                           --interface org.mpris
                           --c-namespace Polybar
                           ${PROJECT_SOURCE_DIR}/mpris.xml
  MAIN_DEPENDENCY mpris.xml
  )

target_include_directories(${PROJECT_NAME} PUBLIC
  ${PROJECT_SOURCE_DIR}
  ${GLIB2_INCLUDE_DIRS}
  ${GIO_UNIX_INCLUDE_DIRS}
  )
target_link_libraries(${PROJECT_NAME}
  ${GLIB2_LIBRARIES}
  ${GIO_UNIX_LIBRARIES}
  )

get_directory_property(HAS_PARENT PARENT_DIRECTORY)
if(HAS_PARENT)
  set(MPRIS_LIBRARIES ${PROJECT_NAME} PARENT_SCOPE)
  set(MPRIS_INCLUDE_DIRS ${PROJECT_SOURCE_DIR})
endif()
