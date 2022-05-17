# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)

set(release_name noa-eden-station2)
set(VER_MAJOR 1)
set(VER_MINOR 1)
set(VER_PATCH 1)
set(VER_TWEAK 7)
set(PROJECT_VER ${VER_MAJOR}.${VER_MINOR}.${VER_PATCH}.${VER_TWEAK})

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# project(noa-eden-station2)
string(TIMESTAMP COMPILE_TIME_DATE %Y%m%d)
string(TIMESTAMP COMPILE_TIME_TIME %H%M%S)

project(${release_name}_${VER_MAJOR}_${VER_MINOR}_${VER_PATCH}_${VER_TWEAK}_${COMPILE_TIME_DATE}${COMPILE_TIME_TIME})

# message("version = ${CMAKE_PROJECT_VERSION}")

idf_build_get_property(project_name PROJECT_NAME)
string(SUBSTRING "${project_name}" 0 48 PROJECT_NAME_CUT)
idf_build_get_property(project_ver PROJECT_VER)
string(SUBSTRING "${project_ver}" 0 31 PROJECT_VER_CUT)

message(STATUS "NOA App \"${PROJECT_NAME_CUT}\" version: ${PROJECT_VER_CUT} ${COMPILE_TIME_DATE}${COMPILE_TIME_TIME}")

idf_build_set_property(COMPILE_DEFINITIONS "-D__NOA_APP_BUILD_TIME__=\"${COMPILE_TIME_DATE}${COMPILE_TIME_TIME}\"" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "-D__NOA_APP_VERSION__=\"${PROJECT_VER_CUT}\"" APPEND)
idf_build_set_property(COMPILE_DEFINITIONS "-D__NOA_APP_NAME__=\"${release_name}\"" APPEND)

