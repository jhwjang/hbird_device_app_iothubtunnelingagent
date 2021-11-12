# CMakeList.txt: iothub_tunneling_agent_Project에 대한 CMake 프로젝트, 여기에 소스를 포함하고
# 프로젝트 특정 논리를 정의합니다.
#
cmake_minimum_required (VERSION 3.8)

# source_dir
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})

# JANSSON
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/third_party/include/jansson)

# CURL
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/third_party/include/curl-7.75.0)

# Paho - hummingbird
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/third_party/include/paho_c_include)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/third_party/include/paho_cpp_include)

# HBirdCrypto
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/third_party/include/HBirdCrypto)

# for lib
if(MSVC)
set(THIRD_PARTY_DIR_LIB_DEBUG "${CMAKE_SOURCE_DIR}/third_party/lib/windows/Debug")
set(THIRD_PARTY_DIR_LIB_RELEASE "${CMAKE_SOURCE_DIR}/third_party/lib/windows/Release")

add_compile_definitions(CURL_STATICLIB)
add_compile_definitions(NOMINMAX)
elseif(BUILD_ARM)
set(THIRD_PARTY_DIR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/lib/arm")    
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
else()
set(THIRD_PARTY_DIR_LIB_PATH "${CMAKE_SOURCE_DIR}/third_party/lib/linux")    
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# 이 프로젝트의 실행 파일에 소스를 추가합니다.
set(CONFIG_SOURCE_FILES 
	main.cc
	main_manager.cc main_manager.h
	bridge_manager.cc bridge_manager.h
	api_manager.cc api_manager.h
	mqtt_manager.cc mqtt_manager.h
	mqtt/mqtt_interface.cc mqtt/mqtt_interface.h
	mqtt/hummingbird_01/hummingbird.cpp mqtt/hummingbird_01/hummingbird.h
	mqtt/hummingbird_01/hummingbird_curl_for_command.cpp mqtt/hummingbird_01/hummingbird_curl_for_command.h
	mqtt/hummingbird_01/hummingbird_topic.cpp mqtt/hummingbird_01/hummingbird_topic.h
	broker_manager.cc broker_manager.h
	mqtt_manager_for_broker.cc mqtt_manager_for_broker.h
	mqtt/mqtt_interface_for_broker.cc mqtt/mqtt_interface_for_broker.h
	mqtt/hummingbird_01/hummingbird_for_broker.cpp mqtt/hummingbird_01/hummingbird_for_broker.h
	mqtt/hummingbird_01/hummingbird_topic_for_broker.cpp mqtt/hummingbird_01/hummingbird_topic_for_broker.h
	sunapi_manager.cpp sunapi_manager.h
	rafagafe_base64.cpp rafagafe_base64.h
)

add_executable ( iothub_tunnelingagent
	${CONFIG_SOURCE_FILES}
)


# new
if(MSVC)

## jansson    
add_library(staticlib_jansson STATIC IMPORTED)
set_target_properties(staticlib_jansson PROPERTIES 
         IMPORTED_LOCATION_RELEASE ${THIRD_PARTY_DIR_LIB_RELEASE}/jansson.lib)
set_target_properties(staticlib_jansson PROPERTIES 
         IMPORTED_LOCATION_DEBUG ${THIRD_PARTY_DIR_LIB_DEBUG}/jansson_d.lib)
		 
## crypto
add_library(staticlib_crypto STATIC IMPORTED)
set_target_properties(staticlib_crypto PROPERTIES
          IMPORTED_LOCATION_RELEASE ${THIRD_PARTY_DIR_LIB_RELEASE}/libcrypto.lib)
set_target_properties(staticlib_crypto PROPERTIES
          IMPORTED_LOCATION_DEBUG ${THIRD_PARTY_DIR_LIB_DEBUG}/libcrypto.lib)		  

## ssl
add_library(staticlib_ssl STATIC IMPORTED)
set_target_properties(staticlib_ssl PROPERTIES
          IMPORTED_LOCATION_RELEASE ${THIRD_PARTY_DIR_LIB_RELEASE}/libssl.lib)
set_target_properties(staticlib_ssl PROPERTIES
          IMPORTED_LOCATION_DEBUG ${THIRD_PARTY_DIR_LIB_DEBUG}/libssl.lib)

## curl
add_library(staticlib_curl STATIC IMPORTED)
set_target_properties(staticlib_curl PROPERTIES
          IMPORTED_LOCATION_RELEASE ${THIRD_PARTY_DIR_LIB_RELEASE}/libcurl.lib)
set_target_properties(staticlib_curl PROPERTIES
          IMPORTED_LOCATION_DEBUG ${THIRD_PARTY_DIR_LIB_DEBUG}/libcurl-d.lib)

## paho.mqtt.c.as
add_library(staticlib_c_paho STATIC IMPORTED)
set_target_properties(staticlib_c_paho PROPERTIES
          IMPORTED_LOCATION_RELEASE ${THIRD_PARTY_DIR_LIB_RELEASE}/paho-mqtt3as-static.lib)
set_target_properties(staticlib_c_paho PROPERTIES
          IMPORTED_LOCATION_DEBUG ${THIRD_PARTY_DIR_LIB_DEBUG}/paho-mqtt3as-static.lib)

## paho.mqtt.cxx
add_library(staticlib_cxx_paho STATIC IMPORTED)
set_target_properties(staticlib_cxx_paho PROPERTIES
          IMPORTED_LOCATION_RELEASE ${THIRD_PARTY_DIR_LIB_RELEASE}/paho-mqttpp3-static.lib)
set_target_properties(staticlib_cxx_paho PROPERTIES
          IMPORTED_LOCATION_DEBUG ${THIRD_PARTY_DIR_LIB_DEBUG}/paho-mqttpp3-static.lib)

## HbirdCrypto
add_library(staticlib_HBirdCrypto STATIC IMPORTED)
set_target_properties(staticlib_HBirdCrypto PROPERTIES
          IMPORTED_LOCATION_RELEASE ${THIRD_PARTY_DIR_LIB_RELEASE}/HBirdCrypto_release.lib)
set_target_properties(staticlib_HBirdCrypto PROPERTIES
          IMPORTED_LOCATION_DEBUG ${THIRD_PARTY_DIR_LIB_DEBUG}/HBirdCrypto_debug.lib)

target_link_libraries( iothub_tunnelingagent PUBLIC 
	WS2_32
	Wldap32
	Normaliz
	iphlpapi
	staticlib_jansson staticlib_crypto staticlib_ssl staticlib_curl staticlib_c_paho staticlib_cxx_paho staticlib_HBirdCrypto
)
	
target_compile_options( iothub_tunnelingagent PRIVATE "$<$<CONFIG:Debug>:/MTd>")
target_compile_options( iothub_tunnelingagent PRIVATE "$<$<CONFIG:Release>:/MT>")

set_target_properties( iothub_tunnelingagent
  PROPERTIES
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/lib"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/bin"
)

set_target_properties( iothub_tunnelingagent
  PROPERTIES 
  LINK_FLAGS "/MANIFESTUAC:\"level='requireAdministrator' uiAccess='false'\" "
)

# TODO: 필요한 경우 테스트를 추가하고 대상을 설치합니다.

  set(CMAKE_EXE_LINKER_FLAGS_DEBUG
	"${CMAKE_EXE_LINKER_FLAGS_DEBUG} /NODEFAULTLIB:MSVCRTD")

##################################################################################################

else() ## linux & arm

## jansson
add_library(lib_jansson SHARED IMPORTED)
set_target_properties(lib_jansson PROPERTIES
    IMPORTED_LOCATION ${THIRD_PARTY_DIR_LIB_PATH}/libjansson.so)

## crypto
add_library(lib_crypto SHARED IMPORTED)
set_target_properties(lib_crypto PROPERTIES
    IMPORTED_LOCATION ${THIRD_PARTY_DIR_LIB_PATH}/libcrypto.so)

## ssl
add_library(lib_ssl SHARED IMPORTED)
set_target_properties(lib_ssl PROPERTIES
    IMPORTED_LOCATION ${THIRD_PARTY_DIR_LIB_PATH}/libssl.so)

## curl
add_library(lib_curl SHARED IMPORTED)
set_target_properties(lib_curl PROPERTIES
    IMPORTED_LOCATION ${THIRD_PARTY_DIR_LIB_PATH}/libcurl.so)

## paho.mqtt.c.as
add_library(lib_c_as_paho SHARED IMPORTED)
set_target_properties(lib_c_as_paho PROPERTIES
    IMPORTED_LOCATION ${THIRD_PARTY_DIR_LIB_PATH}/libpaho-mqtt3as.so)

## paho.mqtt.cxx
add_library(lib_cxx_paho SHARED IMPORTED)
set_target_properties(lib_cxx_paho PROPERTIES
    IMPORTED_LOCATION ${THIRD_PARTY_DIR_LIB_PATH}/libpaho-mqttpp3.so)

link_directories("${THIRD_PARTY_DIR_LIB_PATH}")

target_link_libraries( iothub_tunnelingagent PUBLIC
    lib_jansson
    lib_crypto
    lib_ssl
    lib_curl
    lib_c_as_paho
    lib_cxx_paho
)

set_target_properties( iothub_tunnelingagent
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

endif()
