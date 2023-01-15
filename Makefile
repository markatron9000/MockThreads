LDFLAGS := -rdynamic -pthread
CFLAGS := -O2 -std=c++11

OBJ_LINKER = uthread.o \
	main.o \

CROSS_COMPILE =
CC = $(CROSS_COMPILE) g++
CPP = $(CC) -E

BINARY = uthread

all: $(BINARY)

$(BINARY): $(OBJ_LINKER)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CC) $(CFLAGS) -o $@ -c $(filter %.cpp, $^)

clean:
	rm -rf $(OBJ_LINKER) $(BINARY)
