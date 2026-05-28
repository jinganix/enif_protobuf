CURDIR := $(shell pwd)
C_SRC := $(abspath $(CURDIR)/../c_src)
BUILD := $(CURDIR)/build
COVERAGE_DIR := $(CURDIR)/coverage

CC ?= cc
CFLAGS ?= -O0 -g -std=c99 -Wall -Wextra -Wno-unused-parameter
CFLAGS += -I$(C_SRC) -I$(CURDIR)/stub -DEPB_UNIT_TEST

LIB_SRCS := \
	$(C_SRC)/ep_cache.c \
	$(C_SRC)/ep_node.c \
	$(C_SRC)/ep_encoder.c \
	$(C_SRC)/ep_decoder.c \
	$(C_SRC)/enif_protobuf.c

TEST_SRCS := \
	$(CURDIR)/stub/erl_nif_stub.c \
	$(CURDIR)/test_main.c \
	$(CURDIR)/test_ep_cache.c \
	$(CURDIR)/test_ep_node.c \
	$(CURDIR)/test_ep_encoder.c \
	$(CURDIR)/test_ep_decoder.c \
	$(CURDIR)/test_enif_protobuf.c \
	$(CURDIR)/ep_test_schema.c \
	$(CURDIR)/test_integration.c \
	$(CURDIR)/test_wire_codec.c \
	$(CURDIR)/test_codec_types.c \
	$(CURDIR)/test_codec_features.c \
	$(CURDIR)/test_codec_breadth.c \
	$(CURDIR)/test_coverage_gaps.c \
	$(CURDIR)/test_coverage_exhaustive.c

SRCS := $(LIB_SRCS) $(TEST_SRCS)

TARGET := $(CURDIR)/epb_c_tests
COVERAGE_MIN ?= 0.8

ifeq ($(COVERAGE),1)
CFLAGS += --coverage
LDFLAGS += --coverage
endif

obj_basename = $(subst /,_,$(1:$(CURDIR)/%=%))

LIB_OBJS := $(foreach s,$(LIB_SRCS),$(BUILD)/c_src_$(notdir $(s:.c=.o)))
TEST_OBJS := $(foreach s,$(TEST_SRCS),$(BUILD)/tests_$(call obj_basename,$(s:.c=.o)))
OBJS := $(LIB_OBJS) $(TEST_OBJS)

test_obj = $(BUILD)/tests_$(subst /,_,$(1:$(CURDIR)/%=%))

.PHONY: all test clean coverage coverage-check coverage-clean

all: $(TARGET)

$(TARGET): $(OBJS) | $(BUILD)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) -lm

$(BUILD)/c_src_%.o: $(C_SRC)/%.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_stub_erl_nif_stub.o: $(CURDIR)/stub/erl_nif_stub.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_main.o: $(CURDIR)/test_main.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_ep_cache.o: $(CURDIR)/test_ep_cache.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_ep_node.o: $(CURDIR)/test_ep_node.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_ep_encoder.o: $(CURDIR)/test_ep_encoder.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_ep_decoder.o: $(CURDIR)/test_ep_decoder.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_enif_protobuf.o: $(CURDIR)/test_enif_protobuf.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_ep_test_schema.o: $(CURDIR)/ep_test_schema.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_integration.o: $(CURDIR)/test_integration.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_wire_codec.o: $(CURDIR)/test_wire_codec.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_codec_types.o: $(CURDIR)/test_codec_types.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_codec_features.o: $(CURDIR)/test_codec_features.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_codec_breadth.o: $(CURDIR)/test_codec_breadth.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_coverage_gaps.o: $(CURDIR)/test_coverage_gaps.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD)/tests_test_coverage_exhaustive.o: $(CURDIR)/test_coverage_exhaustive.c | $(BUILD)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD):
	mkdir -p $(BUILD)

test: $(TARGET)
	$(TARGET)

coverage: clean
	$(MAKE) COVERAGE=1 all
	$(MAKE) COVERAGE=1 test
	COVERAGE_MIN=$(COVERAGE_MIN) ./coverage_report.sh

coverage-check: coverage
	@echo "Coverage check passed (see $(COVERAGE_DIR)/summary.txt)"

coverage-clean:
	rm -rf $(BUILD) $(COVERAGE_DIR)
	find $(C_SRC) $(CURDIR) \( -name '*.gcno' -o -name '*.gcda' -o -name '*.gcov' \) -delete 2>/dev/null || true

clean: coverage-clean
	rm -f $(TARGET)
