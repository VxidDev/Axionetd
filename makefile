CC = gcc

CFLAGS = -Wall -O0 -Iinclude

SRC_DIR = src
BUILD_DIR = build
LIB = libaxionetd.a

C_SRC = $(shell find $(SRC_DIR) -name '*.c')

OBJ = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(C_SRC)) 

all: $(LIB)

$(LIB): $(OBJ)
	ar rcs $@ $(OBJ)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(LIB)

