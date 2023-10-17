
#include "feconly-fec-array.h"

void get_fec_rate_feconly(double_t loss, uint8_t group_size, double_t * beta_0) {
    // loss_index = round((loss - start) / interval)
    if(loss < 0.000000)    loss = 0.000000;
    if(loss > 0.500000)    loss = 0.500000;
    uint8_t loss_index = (uint8_t) round((loss - 0.000000) / 0.010000);
    // group_size_index = (group_size - start) / interval
    if(group_size < 5)    group_size = 5;
    if(group_size > 55)    group_size = 55;
    group_size = round(((double_t) group_size - 5) / 5) * 5 + 5;
    uint8_t group_size_index = (uint8_t) round(((double_t) group_size - 5) / 5);

    /* array index */
    uint64_t index = loss_index * 11 + group_size_index;
    /* assignment */
    *beta_0 = ((double_t) fec_array_feconly[index]) / group_size;
};
