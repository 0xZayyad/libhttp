main:
	gcc -c -g lib/libhttp.c -o bin/libhttp.o
	ar rcs bin/libhttp.a bin/libhttp.o

.PHONY: all clean lib httpc

BIN_DIR=bin
LIB_DIR=lib
TOOLS_DIR=tools

all: lib httpc

lib: $(BIN_DIR)/libhttp.a

$(BIN_DIR)/libhttp.a: $(BIN_DIR)/libhttp.o
	@ar rcs $@ $<

$(BIN_DIR)/libhttp.o: $(LIB_DIR)/libhttp.c $(LIB_DIR)/libhttp.h $(LIB_DIR)/libhttp_version.h | $(BIN_DIR)
	@gcc -c -O2 $(LIB_DIR)/libhttp.c -o $@

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

httpc: $(TOOLS_DIR)/httpc.cpp $(LIB_DIR)/libhttp.hpp $(LIB_DIR)/libhttp.h $(BIN_DIR)/libhttp.a
	@g++ -O2 -std=c++17 -I$(LIB_DIR) $(TOOLS_DIR)/httpc.cpp -L$(BIN_DIR) -lhttp -lssl -lcrypto -o $(BIN_DIR)/httpc

clean:
	rm -f $(BIN_DIR)/libhttp.o $(BIN_DIR)/libhttp.a $(BIN_DIR)/httpc 
