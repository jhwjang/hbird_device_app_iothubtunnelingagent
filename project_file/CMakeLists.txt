# CMakeList.txt : 최상위 CMake 프로젝트 파일, 전역 구성을 수행하고
# 여기에 하위 프로젝트를 포함합니다.
#
cmake_minimum_required (VERSION 3.8)

project ("iothub_tunneling_agent_Project")

set( CMAKE_VERBOSE_MAKEFILE true )

######################
## -- DEBUG & RELEASE 

if(WIN32)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_RELWITHDEBINFO
        )
foreach(CompilerFlag ${CompilerFlags})
    if(${CompilerFlag} MATCHES "/MD")
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endif()
    if(${CompilerFlag} MATCHES "_MD")
        string(REPLACE "_MD" "_MT" ${CompilerFlag} "${${CompilerFlag}}")
    endif()

    set(${CompilerFlag} "${${CompilerFlag}}" CACHE STRING "msvc compiler flags" FORCE)
    message("MSVC flags: ${CompilerFlag}:${${CompilerFlag}}")
endforeach()

endif()

######################

# 하위 프로젝트를 포함합니다.
add_subdirectory ("iothub_tunnelingagent")

if(MSVC)
## F5를 이용해 debug 모드 시작 시... 
## Visual Studio 솔루션에서 기본 시작 프로젝트를 지정. 이 특성을 지정하지 않으면 ALL_BUILD 프로젝트가 기본값이 됨.

set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT "iothub_tunnelingagent")
endif()