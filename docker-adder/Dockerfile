FROM alpine:latest

RUN apk update && apk add build-base

COPY adder.c /src/
WORKDIR /src
RUN gcc -Wall -o adder adder.c

ENTRYPOINT ["/src/adder"]
