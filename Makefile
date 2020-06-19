CFLAGS	+= -c -std=c99 -Wall -Wextra -O0 -g

OBJS	= dws.o

.PHONY:	clean

dws: $(OBJS)
	$(CC) -o $@ $(LDFLAGS) $(OBJS)

all: dws

clean:
	@echo make clean
	rm $(OBJS)
	rm dws
