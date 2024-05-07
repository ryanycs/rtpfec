#ifndef GALOIS_H
#define GALOIS_H

#include <stdint.h>

typedef uint8_t GF_ELEMENT;

// Basic galois field operations

void galois_init(int m);
void free_galois();
GF_ELEMENT galois_add(GF_ELEMENT a, GF_ELEMENT b);
GF_ELEMENT galois_sub(GF_ELEMENT a, GF_ELEMENT b);
GF_ELEMENT galois_mul(GF_ELEMENT a, GF_ELEMENT b);
GF_ELEMENT galois_div(GF_ELEMENT a, GF_ELEMENT b);

#endif /* GALOIS_H */
