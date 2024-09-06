# rtpfec

## About

This is a Forward Error Correction C library for RTP base on Random Linear Network Coding (RLNC). The library is designed to be used in a real-time communication system, such as video streaming, where packet loss is common. The library provides a simple API to encode and decode RTP packets with FEC packets.

## Build

```bash
make
```

### Add LD_LIBRARY_PATH to environment

If you encounter the following error:

`error while loading shared libraries: libfec.so: cannot open shared object file: No such file or directory`

Make sure to add the library path to the environment variable `LD_LIBRARY_PATH`:

```bash
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:"absolute/path/to/rtpfec/lib"
```

## Usage

### Encode

```c
#include <rtpfec.h>

int main() {
    fec *ctx;
    fec_param param = {
        .gf_power = 8,
        .gen_size = 4,
        .rtp_payload_size = RTP_PAYLOAD_SIZE,
        .packet_num = 8, // total number send in a generation
        .pt = 97 // paload type of RTP repair packets
    };
    fec_packet out_pkts[MAX_PACKET_NUM];
    char rtp_buf[1024] = {0};

    // Initialize FEC context
    if (fec_init(&ctx, &param) < 0) {
        fprintf(stderr, "Failed to initialize fec context\n");
        return 1;
    }

    // prepare RTP packets here

    // Encode RTP packets
    fec_encode(ctx, rtp_buf, RTP_HEADER_SIZE + RTP_PAYLOAD_SIZE, out_pkts, &count);
}
```

### Decode

```c
#include <rtpfec.h>

int main() {
    fec *ctx;
    fec_param param = {
        .gf_power = 8,
        .gen_size = 4,
        .rtp_payload_size = RTP_PAYLOAD_SIZE,
        .packet_num = 8,
        .pt = 97};
    fec_packet out_pkts[MAX_PACKET_NUM];
    char rtp_buf[1024] = {0};

    // Initialize FEC context
    if (fec_init(&ctx, &param) < 0) {
        fprintf(stderr, "Failed to initialize fec context\n");
        return 1;
    }

    // after receiving RTP packets

    // Decode RTP packets
    fec_decode(ctx, rtp_buf, RECEIVED_PACKET_LENGTH, out_pkts, &count);
}
```

For more examples, please refer to the `example` directory.

## License

Distributed under the MIT License. See `LICENSE` for more information.