CFLAGS	+= -Wall -Wstrict-prototypes -Wmissing-prototypes
CFLAGS	+= -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-qual
CFLAGS	+= -Wsign-compare -Werror-implicit-function-declaration
CFLAGS	+= -Werror

DWS_OBJ = dws.o
DWS_TEST = client_test

.PHONY:	clean

$(DWS_OBJ): dws.c dws.h

tests: $(DWS_TEST)
test: tests
	./$(DWS_TEST) localhost 8000

$(DWS_TEST): client_test.c dws.h $(DWS_OBJ)
	$(CC) $(CFLAGS) client_test.c $(DWS_OBJ) $(LDFLAGS) -o $@ -I.

clean:
	@echo make clean
	rm -f $(DWS_OBJ)
	rm -f $(DWS_TEST)
