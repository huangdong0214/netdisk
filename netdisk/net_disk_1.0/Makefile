SRCS:=$(wildcard *.c)
OBJS:=$(patsubst %.c,%.o,$(SRCS))
CC:=gcc
CFLAGS:=-g
ELF:=netdisk_server
$(ELF):$(OBJS)
	gcc -o $@ $^ -lpthread -lmysqlclient -lcrypt

rebuild:clean $(ELF)
clean:
	rm -rf $(OBJS) $(ELF)
