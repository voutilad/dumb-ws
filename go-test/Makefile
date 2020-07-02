.PHONY: docker deploy clean

TAG = dumb-ws:latest

build: dumb-ws

dumb-ws:
	go build

docker:
	docker build -t $(TAG) .

deploy:
	gcloud app deploy

clean:
	@echo make clean
	rm -f dumb-ws
