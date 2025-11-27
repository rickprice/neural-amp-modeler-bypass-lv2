if (MSVC)
	add_compile_options(
		"$<$<CONFIG:DEBUG>:/W4>"
		"$<$<CONFIG:RELEASE>:/O2>"
	)
else()
	add_compile_options(
		-Wall 
		# -Wpedantic -Wextra -Wstrict-aliasing -Wunreachable-code -Weffc++ -Wno-unused-parameter
		"$<$<CONFIG:DEBUG>:-Og;-ggdb>"
		"$<$<CONFIG:RELWITHDEBINFO>:-Ofast>"
		"$<$<CONFIG:RELEASE>:-Ofast>"
	)
endif()


if (CMAKE_SYSTEM_PROCESSOR MATCHES "(amd64)|(AMD64)|(x86_64)")
	option(USE_NATIVE_ARCH "Enable architecture-specific optimizations" OFF)

	if (USE_NATIVE_ARCH)
		if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
			add_compile_options(/arch:AVX2)
			message(STATUS "Enabling /arch:AVX2")
		 
		else()
			add_compile_options(-march=x86-64-v3)
			message(STATUS "Enabling -march=x86-64-v3")
		endif()
	endif (USE_NATIVE_ARCH)
endif ()

add_subdirectory(../deps/NeuralAudio NeuralAudio)

# Pugl for UI
set(PUGL_SOURCES
	../deps/pugl/src/common.c
	../deps/pugl/src/internal.c
	../deps/pugl/src/x11.c
	../deps/pugl/src/x11_cairo.c
)

# Plugin sources
set(SOURCES nam_lv2.cpp
	nam_plugin.cpp
	nam_plugin.h)

set(NA_SOURCES ../deps/NeuralAudio/NeuralAudio/NeuralModel.h)

# UI sources
set(UI_SOURCES
	nam_ui.cpp
	nam_ui.h
	nam_ui_lv2.cpp
)

add_library(neural_amp_modeler_bypass SHARED ${SOURCES} ${NA_SOURCES})

target_include_directories(neural_amp_modeler_bypass PUBLIC ../deps/NeuralAudio)
target_include_directories(neural_amp_modeler_bypass PUBLIC ../deps/lv2/include)
target_include_directories(neural_amp_modeler_bypass PUBLIC ../deps/denormal)

target_link_libraries(neural_amp_modeler_bypass PRIVATE NeuralAudio)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${SOURCES})
source_group(NAM ${CMAKE_CURRENT_SOURCE_DIR} FILES ${NA_SOURCES})

option(DISABLE_DENORMALS "Disable floating point denormals" ON)

if (DISABLE_DENORMALS)
	add_definitions(-DDISABLE_DENORMALS)
endif (DISABLE_DENORMALS)

set_target_properties(neural_amp_modeler_bypass
	PROPERTIES
	CXX_VISIBILITY_PRESET hidden
	C_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON
	INTERPROCEDURAL_OPTIMIZATION TRUE
	PREFIX ""
)

# Use version script to only export lv2_descriptor symbol
target_link_options(neural_amp_modeler_bypass PRIVATE "-Wl,--version-script=${CMAKE_SOURCE_DIR}/lv2_symbol_export.version")

# Platform

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
	target_compile_definitions(neural_amp_modeler_bypass PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

# Build UI library
add_library(neural_amp_modeler_bypass_ui SHARED ${UI_SOURCES} ${PUGL_SOURCES})

target_include_directories(neural_amp_modeler_bypass_ui PUBLIC ../deps/pugl/include)
target_include_directories(neural_amp_modeler_bypass_ui PUBLIC ../deps/lv2/include)

# Find required UI libraries
find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

target_include_directories(neural_amp_modeler_bypass_ui PUBLIC ${CAIRO_INCLUDE_DIRS})
target_link_libraries(neural_amp_modeler_bypass_ui PRIVATE ${CAIRO_LIBRARIES})
target_link_libraries(neural_amp_modeler_bypass_ui PRIVATE X11 Xcursor Xrandr)

set_target_properties(neural_amp_modeler_bypass_ui
	PROPERTIES
	CXX_VISIBILITY_PRESET hidden
	C_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON
	INTERPROCEDURAL_OPTIMIZATION TRUE
	PREFIX ""
)
