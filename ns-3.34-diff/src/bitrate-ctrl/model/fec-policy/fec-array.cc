
#include "fec-array.h"

/* Please modify the filename as "fec-array.c/cc" */
/* QoE Coefficient: 1e-04 */

void get_fec_count(double_t loss, uint8_t frame_size, uint16_t remaining_time, uint16_t rtt, uint8_t packet, uint8_t * fec_count) {
    // loss_index = round((loss - start) / interval)
    loss = (loss < 0.00) ? 0.00 : ((loss > 0.50) ? 0.50 : loss);
    uint8_t loss_index = (uint8_t) round((loss - 0.00) / 0.01);

    // frame_size_index = ceil((frame_size - start) / interval)
    frame_size = (frame_size < 5) ? 5 : ((frame_size > 55) ? 55 : frame_size);
    uint8_t frame_size_index = (uint8_t) ceil(((double_t) frame_size - 5) / 5);

    // compute the remaining layer
    uint8_t layer = (uint8_t) (remaining_time / rtt);
    layer = (layer < 1) ? 1 : ((layer > 15) ? 15 : layer);
    uint8_t layer_index = (uint8_t) round(((double_t) layer - 1) / 1);

    // packet_index = round((packet - start) / interval)
    packet = (packet < 1) ? 1 : ((packet > 55) ? 55 : packet);
    uint8_t packet_index = (uint8_t) round(((double_t) packet - 1) / 1);

    /* array index */
    uint64_t index =
        loss_index * 9075
        + frame_size_index * 825
        + layer_index * 55
        + packet_index;
    /* assignment */
    *fec_count = (uint8_t) beta_array[index];
};

void get_block_size(double_t loss, uint8_t frame_size, uint16_t ddl, uint16_t rtt, double_t rdisp, uint8_t * block_size) {
    // loss_index = round((loss - start) / interval)
    loss = (loss < 0.00) ? 0.00 : ((loss > 0.50) ? 0.50 : loss);
    uint8_t loss_index = (uint8_t) round((loss - 0.00) / 0.01);

    // frame_size_index = ceil((frame_size - start) / interval)
    frame_size = (frame_size < 5) ? 5 : ((frame_size > 55) ? 55 : frame_size);
    uint8_t frame_size_index = (uint8_t) ceil(((double_t) frame_size - 5) / 5);

    // ddl_index = round((ddl - start) / interval)
    ddl = (ddl < 20) ? 20 : ((ddl > 140) ? 140 : ddl);
    uint8_t ddl_index = (uint8_t) round(((double_t) ddl - 20) / 20);

    // rtt_index = round((rtt - start) / interval)
    rtt = (rtt < 10) ? 10 : ((rtt > 80) ? 80 : rtt);
    uint8_t rtt_index = (uint8_t) round(((double_t) rtt - 10) / 2);

    // rdisp_index = round((rdisp - start) / interval)
    rdisp = (rdisp < 0.00) ? 0.00 : ((rdisp > 1.00) ? 1.00 : rdisp);
    uint8_t rdisp_index = (uint8_t) round((rdisp - 0.00) / 0.02);

    /* array index */
    uint64_t index =
        loss_index * 141372
        + frame_size_index * 12852
        + ddl_index * 1836
        + rtt_index * 51
        + rdisp_index;
    /* assignment */
    *block_size = (uint8_t) block_array[index];
}
