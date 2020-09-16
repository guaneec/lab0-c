/** dude, is my code constant time?
 *
 * This file measures the execution time of a given function many times with
 * different inputs and performs a Welch's t-test to determine if the function
 * runs in constant time or not. This is essentially leakage detection, and
 * not a timing attack.
 *
 * Notes:
 *
 *  - the execution time distribution tends to be skewed towards large
 *    timings, leading to a fat right tail. Most executions take little time,
 *    some of them take a lot. We try to speed up the test process by
 *    throwing away those measurements with large cycle count. (For example,
 *    those measurements could correspond to the execution being interrupted
 *    by the OS.) Setting a threshold value for this is not obvious; we just
 *    keep the x% percent fastest timings, and repeat for several values of x.
 *
 *  - the previous observation is highly heuristic. We also keep the uncropped
 *    measurement time and do a t-test on that.
 *
 *  - we also test for unequal variances (second order test), but this is
 *    probably redundant since we're doing as well a t-test on cropped
 *    measurements (non-linear transform)
 *
 *  - as long as any of the different test fails, the code will be deemed
 *    variable time.
 *
 */

#include "fixture.h"
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpucycles.h"
#include "percentile.h"
#include "random.h"
#include "ttest.h"

#define number_tests                                                    \
    (1 /* first order uncropped */ + number_percentiles /* cropped */ + \
     1 /* second order uncropped */)
#define enough_measurements 10000  // pretty arbitrary
#define test_tries 10
#define number_percentiles 5

static t_ctx *t[number_tests];
static int64_t percentiles[number_percentiles] = {0};
extern const size_t chunk_size;
extern const size_t number_measurements;

// threshold values for Welch's t-test
#define t_threshold_bananas 500  // test failed, with overwhelming probability
#define t_threshold_moderate \
    10  // test failed. Pankaj likes 4.5 but let's be more lenient

static void __attribute__((noreturn)) die(void)
{
    exit(111);
}

// fill percentiles.
// the exponential tendency is mean to approximately match
// the measurements distribution.
static void prepare_percentiles(int64_t *ticks)
{
    for (size_t i = 0; i < number_percentiles; i++) {
        percentiles[i] = percentile(
            ticks, 1 - (pow(0.5, 10 * (double) (i + 1) / number_percentiles)),
            number_measurements);
    }
}

static void differentiate(int64_t *exec_times,
                          int64_t *before_ticks,
                          int64_t *after_ticks)
{
    for (size_t i = 0; i < number_measurements; i++) {
        exec_times[i] = after_ticks[i] - before_ticks[i];
    }
}

static void update_statistics(int64_t *exec_times, uint8_t *classes)
{
    // XXX we could throw away the first 1e4 and the last 1e4 measurements,
    // to minimize measurement noise. test if this actually improves anything.
    for (size_t i = 0; i < number_measurements; i++) {
        int64_t difference = exec_times[i];

        if (difference < 0) {
            continue;  // the cpu cycle counter overflowed
        }

        // do a t-test on the execution time
        t_push(t[0], difference, classes[i]);

        // do a t-test on cropped execution times, for several cropping
        // thresholds.
        for (size_t crop_index = 0; crop_index < number_percentiles;
             crop_index++) {
            if (difference < percentiles[crop_index]) {
                t_push(t[crop_index + 1], difference, classes[i]);
            }
        }

        // do a second-order test (only if we have more than 10000
        // measurements). Centered product pre-processing.
        if (t[0]->n[0] > 10000) {
            double centered = (double) difference - t[0]->mean[classes[i]];
            t_push(t[1 + number_percentiles], centered * centered, classes[i]);
        }
    }
}

// which t-test yields max t value?
static int max_test(void)
{
    int ret = 0;
    double max = 0;
    for (size_t i = 0; i < number_tests; i++) {
        if (t[i]->n[0] > enough_measurements) {
            double x = fabs(t_compute(t[i]));
            if (max < x) {
                max = x;
                ret = i;
            }
        }
    }
    return ret;
}

