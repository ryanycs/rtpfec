#ifndef GALOIS_H
#define GALOIS_H

#include <stdint.h>

typedef uint8_t GF_ELEMENT;

// Basic galois field operations

/*
 * Initialize galois field
 *
 * @param m    power of the galois field (2^m)
 */
void galois_init(int m);

/*
 * Destroy galois field
 */
void galois_destroy();

/*
 * Add two elements in galois field
 *
 * @param a    element a
 * @param b    element b
 * @return     a + b
 */
GF_ELEMENT galois_add(GF_ELEMENT a, GF_ELEMENT b);

/*
 * Subtract two elements in galois field
 *
 * @param a    element a
 * @param b    element b
 * @return     a - b
 */
GF_ELEMENT galois_sub(GF_ELEMENT a, GF_ELEMENT b);

/*
 * Multiply two elements in galois field
 *
 * @param a    element a
 * @param b    element b
 * @return     a * b
 */
GF_ELEMENT galois_mul(GF_ELEMENT a, GF_ELEMENT b);

/*
 * Divide two elements in galois field
 *
 * @param a    element a
 * @param b    element b
 * @return     a / b
 */
GF_ELEMENT galois_div(GF_ELEMENT a, GF_ELEMENT b);

#endif /* GALOIS_H */
