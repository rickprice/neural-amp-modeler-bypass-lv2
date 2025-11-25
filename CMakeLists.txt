cmake_minimum_required(VERSION 3.10)

project(NeuralAmpModelerLv2 VERSION 0.1.9)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
set(LIB_INSTALL_DIR ${CMAKE_INSTALL_PREFIX}/lib CACHE PATH "The library install dir (default: lib)")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (CMAKE_SYSTEM_NAME STREQUAL "Darwin")
	include_directories(SYSTEM /usr/local/include)
elseif (CMAKE_SYSTEM_NAME STREQUAL "Linux")
	# stdc++fs will be linked via target_link_libraries
elseif (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
else()
	message(FATAL_ERROR "Unrecognized Platform!")
endif()

# Add DPF as subdirectory
add_subdirectory(deps/DPF)

# Build NeuralAudio dependency
add_subdirectory(deps/NeuralAudio)

# Add plugin sources
add_subdirectory(src)

# Install LV2 bundle
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/NeuralAmpModeler.lv2
       DESTINATION ${LIB_INSTALL_DIR}/lv2
)

set(CPACK_GENERATOR "DEB")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "fprice@pricemail.ca")
set(CPACK_PACKAGE_VENDOR "Frederick Price")
set(CPACK_PACKAGE_DESCRIPTION "Neural Amp Modeler LV2 plugin implementation with bypass functionality")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Neural Amp Modeler LV2 plugin with bypass")
set(CPACK_DEBIAN_PACKAGE_SECTION sound)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_CONTROL_STRICT_PERMISSION TRUE)
set(CPACK_PACKAGING_INSTALL_PREFIX /usr)
set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_DEBIAN_FILE_NAME DEB-DEFAULT)
set (CPACK_STRIP_FILES ON)

include(CPack)