static bool report(void)
{
    int mt = max_test();
    double max_t = fabs(t_compute(t[mt]));
    double number_traces_max_t = t[mt]->n[0] + t[mt]->n[1];
    double max_tau = max_t / sqrt(number_traces_max_t);

    printf("\033[A\033[2K");
    printf("meas: %7.2lf M, ", (number_traces_max_t / 1e6));
    if (number_traces_max_t < enough_measurements) {
        printf("not enough measurements (%.0f still to go).\n",
               enough_measurements - number_traces_max_t);
        return false;
    }

    /*
     * max_t: the t statistic value
     * max_tau: a t value normalized by sqrt(number of measurements).
     *          this way we can compare max_tau taken with different
     *          number of measurements. This is sort of "distance
     *          between distributions", independent of number of
     *          measurements.
     * (5/tau)^2: how many measurements we would need to barely
     *            detect the leak, if present. "barely detect the
     *            leak" = have a t value greater than 5.
     */
    for (size_t i = 0; i < number_tests; ++i) {
        printf("%7.2f@%f", fabs(t_compute(t[i])), t[i]->n[0]);
    }
    printf("\n");
    printf("max t: %+7.2f, max tau: %.2e, (5/tau)^2: %.2e.\n\n\n", max_t,
           max_tau, (double) (5 * 5) / (double) (max_tau * max_tau));

    if (max_t > t_threshold_bananas) {
        return false;
    } else if (max_t > t_threshold_moderate) {
        return false;
    } else { /* max_t < t_threshold_moderate */
        return true;
    }
}

static bool doit(int mode)
{
    int64_t *before_ticks = calloc(number_measurements + 1, sizeof(int64_t));
    int64_t *after_ticks = calloc(number_measurements + 1, sizeof(int64_t));
    int64_t *exec_times = calloc(number_measurements, sizeof(int64_t));
    uint8_t *classes = calloc(number_measurements, sizeof(uint8_t));
    uint8_t *input_data =
        calloc(number_measurements * chunk_size, sizeof(uint8_t));

    if (!before_ticks || !after_ticks || !exec_times || !classes ||
        !input_data) {
        die();
    }

    prepare_inputs(input_data, classes);
    measure(before_ticks, after_ticks, input_data, mode);
    differentiate(exec_times, before_ticks, after_ticks);

    // we compute the percentiles only if they are not filled yet
    if (percentiles[number_percentiles - 1] == 0) {
        prepare_percentiles(exec_times);
    }
    update_statistics(exec_times, classes);
    bool ret = report();

    free(before_ticks);
    free(after_ticks);
    free(exec_times);
    free(classes);
    free(input_data);
    return ret;
}
static void init_once(void)
{
    init_dut();
    for (int i = 0; i < number_tests; ++i) {
        t_init(t[i]);
    }
}


bool is_insert_tail_const(void)
{
    bool result = false;
    for (int i = 0; i < number_tests; ++i) {
        t[i] = malloc(sizeof(t_ctx));
    }


    for (int cnt = 0; cnt < test_tries; ++cnt) {
        printf("Testing insert_tail...(%d/%d)\n\n", cnt, test_tries);
        init_once();
        for (int i = 0; i < enough_measurements; i += number_measurements)
            printf("\033[A\033[2K\033[A\033[2K");
        if (result == true)
            break;
    }
    for (int i = 0; i < number_tests; ++i) {
        free(t[i]);
    }
    return result;
}

bool is_size_const(void)
{
    bool result = false;
    for (int i = 0; i < number_tests; ++i) {
        t[i] = malloc(sizeof(t_ctx));
    }

    for (int cnt = 0; cnt < test_tries; ++cnt) {
        printf("Testing size...(%d/%d)\n\n", cnt, test_tries);
        init_once();
        for (int i = 0; i < enough_measurements; i += number_measurements)
            result = doit(1);
        printf("\033[A\033[2K\033[A\033[2K");
        // if (result == true)
        //     break;
    }
    for (int i = 0; i < number_tests; ++i) {
        free(t[i]);
    }
    return result;
}
