cmake_minimum_required (VERSION 2.8.11)
project ("mqttsn")

option (CC_MQTTSN_NO_WARN_AS_ERR "Do NOT treat warning as error" OFF)
option (CC_MQTTSN_CLIENT_DEFAULT_LIB "Build and install default variant of MQTT-SN Client library" ON)
option (CC_MQTTSN_BUILD_GATEWAY "Build and install MQTT-SN client library(ies) and applications" ON)
option (CC_MQTTSN_BUILD_PLUGINS "Build and install relevant plugins to CommsChampion suite" OFF)
option (CC_MQTTSN_FULL_SOLUTION "Build and install full solution, including CommsChampion sources." OFF)

if (CMAKE_TOOLCHAIN_FILE AND EXISTS ${CMAKE_TOOLCHAIN_FILE})
    message(STATUS "Loading toolchain from ${CMAKE_TOOLCHAIN_FILE}")
endif()

set (EXTERNALS_DIR "${CMAKE_SOURCE_DIR}/external")

find_package (Git)

if (GIT_FOUND)
    execute_process (
    COMMAND ${GIT_EXECUTABLE} submodule update --init
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif () 

if (NOT CMAKE_CROSSCOMPILING)
    set (CXXTEST_INCLUDE_DIR "${EXTERNALS_DIR}/cxxtest")
    set (CXXTEST_PYTHON_TESTGEN_EXECUTABLE "${CXXTEST_INCLUDE_DIR}/bin/cxxtestgen")
    
    find_package (CxxTest)
    if (CXXTEST_FOUND)
        enable_testing ()
    endif ()
endif ()

if (UNIX AND (NOT CC_MQTTSN_NO_CCACHE))
    find_program(CCACHE_FOUND ccache)
    if(CCACHE_FOUND)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    endif(CCACHE_FOUND)
endif ()    

if (CMAKE_COMPILER_IS_GNUCC)
    set (extra_flags_list
        "--std=c++11"
        "-Wall" "-Wextra" "-Wcast-align" "-Wcast-qual" "-Wctor-dtor-privacy"
        "-Wmissing-include-dirs" "-Wnoexcept" "-Wlogical-op" "-Wstrict-null-sentinel"
        "-Woverloaded-virtual" "-Wredundant-decls" "-Wshadow" "-Wundef" "-Wunused"
        "-Wno-unknown-pragmas" "-fdiagnostics-show-option"
    )
    
    if (NOT CC_MQTTSN_NO_WARN_AS_ERR)
        list (APPEND extra_flags_list "-Werror")
    endif ()
    
    string(REPLACE ";" " " extra_flags "${extra_flags_list}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${extra_flags}")
    
    set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined") 
elseif (MSVC)
    add_definitions( "/wd4503" "/wd4309" "/wd4267" "-D_SCL_SECURE_NO_WARNINGS")
endif ()

set (INSTALL_DIR ${CMAKE_BINARY_DIR}/install)
if (NOT "${CC_MQTTSN_INSTALL_DIR}" STREQUAL "")
    set (INSTALL_DIR "${CC_MQTTSN_INSTALL_DIR}")
endif ()

set (LIB_INSTALL_DIR ${INSTALL_DIR}/lib)
set (BIN_INSTALL_DIR ${INSTALL_DIR}/bin)
set (ETC_INSTALL_DIR ${INSTALL_DIR}/etc)
set (DOC_INSTALL_DIR ${INSTALL_DIR}/doc)
set (INC_INSTALL_DIR ${INSTALL_DIR}/include)
set (PLUGIN_INSTALL_DIR ${INSTALL_DIR}/plugin)


set (DEFAULT_CLIENT_LIB_TGT "cc_mqttsn_client")

if (NOT "${CC_MQTTSN_QT_DIR}" STREQUAL "")
    list (APPEND CMAKE_PREFIX_PATH ${CC_MQTTSN_QT_DIR})
endif ()

set (CC_EXTERNAL FALSE)
set (CC_EXTERNAL_TGT "comms_champion_external")
set (CC_MQTT_EXTERNAL FALSE)
set (CC_MQTT_EXTERNAL_TGT "mqtt_external")

######################################################################

macro (externals install_dir build_cc mqtt_headers)
        include(ExternalProject)
        
        set (cc_tag "v0.10")
        set (cc_main_dir "${CMAKE_BINARY_DIR}/comms_champion")
        set (cc_src_dir "${cc_main_dir}/src")
        set (cc_bin_dir "${cc_main_dir}/build")
        set (cc_mqtt_main_dir "${CMAKE_BINARY_DIR}/mqtt")
        set (cc_mqtt_src_dir "${cc_mqtt_main_dir}/src")
        set (cc_mqtt_bin_dir "${cc_mqtt_main_dir}/build")
        
        if (NOT "${CC_MQTTSN_QT_DIR}" STREQUAL "")
            set (cc_qt_dir_opt -DCC_QT_DIR=${CC_MQTTSN_QT_DIR})
            set (cc_mqtt_qt_dir_opt "-DCC_QT_DIR=${CC_MQTTSN_QT_DIR}")
        endif ()
        
        if (${build_cc})
            set (CC_PLUGIN_LIBRARIES "comms_champion")
            set (CC_COMMS_CHAMPION_FOUND TRUE)
        else ()
            set (ct_lib_only_opt -DCC_COMMS_LIB_ONLY=ON)
        endif ()
    
        ExternalProject_Add(
            "${CC_EXTERNAL_TGT}"
            PREFIX "${cc_bin_dir}"
            STAMP_DIR "${cc_bin_dir}"    
            GIT_REPOSITORY "https://github.com/arobenko/comms_champion.git"
            GIT_TAG "${cc_tag}"
            SOURCE_DIR "${cc_src_dir}"
            CMAKE_ARGS 
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCC_INSTALL_DIR=${install_dir} 
                ${cc_qt_dir_opt} ${ct_lib_only_opt}
            BINARY_DIR "${cc_bin_dir}"
        )
        set (CC_EXTERNAL TRUE)
        set (CC_COMMS_FOUND TRUE)
        
        if (${mqtt_headers})
            set (cc_mqtt_tag "v0.4.1")
            ExternalProject_Add(
                "${CC_MQTT_EXTERNAL_TGT}"
                PREFIX "${cc_mqtt_bin_dir}"
                STAMP_DIR "${cc_mqtt_bin_dir}"    
                GIT_REPOSITORY "https://github.com/arobenko/mqtt.git"
                GIT_TAG "${cc_mqtt_tag}"
                SOURCE_DIR "${cc_mqtt_src_dir}"
                CMAKE_ARGS 
                    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCC_MQTT_INSTALL_DIR=${install_dir} 
                    -DCC_MQTT_LIB_ONLY=ON ${cc_mqtt_qt_dir_opt}
                BINARY_DIR "${cc_mqtt_bin_dir}"
            )
            set (CC_MQTT_EXTERNAL TRUE)
            
            add_dependencies(${CC_MQTT_EXTERNAL_TGT} ${CC_EXTERNAL_TGT})
            set (CC_MQTT_FOUND TRUE)
        endif()
            
        include_directories("${install_dir}/include")
        link_directories("${install_dir}/lib")
        
endmacro()

######################################################################


while (TRUE)
    if (CC_MQTTSN_FULL_SOLUTION)
        externals(${INSTALL_DIR} TRUE TRUE) 
        break()
    endif ()
    
    
    if (EXISTS "${INSTALL_DIR}/cmake/CommsChampionConfig.cmake")
        list (APPEND CMAKE_PREFIX_PATH "${INSTALL_DIR}/cmake")
    endif ()
    
    if (NOT "${CC_MAIN_INSTALL_DIR}" STREQUAL "")
        list (APPEND CMAKE_PREFIX_PATH "${CC_MAIN_INSTALL_DIR}/cmake")
    endif ()
    
    if (NOT "${CC_MQTT_INSTALL_DIR}" STREQUAL "")
        list (APPEND CMAKE_PREFIX_PATH "${CC_MQTT_INSTALL_DIR}/cmake")
    endif ()
    
    find_package(CommsChampion QUIET)
    
    if (NOT CC_COMMS_FOUND)
        set (externals_install "${CMAKE_BINARY_DIR}/ext_install")
        set (build_cc FALSE)
        if (CC_MQTTSN_BUILD_PLUGINS) 
            set (build_cc TRUE)
        endif ()
        
        set (mqtt_headers FALSE)
        if (CC_MQTTSN_BUILD_GATEWAY)
            set (mqtt_headers TRUE)
        endif ()
        
        externals(${externals_install} ${build_cc} ${mqtt_headers})
        break()
    endif ()

    find_package(CommsChampion)
    include_directories(${CC_INCLUDE_DIRS})
        
    if (CC_COMMS_CHAMPION_FOUND)
        link_directories(${CC_PLUGIN_LIBRARY_DIRS})
    endif ()
    
    find_package(CC_Mqtt)
    
    if (CC_MQTT_FOUND)
        include_directories(${CC_MQTT_INCLUDE_DIRS})
    endif ()
    break()
endwhile()

add_subdirectory(protocol)
add_subdirectory(client)
add_subdirectory(gateway)
add_subdirectory(cc_plugin)