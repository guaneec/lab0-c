#include "constant.h"
#include <assert.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "console.h"
#include "cpucycles.h"
#include "queue.h"
#include "random.h"

#define NR_MEASURE 150
/* Allow random number range from 0 to 65535 */
const size_t chunk_size = 32;
/* Number of measurements per test */
const size_t number_measurements = NR_MEASURE;
const int drop_size = 20;
/* Maintain a queue independent from the qtest since
 * we do not want the test to affect the original functionality
 */
static queue_t *q = NULL;
static char random_string[NR_MEASURE][8];
static int random_string_iter = 0;
enum { test_insert_tail, test_size };

/* Implement the necessary queue interface to simulation */
void init_dut(void)
{
    q = NULL;
}

char *get_random_string(void)
{
    random_string_iter = (random_string_iter + 1) % NR_MEASURE;
    return random_string[random_string_iter];
}

void prepare_inputs(uint8_t *input_data, uint8_t *classes)
{
    FILE *f = fopen("div.txt", "r");
    uint64_t m0;
    uint32_t m1, n, d, qq;
    for (int i = 0; i < number_measurements &&
                    fscanf(f, "%lu %u %u %u %u", &m0, &m1, &n, &d, &qq) == 5;
         ++i) {
        *(uint64_t *) (input_data + i * chunk_size) = m0;
        *(uint32_t *) (input_data + i * chunk_size + 8) = m1;
        *(uint32_t *) (input_data + i * chunk_size + 12) = n;
        *(uint32_t *) (input_data + i * chunk_size + 16) = d;
        *(uint32_t *) (input_data + i * chunk_size + 20) = qq;
    }
    fclose(f);

    for (size_t i = 0; i < number_measurements; i++) {
        classes[i] = randombit();
        if (classes[i] == 0 && old_measure)
            *(uint16_t *) (input_data + i * chunk_size) = 0x00;
    }

    for (size_t i = 0; i < NR_MEASURE; ++i) {
        /* Generate random string */
        randombytes((uint8_t *) random_string[i], 7);
        random_string[i][7] = 0;
    }
}

void measure_old(int64_t *before_ticks,
                 int64_t *after_ticks,
                 uint8_t *input_data,
                 int mode)
{
    assert(mode == test_insert_tail || mode == test_size);
    if (mode == test_insert_tail) {
        for (size_t i = drop_size; i < number_measurements - drop_size; i++) {
            char *s = get_random_string();
            dut_new();
            dut_insert_head(
                get_random_string(),
                *(uint16_t *) (input_data + i * chunk_size) > 0 ? 2 : 1);
            before_ticks[i] = cpucycles();
            dut_insert_tail(s, 1);
            after_ticks[i] = cpucycles();
            dut_free();
        }
    } else {
        for (size_t i = drop_size; i < number_measurements - drop_size; i++) {
            dut_new();
            dut_insert_head(
                get_random_string(),
                *(uint16_t *) (input_data + i * chunk_size) > 0 ? 2 : 1);
            before_ticks[i] = cpucycles();
            dut_size(1);
            after_ticks[i] = cpucycles();
            dut_free();
        }
    }
}

void measure(int64_t *before_ticks,
             int64_t *after_ticks,
             uint8_t *input_data,
             uint8_t *classes,
             int mode)
{
    assert(mode == test_insert_tail || mode == test_size);
    if (mode == test_insert_tail) {
        for (size_t i = 0; i < number_measurements; i++) {
            char *s = get_random_string();
            queue_t *q0 = q_new();
            queue_t *q1 = q_new();
            queue_t *qi = classes[i] ? q1 : q0;
            q_insert_head(q0, get_random_string());
            q_insert_head(q1, get_random_string());
            q_insert_head(q1, get_random_string());
            before_ticks[i] = cpucycles();
            q_insert_tail(qi, s);
            after_ticks[i] = cpucycles();
            q_free(q0);
            q_free(q1);
        }
    } else {
        for (size_t i = 0; i < number_measurements; i++) {
            int c = classes[i];
            uint64_t m0 = *(uint64_t *) (input_data + i * chunk_size);
            uint32_t m1 = *(uint32_t *) (input_data + i * chunk_size + 8);
            uint32_t n = *(uint32_t *) (input_data + i * chunk_size + 12);
            uint32_t d = *(uint32_t *) (input_data + i * chunk_size + 16);
            uint32_t qq = *(uint32_t *) (input_data + i * chunk_size + 20);
            uint32_t qqq = 0;
            if (c) {
                before_ticks[i] = cpucycles();
                qqq = ((uint64_t) m1 * (uint64_t) n) >> 32;
                after_ticks[i] = cpucycles();
            } else {
                before_ticks[i] = cpucycles();
                qqq = ((__uint128_t) m0 * n) >> 64;
                after_ticks[i] = cpucycles();
            }
            assert(qqq == qq);
            assert(n / d == qq);
        }
    }
}
