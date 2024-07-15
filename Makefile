# gdb wrapper makefile

TARGET = gdb_wrapper
OBJECTS = bin/main.o
LIBS    = 

all: $(TARGET)
	size $(TARGET)

$(TARGET): $(OBJECTS)
	gcc $^ $(LIBS) -o $@

bin/%.o: src/%.c
	gcc -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean
