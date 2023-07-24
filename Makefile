#Makefile for diffstub

CC = g++
CFLAGS = -Wall -W -g
SRCS = ngx_diffstub_internal.cpp main.cpp
OBJS = $(SRCS:.cpp=.o)

all: diffstub

diffstub: $(OBJS) cxxopts.hpp
	$(CC) $(CFLAGS) $(SRCS) -o $@

clean:
	rm -f *.o diffstub

