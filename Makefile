# File: Makefile
# Project: IFJ26
# Authors: Alexandr Jordán (xjordaa00)
# Translator: gcc 16.2.1

CC = gcc
CFLAGS = -std=c2x -pedantic -Wall -Wextra -O2 -g
TARGET = ifj26

SRCS = main.c
OBJS = $(SRCS:.c=.o)

.PHONY: all clean format

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

format:
	clang-format -i $$(find . -maxdepth 1 -name "*.c" -o -name "*.h")

clean: 
	rm -f $(TARGET) *.o
