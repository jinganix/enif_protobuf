# Based on c_src.mk from erlang.mk by Loic Hoguin <essen@ninenines.eu>

CURDIR := $(shell pwd)
BASEDIR := $(abspath $(CURDIR)/..)

PROJECT ?= $(notdir $(BASEDIR))
PROJECT := $(strip $(PROJECT))

ERTS_INCLUDE_DIR ?= $(shell erl -noshell -eval "io:format(\"~ts/erts-~ts/include/\", [code:root_dir(), erlang:system_info(version)])." -s erlang halt)
ERL_INTERFACE_INCLUDE_DIR ?= $(shell erl -noshell -eval "io:format(\"~ts\", [code:lib_dir(erl_interface, include)])." -s erlang halt)
ERL_INTERFACE_LIB_DIR ?= $(shell erl -noshell -eval "io:format(\"~ts\", [code:lib_dir(erl_interface, lib)])." -s erlang halt)

C_SRC_DIR = $(CURDIR)
C_SRC_OUTPUT ?= $(CURDIR)/../priv/$(PROJECT).so

NIF_SOURCES = \
	$(C_SRC_DIR)/enif_protobuf.c \
	$(C_SRC_DIR)/ep_cache.c \
	$(C_SRC_DIR)/ep_decoder.c \
	$(C_SRC_DIR)/ep_encoder.c \
	$(C_SRC_DIR)/ep_node.c

# System type and C compiler/flags.

ifeq ($(OS),Windows_NT)
	PRIV_DIR := $(BASEDIR)/priv
	# OTP on Windows is a native PE binary; link the NIF with MinGW-w64, not MSYS libc.
	ifneq ($(wildcard /mingw64/bin/gcc.exe),)
		CC := /mingw64/bin/gcc
	else ifneq ($(wildcard /ucrt64/bin/gcc.exe),)
		CC := /ucrt64/bin/gcc
	else
		CC ?= gcc
	endif
	MINGW_BIN := $(dir $(shell command -v $(CC) 2>/dev/null || echo $(CC)))
	CFLAGS ?= -O3 -std=c99 -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -finline-functions -Wall
	C_SRC_OUTPUT = $(CURDIR)/../priv/$(PROJECT).dll
	CFLAGS += -I "$(ERTS_INCLUDE_DIR)"
	CXXFLAGS += -I "$(ERTS_INCLUDE_DIR)"
	LDFLAGS += -shared -static-libgcc
	SOURCES = $(NIF_SOURCES)
	WIN_RUNTIME_DLLS := libgcc_s_seh-1.dll libwinpthread-1.dll
else
UNAME_SYS := $(shell uname -s)
ifeq ($(UNAME_SYS), Darwin)
    ARCH_SYS := $(shell uname -p)
    ifeq ($(ARCH_SYS), arm)
        ARCH_NAME := arm64
    else
        ARCH_NAME := x86_64
    endif
	CC ?= cc
	CFLAGS ?= -O3 -std=c99 -arch $(ARCH_NAME) -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -arch $(ARCH_NAME) -finline-functions -Wall
	LDFLAGS ?= -arch $(ARCH_NAME) -flat_namespace -undefined dynamic_lookup
else ifeq ($(UNAME_SYS), FreeBSD)
	CC ?= cc
	CFLAGS ?= -O3 -std=c99 -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -finline-functions -Wall
else ifeq ($(UNAME_SYS), Linux)
	CC ?= gcc
	CFLAGS ?= -O3 -std=c99 -finline-functions -Wall -Wmissing-prototypes
	CXXFLAGS ?= -O3 -finline-functions -Wall
endif

CFLAGS += -fPIC -I $(ERTS_INCLUDE_DIR) -I $(ERL_INTERFACE_INCLUDE_DIR)
CXXFLAGS += -fPIC -I $(ERTS_INCLUDE_DIR) -I $(ERL_INTERFACE_INCLUDE_DIR)

LDLIBS += -L $(ERL_INTERFACE_LIB_DIR) -lei -lpthread
LDFLAGS += -shared

SOURCES := $(shell find $(C_SRC_DIR) -type f \( -name "*.c" -o -name "*.C" -o -name "*.cc" -o -name "*.cpp" \) ! -path "*/test/*")
endif

# Verbosity.

c_verbose_0 = @echo " C     " $(?F);
c_verbose = $(c_verbose_$(V))

cpp_verbose_0 = @echo " CPP   " $(?F);
cpp_verbose = $(cpp_verbose_$(V))

link_verbose_0 = @echo " LD    " $(@F);
link_verbose = $(link_verbose_$(V))

OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))

COMPILE_C = $(c_verbose) $(CC) $(CFLAGS) $(CPPFLAGS) -c
COMPILE_CPP = $(cpp_verbose) $(CXX) $(CXXFLAGS) $(CPPFLAGS) -c

$(C_SRC_OUTPUT): $(OBJECTS)
	@mkdir -p $(BASEDIR)/priv/
	$(link_verbose) $(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(C_SRC_OUTPUT)
ifeq ($(OS),Windows_NT)
	@for dll in $(WIN_RUNTIME_DLLS); do \
		if [ -f "$(MINGW_BIN)$$dll" ]; then \
			cp -f "$(MINGW_BIN)$$dll" "$(PRIV_DIR)/"; \
		fi; \
	done
endif

%.o: %.c
	$(COMPILE_C) $(OUTPUT_OPTION) $<

%.o: %.cc
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

%.o: %.C
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

%.o: %.cpp
	$(COMPILE_CPP) $(OUTPUT_OPTION) $<

clean:
	@rm -f $(C_SRC_OUTPUT) $(OBJECTS)
ifeq ($(OS),Windows_NT)
	@rm -f $(BASEDIR)/priv/libgcc_s_seh-1.dll $(BASEDIR)/priv/libwinpthread-1.dll
endif
