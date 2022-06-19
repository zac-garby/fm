#include "operator.h"

fm_operator fm_new_op(int recv_n, int send_n, int fixed, float transpose) {
    fm_operator op;

    op.recv_n = recv_n;
    op.send_n = send_n;
    op.fixed = fixed;
    op.transpose = transpose;
    op.phase = 0.0f;
    op.wave_type = FN_SIN;

    op.recv = malloc(sizeof(int) * recv_n);
    op.recv_level = malloc(sizeof(float) * recv_n);

    op.send = malloc(sizeof(int) * send_n);
    op.send_level = malloc(sizeof(float) * send_n);

    return op;
}
