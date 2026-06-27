CC := gcc

SRC_DIR := src
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

FLAGS := -std=gnu23 -O2 -Wall -Wextra -Wconversion
FLAGS += -D_TIME_BITS=64 -D_FILE_OFFSET_BITS=64
FLAGS += $(shell pkg-config libcurl --cflags)
LIBS := $(shell pkg-config libcurl --libs)

TARGET := $(BUILD_DIR)/stkonline

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(FLAGS) $(LIBS) -o $(TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) -c $< $(FLAGS) $(INCLUDE) -o $@

clean:
	rm -rf $(TARGET) $(BUILD_DIR)
