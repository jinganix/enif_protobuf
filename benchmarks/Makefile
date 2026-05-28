# Benchmarks for enif_protobuf vs gpb (same harness as gpb/benchmarks).
#
# Prerequisites: Erlang/OTP, gpb (via parent test profile or GPB_PATH).
#
# Quick start:
#   make build              # compile parent + benchmark beams
#   make gpb-benchmarks     # pure Erlang gpb (records)
#   make epb-benchmarks     # enif_protobuf NIF
#   make benchmarks         # run both backends
#   make update-readme      # run benchmarks and refresh README.md files

ROOT       ?= ..
TOOL_SH    := $(abspath $(ROOT)/scripts/with_tool_versions.sh)
REBAR      ?= $(TOOL_SH) $(ROOT)/rebar3
GPB_PATH   ?= $(ROOT)/_build/test/lib/gpb
GPB_EBIN   ?= $(if $(wildcard $(GPB_PATH)/ebin/gpb_compile.beam),$(GPB_PATH)/ebin,$(abspath ../gpb/ebin))
GPB_INCLUDE ?= $(abspath $(GPB_EBIN)/../include)

EPB_EBIN ?= $(ROOT)/_build/default/lib/enif_protobuf/ebin

# Erlang from repo .tool-versions (asdf/mise); override only if needed.
erl     ?= $(shell $(TOOL_SH) --erl)
erlc    ?= $(shell $(TOOL_SH) --erlc)
escript ?= $(shell $(TOOL_SH) --escript)
ERLANG_VERSION ?= $(shell $(TOOL_SH) --erlang-version)
ERL_ROOT ?= $(shell $(erl) -noshell -eval 'io:format("~s",[code:root_dir()]), halt().')
ERL_NIF_CFLAGS ?= -I$(ERL_ROOT)/usr/include

ifdef HIPE
  HIPE_ERLC_OPTS := +'native' +'{hipe,[o3]}'
else
  HIPE_ERLC_OPTS :=
endif

PROTOBUF_CFLAGS  ?= $(shell pkg-config --cflags protobuf 2>/dev/null)
PROTOBUF_LDFLAGS ?= $(shell pkg-config --libs protobuf 2>/dev/null)
PROTOC           ?= $(shell which protoc 2>/dev/null)
CXXSTD           ?= -std=c++17
UNAME_S          ?= $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
NIF_LDFLAGS      ?= -undefined dynamic_lookup
else
NIF_LDFLAGS      ?=
endif

.PHONY: all build clean \
	benchmarks gpb-benchmarks gpb-nif-benchmarks epb-benchmarks benchmarks-quick \
	maps-benchmarks erl-benchmarks d-benchmarks nif-benchmarks \
	update-readme show-hw-sw-info

all: build

build: check-erlang parent-compile benchmarks-gr-code

check-erlang:
	@$(TOOL_SH) --check

parent-compile: check-erlang
	@test -x $(ROOT)/rebar3 || (echo "Missing $(ROOT)/rebar3; run 'make -C $(ROOT)' first." && exit 1)
	$(REBAR) compile
	@test -f $(GPB_EBIN)/gpb_compile.beam || $(REBAR) as test compile

benchmarks-erl-code: tmp/msg_r.beam tmp/d_r.beam \
		tmp/msg_mo.beam tmp/d_mo.beam \
		tmp/msg_mp.beam tmp/d_mp.beam

benchmarks-gr-code: tmp/msg_r.beam

benchmarks-maps-mo-code: tmp/msg_mo.beam

benchmarks-nif-code: tmp/msg.beam tmp/msg.nif.so tmp/d.beam tmp/d.nif.so

# Standard Google benchmark messages (records), both backends
benchmarks: build
	@$(MAKE) gpb-benchmarks
	@$(MAKE) gpb-nif-benchmarks
	@$(MAKE) epb-benchmarks

benchmarks-quick: build
	@$(MAKE) BENCH_TARGET_TIME=3 gpb-benchmarks
	@$(MAKE) BENCH_TARGET_TIME=3 gpb-nif-benchmarks
	@$(MAKE) BENCH_TARGET_TIME=3 epb-benchmarks

gpb-benchmarks: build
	@$(call show-hw-sw-info)
	@echo
	@echo "=== gpb (pure Erlang, records) ==="
	@echo
	$(PROTO_BENCH) --backend gpb \
	  msg_r Message1 google_message1.dat \
	  msg_r Message2 google_message2.dat

gpb-nif-benchmarks: benchmarks-nif-code
	@echo
	@echo "=== gpb NIF (libprotobuf, standard messages) ==="
	@echo
	$(PROTO_BENCH) --backend gpb \
	  msg Message1 google_message1.dat \
	  msg Message2 google_message2.dat

epb-benchmarks: build
	@echo
	@echo "=== enif_protobuf (NIF) ==="
	@echo
	$(PROTO_BENCH) --backend epb \
	  msg_r Message1 google_message1.dat \
	  msg_r Message2 google_message2.dat

maps-benchmarks: benchmarks-maps-mo-code parent-compile
	@$(call show-hw-sw-info)
	@echo
	$(PROTO_BENCH) --backend gpb \
	  msg_mo Message1 google_message1.dat \
	  msg_mo Message2 google_message2.dat
	@echo
	$(PROTO_BENCH) --backend epb \
	  msg_mo Message1 google_message1.dat \
	  msg_mo Message2 google_message2.dat

