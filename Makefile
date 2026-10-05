CC = gcc

CFLAGS = -Wall -Wextra -fPIC
LIBS = -lsqlite3

BUILD_DIR = build

LIBRARY = libinventory.so
TEST_PROGRAM = inventory_test

LIB_SOURCES = src/database.c \
              src/product.c \
              src/inventory.c \
			  src/user.c \
			  src/location.c

LIB_OBJECTS = $(LIB_SOURCES:src/%.c=$(BUILD_DIR)/%.o)

TEST_OBJECT = $(BUILD_DIR)/main.o

all: $(LIBRARY) $(TEST_PROGRAM)

$(LIBRARY): $(LIB_OBJECTS)
	$(CC) -shared -o $@ $^ $(LIBS)

$(TEST_PROGRAM): $(TEST_OBJECT) $(LIB_OBJECTS)
	$(CC) -o $@ $^ $(LIBS)

$(BUILD_DIR)/%.o: src/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(LIBRARY)
	rm -f $(TEST_PROGRAM)

.PHONY: all clean
