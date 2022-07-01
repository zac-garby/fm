#include "filter.h"

fm_biquad fm_new_biquad() {
    fm_biquad bq;

    for (int i = 0; i < 3; i++) {
        bq.x[i] = 0;
        bq.y[i] = 0;
    }

    return bq;
}

void fm_biquad_passthrough(fm_biquad *bq) {
    bq->b[0] = 1;
    bq->b[1] = 0;
    bq->b[2] = 0;
    
    bq->a[1] = 0;
    bq->a[2] = 0;
}

float fm_biquad_run(fm_biquad *bq, float x0) {
    bq->x[2] = bq->x[1];
    bq->x[1] = bq->x[0];
    bq->x[0] = x0;

    float y0 = bq->b[0] * bq->x[0]
            + bq->b[1] * bq->x[1]
            + bq->b[2] * bq->x[2]
            - bq->a[1] * bq->y[1]
            - bq->a[2] * bq->y[2];

    bq->y[2] = bq->y[1];
    bq->y[1] = bq->y[0];
    bq->y[0] = y0;

    return y0;
}
