#ifndef H_FM_EXPORT
#define H_FM_EXPORT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "player.h"

int fm_export_wav(char *filename, fm_player *player,
                  int sample_rate, int bitrate, double time);

#endif
