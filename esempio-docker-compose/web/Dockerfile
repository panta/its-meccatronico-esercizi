FROM golang:alpine

ENV GO111MODULE=on

RUN mkdir /app
WORKDIR /app
COPY . /app/

RUN GO111MODULE=on go mod vendor
RUN go build -o server server.go

ENTRYPOINT [ "/app/server" ]
