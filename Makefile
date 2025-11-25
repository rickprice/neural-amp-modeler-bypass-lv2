#!/usr/bin/make -f
# Makefile for Neural Amp Modeler DPF Plugin

# Project name
NAME = NeuralAmpModeler

# Files to build
FILES_DSP = \
	src/NAMPlugin.cpp

FILES_UI = \
	src/NAMUI.cpp

# Include DPF Makefile FIRST
include deps/DPF/Makefile.plugins.mk

# Then set our custom flags
# Include paths - must be absolute or relative to where make is run
BUILD_CXX_FLAGS += -I$(CURDIR)/deps/NeuralAudio
BUILD_CXX_FLAGS += -I$(CURDIR)/deps/denormal
BUILD_CXX_FLAGS += -I$(CURDIR)/deps/NeuralAudio/deps/NeuralAmpModelerCore/Dependencies/nlohmann
BUILD_CXX_FLAGS += -I$(CURDIR)/deps/NeuralAudio/deps/RTNeural

# Library paths and links
LINK_FLAGS += -L$(CURDIR)/deps/NeuralAudio/build/NeuralAudio
LINK_FLAGS += -lNeuralAudio
LINK_FLAGS += -lstdc++fs

# C++ standard
BUILD_CXX_FLAGS += -std=c++20

# Optimization flags
ifeq ($(DEBUG),true)
BUILD_CXX_FLAGS += -O0 -g
else
BUILD_CXX_FLAGS += -O3 -ffast-math
endif

# Architecture-specific optimizations
ifneq (,$(findstring x86_64,$(shell uname -m)))
ifeq ($(NATIVE_ARCH),true)
BUILD_CXX_FLAGS += -march=x86-64-v3
endif
endif

# Disable denormals
BUILD_CXX_FLAGS += -DDISABLE_DENORMALS

# Override all target to build LV2, VST2, VST3, and CLAP
all: lv2_sep vst3 clap

# Build NeuralAudio dependency
.PHONY: deps
deps:
	mkdir -p deps/NeuralAudio/build
	cd deps/NeuralAudio/build && cmake .. && $(MAKE)

# Clean including dependencies
.PHONY: clean-all
clean-all: clean
	rm -rf deps/NeuralAudio/build
