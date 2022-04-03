BUILD_DIR ?= ./build
SRC_DIRS ?= ./src
INC_DIRS := $(shell find $(SRC_DIRS) -type d)
INC_FLAGS := $(addprefix -I,$(INC_DIRS))
LIB_DIRS := $(shell find $(SRC_DIRS) -type d)
LIB_FLAGS := $(addprefix -L,$(LIB_DIRS))

HAMT_BENCH_SRCS := \
	lib/hamt/src/hamt.c \
	lib/hamt/src/murmur3.c \
	src/hamt/bench.c \
	src/utils.c \
	src/words.c

HAMT_BENCH_OBJS := $(HAMT_BENCH_SRCS:%=$(BUILD_DIR)/%.o)
HAMT_BENCH_DEPS := $(HAMT_BENCH_OBJS:.o=.d)

GLIB_BENCH_SRCS := \
	src/glib/bench.c \
	src/utils.c \
	src/words.c

HSEARCH_BENCH_SRCS := \
	src/hsearch/bench.c \
	src/utils.c \
	src/words.c

HAMT_PROFILE_SRCS := \
	lib/hamt/src/hamt.c \
	lib/hamt/src/murmur3.c \
	src/hamt/profile.c \
	src/utils.c \
	src/words.c

HAMT_PROFILE_OBJS := $(HAMT_PROFILE_SRCS:%=$(BUILD_DIR)/%.o)
HAMT_PROFILE_DEPS := $(HAMT_PROFILE_OBJS:.o=.d)

CCFLAGS ?= -MMD -MP -O3 -g # -Rpass=tailcallelim

all: hamt glib hsearch

profile: $(BUILD_DIR)/profile-hamt

hamt: $(BUILD_DIR)/bench-hamt

glib: $(BUILD_DIR)/bench-glib

hsearch: $(BUILD_DIR)/bench-hsearch

$(BUILD_DIR)/bench-hamt: $(HAMT_BENCH_SRCS)
	$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CCFLAGS) $(CFLAGS) $(INC_FLAGS) -Ilib/hamt/include $(HAMT_BENCH_SRCS) -o $@ $(LDFLAGS) $(LIB_FLAGS) -lgc

$(BUILD_DIR)/bench-glib: $(GLIB_BENCH_SRCS)
	$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CCFLAGS) -o $@ $(GLIB_BENCH_SRCS) `pkg-config --cflags --libs glib-2.0`

$(BUILD_DIR)/bench-hsearch: $(HSEARCH_BENCH_SRCS)
	$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CCFLAGS) $(CFLAGS) $(INC_FLAGS) $(HSEARCH_BENCH_SRCS) -o $@ $(LDFLAGS) $(LIB_FLAGS)

$(BUILD_DIR)/profile-hamt: $(HAMT_PROFILE_SRCS)
	$(MKDIR_P) $(BUILD_DIR)
	$(CC) $(CCFLAGS) $(CFLAGS) $(INC_FLAGS) -Ilib/hamt/include $(HAMT_PROFILE_SRCS) -o $@ $(LDFLAGS) $(LIB_FLAGS) -lgc `pkg-config --libs libprofiler`


## c source
#$(BUILD_DIR)/%.c.o: %.c
#	$(MKDIR_P) $(dir $@)
#	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	$(RM) -r $(BUILD_DIR)

MKDIR_P ?= mkdir -p

