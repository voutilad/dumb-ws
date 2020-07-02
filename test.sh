#!/bin/sh

COMMON="-e -c cert.pem -k key.pem -h localhost"

echo "starting go services..."

./go-test/dumb-ws ${COMMON} -p 8000 &
httpJob="$!"
echo "started http listener ${httpJob}"

./go-test/dumb-ws -t ${COMMON} -p 8443 &
httpsJob="$!"
echo "started https listener ${httpsJob}"

sleep 2

stopJobs() {
    for pid in "$@"; do
        if ! kill ${pid}; then
            echo "Failed to stop job ${pid}"
        else
            echo "Stopped job ${pid}"
        fi
    done
}

echo "running http test..."
if ! ./client_test -h localhost -p 8000; then
    echo "HTTP TEST FAILED! ($?)"
    stopJobs $httpJob $httpsJob
    exit 1
fi

sleep 1

echo "running https (tls) test..."
if ! ./client_test -t -h localhost -p 8443; then
    echo "HTTPS TEST FAILED! ($?)"
    stopJobs $httpJob $httpsJob
    exit 1
fi

stopJobs $httpJob $httpsJob
