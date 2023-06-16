#Makefile for diffstub

CC = g++
CFLAGS = -Wall -W -g
SRCS = main.cpp
OBJS = $(SRCS:.cpp=.o)

all: diffstub

diffstub: $(OBJS) cxxopts.hpp
	$(CC) $(CFLAGS) $(SRCS) -o $@

clean:
	rm -f *.o diffstub

