# Makefile
# IFJ26
# Authors: Alexandr Jordán (xjordaa00)
# Translator: gcc 16.2.1

CC = gcc
CFLAGS = -std=c23 -pedantic -Wall -Wextra -O2 -g
TARGET = ifj26

SRCS = main.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean: 
	rm -f $(TARGET)
