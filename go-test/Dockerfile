FROM gcr.io/gcp-runtimes/go1-builder:1.13 as builder

WORKDIR /go/src/app
COPY go.mod go.sum ./
COPY server.go .

RUN /usr/local/go/bin/go build -o app .

# Application image.
FROM gcr.io/distroless/base:latest

COPY --from=builder /go/src/app/app /usr/local/bin/app

ENV PORT=8080
EXPOSE 8080/tcp

CMD ["/usr/local/bin/app"]
