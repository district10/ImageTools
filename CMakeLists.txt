project( PanoAdjust )
cmake_minimum_required( VERSION 2.8 )

configure_file(
    "${PROJECT_SOURCE_DIR}/Configs.h.in"
    "${PROJECT_BINARY_DIR}/Configs.h" )
include_directories( ${CMAKE_BINARY_DIR}/ )

include_directories( ${CMAKE_SOURCE_DIR}/3rdParty )
include_directories( ${CMAKE_SOURCE_DIR}/3rdParty/FreeImage )
include_directories( ${CMAKE_SOURCE_DIR}/3rdParty/VLFeat )
include_directories( ${CMAKE_SOURCE_DIR}/src )

link_directories( ${CMAKE_SOURCE_DIR}/3rdParty/FreeImage )
add_subdirectory( 3rdParty/VLFeat )

file( GLOB SRCS src/* )
add_executable( ${PROJECT_NAME} ${SRCS} )
target_link_libraries( ${PROJECT_NAME} FreeImage VLFeat )
