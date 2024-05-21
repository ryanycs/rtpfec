#include <stdio.h>
#include <stdlib.h>

#include <galois.h>

static int field_size;
static int *alpha;     // power representation of galois field
static int *alpha_inv; // mapping integer to power representation
static GF_ELEMENT **galois_mul_table;
static GF_ELEMENT **galois_div_table;
void galois_create_mul_table();
void galois_create_div_table();

static int primitive_poly[] = {
    0,   // 0
    1,   // 1
    7,   // x^2 + x + 1
    11,  // x^3 + x + 1
    19,  // x^4 + x + 1
    37,  // x^5 + x^2 + 1
    67,  // x^6 + x + 1
    137, // x^7 + x^3 + 1
    285  // x^8 + x^4 + x^3 + x^2 + 1
};

void galois_init(int m) {
    if (m < 1 || m > 8) {
        fprintf(stderr, "galois_init: m should be in [1, 8]\n");
        exit(1);
    }

    field_size = 1 << m;

    /* Allocate memory for galois_mul_table and galois_div_table */
    galois_mul_table = (GF_ELEMENT **)malloc(field_size * sizeof(GF_ELEMENT *));
    galois_div_table = (GF_ELEMENT **)malloc(field_size * sizeof(GF_ELEMENT *));
    for (int i = 0; i < field_size; ++i) {
        galois_mul_table[i] = (GF_ELEMENT *)malloc(field_size * sizeof(GF_ELEMENT));
        galois_div_table[i] = (GF_ELEMENT *)malloc(field_size * sizeof(GF_ELEMENT));
    }

    /* Allocate memory for alpha and alpha_inv */
    alpha = (int *)malloc(field_size * sizeof(int));
    alpha_inv = (int *)malloc(field_size * sizeof(int));

    /* initialize alpha and alpha_inv */
    alpha[0] = 1;     // alpha^0 = 1
    alpha_inv[1] = 0; // 1 = alpha^0

    /* construct galois field */
    for (int i = 1; i < field_size - 1; ++i) {
        alpha[i] = alpha[i - 1] << 1; // alpha^i = alpha^(i - 1) * alpha

        /* if i >= m, then alpha[i] >= field_size */
        if (alpha[i] >= field_size) {
            alpha[i] ^= primitive_poly[m];
        }
        /* record the mapping from integer to power representation */
        alpha_inv[alpha[i]] = i;
    }

    galois_create_mul_table();
    galois_create_div_table();
}

/*
 * Create multiplication table for galois field
 * galois_mul_table[i][j] = i * j = (alpha_inv[i] + alpha_inv[j]) % (field_size - 1)
 */
void galois_create_mul_table() {
    for (int i = 0; i < field_size; ++i) {
        for (int j = 0; j < field_size; ++j) {
            if (i == 0 || j == 0) {
                galois_mul_table[i][j] = 0;
            } else {
                int index = (alpha_inv[i] + alpha_inv[j]) % (field_size - 1);
                galois_mul_table[i][j] = alpha[index];
            }
        }
    }
}

/*
 * Create division table for galois field
 * galois_div_table[i][j] = i / j = (alpha_inv[i] - alpha_inv[j]) % (field_size - 1)
 */
void galois_create_div_table() {
    for (int i = 0; i < field_size; ++i) {
        for (int j = 0; j < field_size; ++j) {
            if (i == 0 || j == 0) {
                galois_div_table[i][j] = 0;
            } else {
                int index = (alpha_inv[i] - alpha_inv[j]) % (field_size - 1);

                /* if index < 0, then index += field_size - 1 to make it positive */
                if (index < 0) {
                    index += field_size - 1;
                }

                galois_div_table[i][j] = alpha[index];
            }
        }
    }
}

GF_ELEMENT galois_add(GF_ELEMENT a, GF_ELEMENT b) { return a ^ b; }

GF_ELEMENT galois_sub(GF_ELEMENT a, GF_ELEMENT b) { return a ^ b; }

GF_ELEMENT galois_mul(GF_ELEMENT a, GF_ELEMENT b) {
    return galois_mul_table[a][b];
}

GF_ELEMENT galois_div(GF_ELEMENT a, GF_ELEMENT b) {
    return galois_div_table[a][b];
}

void galois_destroy() {
    for (int i = 0; i < field_size; ++i) {
        free(galois_mul_table[i]);
        free(galois_div_table[i]);
    }
    free(galois_mul_table);
    free(galois_div_table);
    free(alpha);
    free(alpha_inv);
}