erl-benchmarks: benchmarks-erl-code parent-compile
	@$(call show-hw-sw-info)
	@echo
	$(PROTO_BENCH) --backend gpb \
	  --echo "-- Records (gpb) --" \
	  msg_r  Message1 google_message1.dat \
	  msg_r  Message2 google_message2.dat \
	  --echo "-- Maps (omitted, gpb) --" \
	  msg_mo Message1 google_message1.dat \
	  msg_mo Message2 google_message2.dat \
	  --echo "-- Maps (present_undefined, gpb) --" \
	  msg_mp Message1 google_message1.dat \
	  msg_mp Message2 google_message2.dat
	@echo
	$(PROTO_BENCH) --backend epb \
	  --echo "-- Records (enif_protobuf) --" \
	  msg_r  Message1 google_message1.dat \
	  msg_r  Message2 google_message2.dat \
	  --echo "-- Maps (omitted, enif_protobuf) --" \
	  msg_mo Message1 google_message1.dat \
	  msg_mo Message2 google_message2.dat \
	  --echo "-- Maps (present_undefined, enif_protobuf) --" \
	  msg_mp Message1 google_message1.dat \
	  msg_mp Message2 google_message2.dat

d-benchmarks: benchmarks-erl-code parent-compile
	@$(call show-hw-sw-info)
	@echo
	$(PROTO_BENCH) --backend gpb \
	  --echo "-- Records (gpb) --" \
	  d_r  dm1 --multi d-msgs/d-msg-n*.dat --end-multi \
	  d_r  dm1 d-msgs/d-all-concatenated.dat
	@echo
	$(PROTO_BENCH) --backend epb \
	  --echo "-- Records (enif_protobuf) --" \
	  d_r  dm1 --multi d-msgs/d-msg-n*.dat --end-multi \
	  d_r  dm1 d-msgs/d-all-concatenated.dat

nif-benchmarks: benchmarks-nif-code
	@$(call show-hw-sw-info)
	@echo
	@echo "=== gpb NIF (libprotobuf) ==="
	@echo
	$(PROTO_BENCH) --backend gpb \
	  msg  Message1 google_message1.dat \
	  msg  Message2 google_message2.dat \
	  d dm1 --multi d-msgs/d-msg-n*.dat --end-multi \
	  d dm1 d-msgs/d-all-concatenated.dat

update-readme: build
	@./update_readme.sh

show-hw-sw-info:
	@$(call show-hw-sw-info)

PROTO_BENCH = $(TOOL_SH) ./proto-bench

define show-hw-sw-info
	echo "tool-versions erlang: $(ERLANG_VERSION)"; \
	echo "erl: $(erl)"; \
	echo "System"; \
	uname -srvm 2>/dev/null || uname -a; \
	if [ -r /proc/cpuinfo ]; then \
	  echo "CPU info"; \
	  egrep '^model name' /proc/cpuinfo 2>/dev/null | head -1; \
	  egrep '^cache size' /proc/cpuinfo 2>/dev/null | head -1; \
	  printf 'cores/threads   : %s\n' $$(egrep -c '^processor' /proc/cpuinfo 2>/dev/null); \
	  egrep '^bogomips' /proc/cpuinfo 2>/dev/null | head -1; \
	elif command -v sysctl >/dev/null 2>&1; then \
	  echo "CPU info"; \
	  sysctl -n machdep.cpu.brand_string 2>/dev/null; \
	  sysctl -n hw.physicalcpu 2>/dev/null | awk '{printf "physical cpus  : %s\n", $$1}'; \
	  sysctl -n hw.logicalcpu 2>/dev/null | awk '{printf "logical cpus   : %s\n", $$1}'; \
	fi; \
	echo; \
	$(erl) -noshell -eval 'io:format("OTP release ~s~n", [erlang:system_info(otp_release)]), io:format("~s~n", [erlang:system_info(system_version)]), halt().'
endef

gpb-compile = \
	$(erl) -boot start_clean -pa $(GPB_EBIN) -noshell -noinput +B \
		-v never -c false -I`pwd` -o tmp \
		-modsuffix $1 \
		$(2) \
		-s gpb_compile c $(3)

tmp/%_r.erl: %.proto
	@[ -d tmp ] || mkdir tmp
	$(call gpb-compile,_r,,$<)

tmp/%_mo.erl: %.proto
	@[ -d tmp ] || mkdir tmp
	$(call gpb-compile,_mo,-maps -maps_unset_optional omitted,$<)

tmp/%_mp.erl: %.proto
	@[ -d tmp ] || mkdir tmp
	$(call gpb-compile,_mp,-maps -maps_unset_optional present_undefined,$<)

tmp/%.beam: tmp/%.erl
	cd tmp && $(erlc) -Wall $(HIPE_ERLC_OPTS) -I$(GPB_INCLUDE) +debug_info \
		$(patsubst tmp/%,%,$<)

tmp/%.nif.so: %.proto
	@[ -d tmp ] || mkdir tmp
	$(PROTOC) --cpp_out=tmp/ $*.proto
	$(CXX) -g -fPIC -O3 $(CXXSTD) $(PROTOBUF_CFLAGS) $(CXXFLAGS) \
	  -o tmp/$*.pb.o -c tmp/$*.pb.cc
	$(CXX) -g -fPIC -Wall -O3 $(CXXSTD) $(ERL_NIF_CFLAGS) $(PROTOBUF_CFLAGS) $(CXXFLAGS) \
	  -o tmp/$*.nif.o -c tmp/$*.nif.cc
	$(CXX) -g -fPIC -shared -O3 -Wall $(NIF_LDFLAGS) $(CXXSTD) $(PROTOBUF_CFLAGS) $(CXXFLAGS) \
	  -o $@ tmp/$*.nif.o tmp/$*.pb.o $(PROTOBUF_LDFLAGS)

tmp/%.erl: %.proto
	@[ -d tmp ] || mkdir tmp
	$(call gpb-compile,"",-nif,$<)

clean:
	$(RM) -r tmp
