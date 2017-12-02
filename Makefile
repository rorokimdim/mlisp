CC=cc
CFLAGS=-g -Wall -std=c99
LDFLAGS=
LIBS=-ledit

SRC_DIR=./src
BUILD_DIR=./build
INCLUDES=-I$(SRC_DIR)
SRC_LIST=$(wildcard $(SRC_DIR)/*.c)
OBJ_LIST = $(addprefix $(BUILD_DIR)/, $(notdir $(SRC_LIST:.c=.o)))
EXECUTABLE=mlisp

all: $(SRC_LIST) $(EXECUTABLE)

$(EXECUTABLE): $(OBJ_LIST)
	$(CC) $(LDFLAGS) $(LIBS) $(OBJ_LIST) -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

#
# Phony Targets
#

.PHONY: run
run: $(EXECUTABLE)
	./$(EXECUTABLE)

.PHONY: valgrind
valgrind: $(EXECUTABLE)
	valgrind --leak-check=yes ./$(EXECUTABLE)

.PHONY: debug
debug: $(EXECUTABLE)
	lldb ./$(EXECUTABLE)

.PHONY: clean
clean:
	rm -f $(EXECUTABLE)
	rm -rf $(BUILD_DIR)
