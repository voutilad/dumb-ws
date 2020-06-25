CFLAGS	+= -Wall -Weverything -Werror -Wno-padded
LDFLAGS	+= -ltls

DWS_OBJ = dws.o
DWS_TEST = client_test

.PHONY:	clean certs

$(DWS_OBJ): dws.c dws.h

tests: $(DWS_TEST)
test: tests
	./$(DWS_TEST) -h localhost -p 8000

$(DWS_TEST): client_test.c dws.h $(DWS_OBJ)
	$(CC) $(CFLAGS) -g -O0 client_test.c $(DWS_OBJ) $(LDFLAGS) -o $@ -I.

certs:
	@echo making certs...
	openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365

clean:
	@echo make clean
	rm -f $(DWS_OBJ)
	rm -f $(DWS_TEST)
