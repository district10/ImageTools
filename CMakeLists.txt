project( ImageTools )
cmake_minimum_required( VERSION 2.8 )

include( CheckCXXCompilerFlag )
check_cxx_compiler_flag( "-std=c++11"   COMPILER_SUPPORTS_CXX11 )
check_cxx_compiler_flag( "-std=c++0x"   COMPILER_SUPPORTS_CXX0X )
if( COMPILER_SUPPORTS_CXX11 )
    if( CMAKE_COMPILER_IS_GNUCXX )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11" )
    else()
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
    endif()
elseif( COMPILER_SUPPORTS_CXX0X)
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x" )
else()
    # MSVC, On by default (if available)
endif()

add_compile_options(-fpermissive)

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

file( GLOB UtilsSrc src/* )
add_library( Utils ${UtilsSrc})

file( GLOB SRCS examples/*.cpp)
foreach( src ${SRCS} )
    string( REGEX REPLACE "(^.*/|.cpp$)" "" exe ${src} )
    message( STATUS "${exe} <-- ${src}" )
    add_executable( ${exe} ${src} )
    target_link_libraries( ${exe} FreeImage VLFeat Utils )
endforeach( src )
