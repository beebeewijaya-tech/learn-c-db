CFLAGS = -Iinclude -Isrc -Isrc/file -Isrc/common -Isrc/shared -Isrc/parse
TARGET = ./bin/dbview
SRC = $(shell find src -name '*.c')
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))


run: clean default
	./$(TARGET) -f ./mynewdb-db.db -n

default: $(TARGET)

clean: 
	rm -f obj/*.o
	rm -f bin/*
	rm -f *.db

$(TARGET): $(OBJ)
	gcc -o $@ $^

obj/%.o: src/%.c
	mkdir -p $(dir $@)
	gcc -c $< -o $@ $(CFLAGS)
