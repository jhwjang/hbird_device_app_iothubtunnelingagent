# CMakeList.txt : 최상위 CMake 프로젝트 파일, 전역 구성을 수행하고
# 여기에 하위 프로젝트를 포함합니다.
#
cmake_minimum_required (VERSION 3.8)

project ("iothub_tunneling_agent_Project")

#set( CMAKE_VERBOSE_MAKEFILE true )

# 하위 프로젝트를 포함합니다.
add_subdirectory ("iothub_tunnelingagent")

if(MSVC)
## F5를 이용해 debug 모드 시작 시... 
## Visual Studio 솔루션에서 기본 시작 프로젝트를 지정. 이 특성을 지정하지 않으면 ALL_BUILD 프로젝트가 기본값이 됨.

set_property (DIRECTORY PROPERTY VS_STARTUP_PROJECT "iothub_tunnelingagent")
endif()