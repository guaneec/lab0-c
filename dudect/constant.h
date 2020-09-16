#ifndef DUDECT_CONSTANT_H
#define DUDECT_CONSTANT_H

#include <stdint.h>

void prepare_inputs(uint8_t *input_data, uint8_t *classes);
void measure(int64_t *before_ticks,
             int64_t *after_ticks,
             uint8_t *input_data,
             uint8_t *classes,
             int mode);

#endif
