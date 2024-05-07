typedef struct fec fec;
typedef struct fec_op fec_op;

struct fec_op {
    int (*encode)(void *data, int size, void *pkt, int *pkt_size);
    int (*decode)(void *data, int size, void *pkt, int *pkt_size);
};

struct fec {}
    void *data;
    fec_op *op;
};

int fec_create(fec **f, fec_op *op) {
    *f = malloc(sizeof(fec));
    if (!*f) return -1;
    (*f)->data = op->create();
    if (!(*f)->data) {
        free(*f);
        return -1;
    }
    (*f)->op = op;
    return 0;
}

int fec_destroy(fec *f) {
    f->op->destroy(f->data);
    free(f);
    return 0;
}

/*
 * A wapper for fec_op->encode
 */
int fec_encode(fec *f, void *data, int size, void *pkt, int *pkt_size) {
    return f->op->encode(f->data, data, size, pkt, pkt_size);
}

/*
 * A wapper for fec_op->decode
 */
int fec_decode(fec *f, void *data, int size, void *pkt, int *pkt_size) {
    return f->op->decode(f->data, data, size, pkt, pkt_size);
}
