#ifndef DUDECT_CONSTANT_H
#define DUDECT_CONSTANT_H

#include <stdint.h>

void prepare_inputs(uint8_t *input_data, uint8_t *classes);
void measure(int64_t *before_ticks,
             int64_t *after_ticks,
             uint8_t *input_data,
             uint8_t *classes,
             int mode);
void measure_old(int64_t *before_ticks,
                 int64_t *after_ticks,
                 uint8_t *input_data,
                 int mode);
#define dut_new() ((void) (q = q_new()))

#define dut_size(n)                                \
    do {                                           \
        for (int __iter = 0; __iter < n; ++__iter) \
            q_size(q);                             \
    } while (0)

#define dut_insert_head(s, n)    \
    do {                         \
        int j = n;               \
        while (j--)              \
            q_insert_head(q, s); \
    } while (0)

#define dut_insert_tail(s, n)    \
    do {                         \
        int j = n;               \
        while (j--)              \
            q_insert_tail(q, s); \
    } while (0)

#define dut_free() ((void) (q_free(q)))

void init_dut();
#endif
