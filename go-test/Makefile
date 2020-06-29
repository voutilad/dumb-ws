.PHONY: docker deploy

TAG = dumb-ws:latest

docker:
	docker build -t $(TAG) .

deploy:
	gcloud app deploy
