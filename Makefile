FLAGS = -O2 -Wall -Wextra
CC = gcc

TARGET_NAME = lykata

SRC  = $(shell find ./src -type f -name *.c)
OBJS = $(SRC:.c=.o)

all: $(TARGET_NAME)


%.o: %.c
	$(CC) $(FLAGS) -c $< -o $@ 

$(TARGET_NAME): $(OBJS)
	$(CC) $(OBJS) -o $@ $(FLAGS) -lncurses -ljson-c

clean:
	rm $(OBJS) $(TARGET_NAME)

.PHONY: all clean

