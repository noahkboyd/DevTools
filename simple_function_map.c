#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <stddef.h>

#define any(buffer_ptr, end_ptr, predicate_block_ptr__p) ({ \
    typeof(buffer_ptr) _p = buffer_ptr;            \
    bool _run = _p != end_ptr;                     \
    bool _some = false;                            \
    while(_run) {                                  \
        _some = predicate_block_ptr__p; _p++;      \
        /* continue if case fail & more buffer */  \
        _run = !_some & (_p < end_ptr);            \
    }                                              \
    _some; /* statement expression return */       \
})

#define all(buffer_ptr, end_ptr, predicate_block_ptr__p) ({ \
    typeof(buffer_ptr) _p = buffer_ptr;            \
    bool _run = _p != end_ptr;                     \
    bool _some = false;                            \
    while(_run) {                                  \
        _some = predicate_block_ptr__p; _p++;      \
        /* continue if case pass & more buffer */  \
        _run = _some & (_p < end_ptr);             \
    }                                              \
    _some; /* statement expression return */       \
})

typedef uint32_t u32;
typedef  int32_t i32;
typedef uint64_t u64;
typedef  int64_t i64;

typedef struct {
    u32 input, output;
} KeyValue;

typedef struct {
    KeyValue *key_values;
    size_t size;
} Mappings;

#define ROTR(x_u32, pos_shift) ((x_u32 >> pos_shift) | (x_u32 << (32 -  pos_shift)))

// Ops:,
// Arithmetic Ops: +/-, -(negate), *
// Bitwise    Ops: ~, |, &, ^, <<, >>, signed >>, right rotate

// Comparison function for integers
int compare_KeyValues(const void *a, const void *b) {
    u32 arg1 = ((const KeyValue*)a)->input;
    u32 arg2 = ((const KeyValue*)b)->input;
    // pos: arg1 greater, 0: equal, neg: arg2 greater
    return arg1 - arg2;
}

#define BUFFER_SIZE 100
bool find_expression(char buffer[BUFFER_SIZE], const Mappings *mappings) {
    // Note that Overflow is a feature

    const size_t msize = mappings->size;

    // 1. Size 0 No Relationship Exception
    if (msize == 0) return false;

    const KeyValue * const first_ptr = &mappings->key_values[0];
    const u32 y0 = first_ptr->output;

    // 2. Size 1 - Constant Relationship
    if (msize == 1) {
        sprintf(buffer, "y = %u", y0);
        return true;
    }

    const KeyValue * const end_ptr   = &first_ptr[msize];
    const u32 x0 = first_ptr->input;

    // 3. Duplicate Inputs No Relationship Exception
    {
        // sort by input
        qsort((void*)first_ptr, msize, sizeof(KeyValue), compare_KeyValues);
        // check adjacent
        u32 prev = x0;
        bool has_duplicate = any(first_ptr + 1, end_ptr, ({
            u32 curr = _p->input;
            bool my_case = curr == prev;
            prev = curr;
            my_case;
        }));
        if (has_duplicate) return false;
    }

    // 3. Test Constant Relationship
    {
        const KeyValue *current = first_ptr;
        while (++current != end_ptr && current->output == y0);

        // no short circuit -> means all cases passed
        if (current == end_ptr) {
            sprintf(buffer, "y = %u", y0);
            return true;
        }
    }

    // 4. Test Single Op Relationship
    // Arithmetic Ops: +/-, -(negate), *
    // Bitwise    Ops: ~, |, &, ^, <<, >>, signed >>, right rotate
    for (int a = 0; a < 32; a++) {
        bool rshift = true, lshift = true, rshift_v = true, lshift_v = true;
        for (size_t i = 0; i < mappings->size; i++) {
            u32 x = first_ptr[i].input;
            u32 y = first_ptr[i].output;
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

    // 2. Linear: y = mx + b
    if (msize >= 2) { // always works for 2 items
        u32 x1 = first_ptr[1].input;
        u32 y1 = first_ptr[1].output;
        if (x1 != x0) {
            i32 m = ((i32)y1 - (i32)y0) / ((i32)x1 - (i32)x0);
            i32 b = (i32)y0 - (m * (i32)x0);

            // Check rest
            const KeyValue *current = first_ptr + 2;
            u32 x, y;
            do {
                u32 x = current->input;
                u32 y = current->output;
            } while ((u32)(m * x + b) == y && ++current != end_ptr);

            if (current == end_ptr) {
                u32 m_abs = abs(m);
                if ((m_abs & (m_abs - 1)) == 0) {
                    u32 shift = 0;
                    u32 copy_m_abs = m_abs;
                    while (copy_m_abs >>= 1) shift++;

                    sprintf(buffer, "y = (%sx << %u) %s %u", m < 0 ? "-" : 0 , shift, b >= 0 ? "+" : "-", (u32)(b < 0 ? -b : b));
                } else {
                    sprintf(buffer, "y = %ld*x %s %u", (long)m, b >= 0 ? "+" : "-", (u32)(b < 0 ? -b : b));
                }
                return true;
            }
        }
    }

    // 3. Exponential: y = (A << (B*x)) + C
    // To solve for 3 unknowns, we check a few small possibilities for A and B
    for (u32 b = 1; b <= 32; b++) { // 204,800 a b combos
        for (u32 ae = 0; ae <= 24; ae += 8) { // A exponent
        for (u32 am = 1; am <= 255; am++) {   // A mantissa
            u32 a = am << ae; // 25*256 = 6,400 a combos

            // Calculate C using the first mapping: y0 = (a << (b * x0)) + c
            u32 c = y0 - (a << (b * x0));

            // Check rest
            const KeyValue *current = first_ptr + 1;
            u32 x, y;
            do {
                x = current->input;
                y = current->output;
            }  while ((a << (b * x)) + c == y && ++current != end_ptr);

            // Pass if loop broke early on failed item
            if (current == end_ptr) {
                char b_str[16] = "", c_paran[2] = "", c_str[16] = "";
                // b is power of 2?
                if (b == 1) {
                    strcpy_s(b_str, sizeof(b_str), "x");
                } else if ((b & (b - 1)) == 0) {
                    u32 shift = 0;
                    while (b >>= 1) shift++;
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

    // 4. Quadratic: y = ax^2 + bx + c (Simplified check)
    if (mappings->size >= 3) {
        // Solving via basic finite differences or Cramer's would go here
        // For brevity, checking simple y = x*x
        bool sq = true;
        for (size_t i = 0; i < msize; i++) {
            if (first_ptr[i].input * first_ptr[i].input != first_ptr[i].output) {
                sq = false; break;
            }
        }
        if (sq) { strcpy_s(buffer, BUFFER_SIZE, "y = x * x"); return true; }
    }

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

    // Call find_expression using the new Mappings pointer
    if (find_expression(buffer, &mappings)) {
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
