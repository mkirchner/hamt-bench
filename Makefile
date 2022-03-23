BUILD_DIR ?= ./build
SRC_DIRS ?= ./src
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))

PERF_SRCS := \
	../hamt/src/hamt.c \
	../hamt/src/murmur3.c \
	src/hamt/perf.c \
	src/utils.c \
	src/words.c

PERF_OBJS := $(PERF_SRCS:%=$(BUILD_DIR)/%.o)
PERF_DEPS := $(PERF_OBJS:.o=.d)

CPPFLAGS ?= $(INC_FLAGS) -MMD -MP -O3 -Rpass=tailcallelim
#LDFLAGS ?= -luuid

hamt: $(BUILD_DIR)/bench-hamt

$(BUILD_DIR)/bench-hamt: $(PERF_SRCS)
	$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -I../hamt/include $(PERF_SRCS) -o $@ $(LDFLAGS)

## c source
#$(BUILD_DIR)/%.c.o: %.c
#	$(MKDIR_P) $(dir $@)
#	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

MKDIR_P ?= mkdir -p

