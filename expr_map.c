#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#include "common.h"

typedef struct {
    u32 input, output;
} KeyValue;

typedef struct {
    KeyValue *key_values;
    size_t size;
} Mappings;

// Ops:
// Arithmetic Ops: +/-, -(negate), *
// Bitwise    Ops: ~, |, &, ^, <<, >>, signed >>, right rotate

// Function to calculate the determinant of a 3x3 matrix
u32 determinant(u32 m[3][3]) {
    return m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) -
           m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0]) +
           m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]);
}

// Comparison function for integers
int compare_KeyValues(const void *a, const void *b) {
    // pos: a greater, 0: equal, neg: b greater
    return ((const KeyValue*)a)->input - ((const KeyValue*)b)->input;
}

#define BUFFER_SIZE 100
/* Finds simple expression equivalent to mapped values.
 * Desgin is:
 *  1. Calculate parameters for template expression based on first few values.
 *  2. Check against ALL (including first few - helps prevent bad calculations from being 'confirmed')
 *  3. On error return false + error message
 */
bool map_2_simple_expr(char buffer[BUFFER_SIZE], const Mappings *mappings) {
    // Note that Overflow is a feature

    const size_t msize = mappings->size;

    // 1. Size 0 No Relationship Exception
    if (msize == 0) {
        sprintf(buffer, "No mappings provided.");
        return false;
    }

    const KeyValue * const start_ptr = &mappings->key_values[0];
    const u32 y0 = start_ptr->output;

    // 2. Size 1 - Constant Relationship
    if (msize == 1) {
        sprintf(buffer, "y = %u", y0);
        return true;
    }

    const KeyValue * const end_ptr = &start_ptr[msize];
    const u32 x0 = start_ptr->input;

    // 3. Duplicate Inputs No Relationship Exception
    {
        // sort by input
        qsort((void*)start_ptr, msize, sizeof(KeyValue), compare_KeyValues);
        // check adjacent
        u32 prev = x0;
        bool has_duplicate = any(&start_ptr[1], end_ptr, ({
            u32 curr = _any_ptr->input;
            bool my_case = curr == prev;
            prev = curr;
            my_case;
        }));
        if (has_duplicate) {
            sprintf(buffer, "Mappings included duplicate inputs.");
            return false;
        }
    }

    // 4. Test Constant Relationship
    {
        bool const_relation = all(&start_ptr[1], end_ptr, ({ _all_ptr->output == y0; }));
        if (const_relation) {
            sprintf(buffer, "y = %u", y0);
            return true;
        }
    }

    // 5. Test Single Op Relationship TODO
    // Arithmetic Ops: +/-, -(negate), *
    // Bitwise    Ops: ~, |, &, ^, <<, >>, signed >>, right rotate
    for (int a = 0; a < 32; a++) {
        bool rshift = true, lshift = true, rshift_v = true, lshift_v = true;
        for (size_t i = 0; i < mappings->size; i++) {
            u32 x = start_ptr[i].input;
            u32 y = start_ptr[i].output;
            if ((x >> a) != y) rshift = false;
            if ((x << a) != y) lshift = false;
            if (x < 32 && (a >> x) != y) rshift_v = false; else if (x >= 32) rshift_v = false;
            if (x < 32 && (a << x) != y) lshift_v = false; else if (x >= 32) lshift_v = false;
        }
        if (rshift) { sprintf(buffer, "y = x >> %d", a); return true; }
        if (lshift) { sprintf(buffer, "y = x << %d", a); return true; }
        if (rshift_v) { sprintf(buffer, "y = %d >> x", a); return true; }
        if (lshift_v) { sprintf(buffer, "y = %d << x", a); return true; }
    }

    // 6. Linear: y = mx + b
    if (msize >= 2) { // always works for 2 items
        u32 x1 = start_ptr[1].input;
        u32 y1 = start_ptr[1].output;

        u32 m = ((i64)y1 - (i64)y0) / ((i64)x1 - (i64)x0);
        u32 b = y0 - (m * x0);

        // Check rest
        bool all_passed = all(start_ptr, end_ptr, ({
            ((m * (i64)(_all_ptr->input) + b) == _all_ptr->output);
        }));

        if (all_passed) {
            u32 m_abs = abs((i32)m); // pretend m is i32 for << optimization
            if (is_power_2(m_abs)) {
                u32 shift = log2_pure_power(m_abs);
                sprintf(buffer, "y = %u %c (x << %u)", b, m < 0 ? '-' : '+', shift);
            } else {
                sprintf(buffer, "y = %u*x + %u", m, b);
            }
            return true;
        }
    }

    // 7. Exponential: y = (A << (B*x)) + C
    // To solve for 3 unknowns, we check a few small possibilities for A and B
    for (u32 b = 1; b <= 32; b++) {
        for (u32 ae = 0; ae <= 24; ae += 1) { // A exponent
        for (u32 am = (ae == 0) ? 1 : 127; am <= 255; am++) { // A mantissa
            u32 a = am << ae;

            // Calculate C using the first mapping: y0 = (a << (b * x0)) + c
            u32 c = y0 - (a << (b * x0));

            // Check rest
            bool rest_passed = all(start_ptr, end_ptr, ({ ((a << (b * _all_ptr->input)) + c == _all_ptr->output); }));

            if (rest_passed) {
                char b_str[16] = "", c_paran[2] = "", c_str[16] = "";
                if (b == 1) {
                    strcpy_s(b_str, sizeof(b_str), "x");
                } else if (is_power_2(b)) {
                    u32 shift = log2_pure_power(b);
                    sprintf(b_str, "(x << %u)", shift);
                } else {
                    sprintf(b_str, "(%u * x)", b);
                }
                if (c != 0) {
                    strcpy_s(c_paran, sizeof(c_paran), "(");
                    sprintf(c_str, ") + %u", c);
                }

                sprintf(buffer, "y = %s%u << %s%s", c_paran, a, b_str, c_str);
                return true;
            }
        }
    }
    }

    // 8. Quadratic: y = ax^2 + bx + c
    {
        u32 x1, y1, x2, y2; {
            const KeyValue *temp1 = &start_ptr[1];
            x1 = temp1->input;
            y1 = temp1->output;
            const KeyValue *temp2 = &start_ptr[2];
            x2 = temp2->input;
            y2 = temp2->output;
        }

        // Matrix M based on ax^2 + bx + c = y
        u32 M[3][3] = {
            {x0 * x0, x0, 1},
            {x1 * x1, x1, 1},
            {x2 * x2, x2, 1}
        };

        u32 detM = determinant(M);

        // detM = 0 if points are collinear or share x-coordinates - no unique parabola would exist
        if (detM != 0) {
            // Matrices for a, b, and c (replacing columns with y-values)
            u32 Ma[3][3] = {{y0     , x0,  1}, {y1     , x1,  1}, {y2     , x2,  1}};
            u32 Mb[3][3] = {{x0 * x0, y0,  1}, {x1 * x1, y1,  1}, {x2 * x2, y2,  1}};
            u32 Mc[3][3] = {{x0 * x0, x0, y0}, {x1 * x1, x1, y1}, {x2 * x2, x2, y2}};

            u32 a = determinant(Ma) / detM;
            u32 b = determinant(Mb) / detM;
            u32 c = determinant(Mc) / detM;

            // Check
            bool check_pass = all(start_ptr, end_ptr, ({
                u32 x = _all_ptr->input;
                u32 y = _all_ptr->output;
                ((a * x * x + b * x + c) == y);
             }));

            char a_str[16] = "", b_str[16] = "", c_str[16] ="";
            if (a != 0) sprintf(a_str, "%u * ", a);
            if (b != 0) sprintf(b_str, "%u * ", b);
            if (c != 0) sprintf(c_str, " + %u", a);

            sprintf(buffer, "y = %sx * x + %sx%s", a_str, b_str, c_str);
        }
    }

    sprintf(buffer, "Failed to find mapping expression.");
    return false;
}






