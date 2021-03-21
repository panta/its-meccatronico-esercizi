FROM alpine:latest

RUN apk update && apk add build-base

COPY . /src/
WORKDIR /src
RUN gcc -std=c99 -Wall -o worker main.c img.c -lm

ENTRYPOINT ["/src/worker"]
