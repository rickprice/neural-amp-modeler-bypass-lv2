# Build Neural Amp Modeler plugin using DPF
dpf_add_plugin(NeuralAmpModeler
  UI_TYPE opengl
  USE_NANOVG
  USE_FILE_BROWSER
  TARGETS lv2 vst3 clap
  FILES_DSP
      NAMPlugin.cpp
  FILES_UI
      NAMUI.cpp)

# Include directories for both DSP and UI
target_include_directories(NeuralAmpModeler PUBLIC
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/deps/NeuralAudio
  ${CMAKE_SOURCE_DIR}/deps/denormal
)

# Link NeuralAudio library
target_link_libraries(NeuralAmpModeler PUBLIC
  NeuralAudio
)

# Platform-specific libraries
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
  target_link_libraries(NeuralAmpModeler PUBLIC stdc++fs)
endif()

# Compiler flags
if (MSVC)
  target_compile_options(NeuralAmpModeler PUBLIC
    "$<$<CONFIG:DEBUG>:/W4>"
    "$<$<CONFIG:RELEASE>:/O2>"
  )
else()
  target_compile_options(NeuralAmpModeler PUBLIC
    -Wall
    "$<$<CONFIG:DEBUG>:-Og;-ggdb>"
    "$<$<CONFIG:RELWITHDEBINFO>:-Ofast>"
    "$<$<CONFIG:RELEASE>:-Ofast>"
  )
endif()

# Architecture-specific optimizations
if (CMAKE_SYSTEM_PROCESSOR MATCHES "(amd64)|(AMD64)|(x86_64)")
  option(USE_NATIVE_ARCH "Enable architecture-specific optimizations" OFF)

  if (USE_NATIVE_ARCH)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
      target_compile_options(NeuralAmpModeler PUBLIC /arch:AVX2)
      message(STATUS "Enabling /arch:AVX2")
    else()
      target_compile_options(NeuralAmpModeler PUBLIC -march=x86-64-v3)
      message(STATUS "Enabling -march=x86-64-v3")
    endif()
  endif()
endif()

# Disable denormals
option(DISABLE_DENORMALS "Disable floating point denormals" ON)
if (DISABLE_DENORMALS)
  target_compile_definitions(NeuralAmpModeler PUBLIC DISABLE_DENORMALS)
endif()