int main() {
    printf("=== 32-bit Expression Calculator ===\n");
    printf("Finds simplest C expression using hard coded templates to map inputs to outputs.\n");
    printf("Ops: +, -, -(negate), *, ~, -, <<, >>, >>> (arith), ROR, |, &, ^\n");

    // Read number of mappings
    u32 nmap;
    printf("Number of mappings: ");
    if (scanf_s("%d", &nmap) != 1 || nmap < 1) {
        fprintf(stderr, "Invalid number of mappings.\n");
        return 1;
    }

    // Initialize the Mappings struct
    Mappings mappings;
    mappings.size = (size_t)nmap;
    mappings.key_values = malloc(sizeof(KeyValue) * mappings.size);

    if (!mappings.key_values) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }

    // Read mappings
    for (u32 i = 0; i < nmap; i++) {
        u32 in, out;
        printf("  Mapping %d (input output in hex): ", i + 1);
        if (scanf_s("%x %x", &in, &out) != 2) {
            fprintf(stderr, "Invalid input format.\n");
            free(mappings.key_values);
            return 1;
        }
        mappings.key_values[i].input = in;
        mappings.key_values[i].output = out;
    }

    char buffer[100];
    printf("\nSearching for expression...\n");

    if (map_2_simple_expr(buffer, &mappings)) {
        printf("\n=== FOUND! ===\n");
        printf("Result: %s\n", buffer);

        if (strstr(buffer, "ROR")) {
            printf("\nNOTE: Add this to your code for ROR:\n");
            printf("#define ROR(x, r) (((x) >> ((r) & 31)) | ((x) << (32 - ((r) & 31))))\n");
        }
    } else {
        printf("\nNo matching expression found.\n");
    }

    // Cleanup
    free(mappings.key_values);
    return 0;
}
