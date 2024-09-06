
## About

This is a simple RTP server and client implementation. The server sends RTP packets to the client, and the client receives the packets with a certain packet loss rate.

## Build

```bash
make
```

## Usage

### Server

`Usage: ./server <serverIP> <serverPort> <num>`

```bash
# Send 24 source RTP packets to client
./server 0.0.0.0 8080 24
```

### Client

`Usage: ./client <serverIP> <serverPort> <lossRate>`

```bash
# Received packets with 10% packet loss rate
./client 0.0.0.0 8080 10
```