CFLAGS	+= -O2 -Wall -Werror -Wno-padded -Wno-format-nonliteral

LDFLAGS != 	if [ X"$(OS)" = X"Windows_NT" ]; then \
				echo ${LDFLAGS} -llibretls -lws2_32 ; \
			else \
				echo ${LDFLAGS} -ltls ;\
			fi

DWS_OBJ = dws.o
DWS_TEST = client_test

KEYGEN = openssl req -x509 -newkey rsa:4096 -keyout key.pem \
		-out cert.pem -days 30 -nodes -subj '/CN=localhost'

.PHONY:	all clean

all: $(DWS_OBJ)
$(DWS_OBJ): dws.c dws.h

tests: $(DWS_TEST)
test: tests certs
	@echo ">> testing unencrypted websocket connection to localhost:8000"
	./$(DWS_TEST) -h localhost -p 8000
	@echo ">> testing encrypted websocket connection to localhost:8443"
	./$(DWS_TEST) -t -h localhost -p 8443

$(DWS_TEST): client_test.c dws.h $(DWS_OBJ)
	$(CC) $(CFLAGS) -g -O0 client_test.c $(DWS_OBJ) $(LDFLAGS) -o $@ -I.

.NOTPARALLEL: certs
certs: cert.pem key.pem
cert.pem:
	$(KEYGEN)
key.pem:
	$(KEYGEN)

clean:
	@echo make clean
	rm -f $(DWS_OBJ)
	rm -f $(DWS_TEST)
	rm -f cert.pem key.pem
