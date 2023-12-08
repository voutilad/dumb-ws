CFLAGS_TLS !=	if [ `uname` = "Darwin" ]; then \
			pkg-config --cflags libtls ;\
		fi
CFLAGS	+= -O2 -Wall -Werror -Wno-padded -Wno-format-nonliteral $(CFLAGS_TLS)

LDFLAGS != 	if [ X"$(OS)" = X"Windows_NT" ]; then \
				echo ${LDFLAGS} -llibretls -lws2_32 ; \
			elif [ `uname` = "Darwin" ]; then \
				pkg-config --libs libtls ; \
			else \
				echo ${LDFLAGS} -ltls ;\
			fi

DWS_OBJ = dws.o
DWS_CLIENT_TEST = client_test

KEYGEN = openssl req -x509 -newkey rsa:4096 -keyout key.pem \
		-out cert.pem -days 30 -nodes -subj "/CN=localhost" \
		-addext "subjectAltName = DNS:localhost, IP: 127.0.0.1"

.PHONY:	all clean

all: $(DWS_OBJ)
$(DWS_OBJ): dws.c dws.h

test-service: certs
	make -C go-test build

test: $(DWS_CLIENT_TEST) test-service
	./test.sh

$(DWS_CLIENT_TEST): client_test.c dws.h $(DWS_OBJ)
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
	make -C go-test clean
