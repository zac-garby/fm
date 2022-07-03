#include "export.h"

#define WAV_CHUNK_SIZE 65536

typedef struct wav_header {
    unsigned char chunk_id[4];
    unsigned int file_size;
    unsigned char format_id[4];
    unsigned char fmt_id[4];
    unsigned int format_len;
    unsigned short format_type;
    unsigned short num_channels;
    unsigned int sample_rate;
    unsigned int bytes_per_second;
    unsigned short bytes_per_sample;
    unsigned short bits_per_sample;
    unsigned char data_id[4];
    unsigned int data_size;
} wav_header;

int fm_export_wav(char *filename, fm_player *player,
                  int sample_rate, int bitrate, double time) {
    printf("beginning export...\n");
    
    fm_player_reset(player);

    for (int i = 0; i < player->num_instrs; i++) {
        player->next_notes[i] = 0;
    }

    int num_samples = (int) (sample_rate * time);
    int byterate = bitrate / 8;
    double time_per_frame = 1.0 / sample_rate;
    int frames_per_quantum = sample_rate / TIME_QUANTIZE;

    wav_header h;
    memcpy(h.chunk_id, "RIFF", 4);
    memcpy(h.format_id, "WAVE", 4);
    memcpy(h.fmt_id, "fmt ", 4);
    memcpy(h.data_id, "data", 4);
    h.format_len = 16;
    h.format_type = 1;
    h.num_channels = 1;
    h.sample_rate = sample_rate;
    h.bytes_per_second = sample_rate * byterate;
    h.bytes_per_sample = byterate;
    h.bits_per_sample = bitrate;
    h.data_size = num_samples * byterate;
    h.file_size = h.data_size + sizeof(wav_header);

    FILE *f = fopen(filename, "wb");
    fwrite(&h, sizeof(h), 1, f);

    short *data = malloc(sizeof(short) * WAV_CHUNK_SIZE);
    int chunk_idx = 0;
    
    for (int frame = 0; frame < num_samples; frame++) {
        if (player->quantize_counter++ >= frames_per_quantum) {
            fm_player_schedule(player, time_per_frame);
            player->quantize_counter = 0;
        }

        float sample = 0;

        for (int i = 0; i < player->num_instrs; i++) {
            sample += fm_instr_get_next_output(&player->instrs[i],
                                               frame * time_per_frame);
        }
        
        data[chunk_idx++] = (short) (sample * 32767 * player->volume);
        player->playhead += time_per_frame;

        if (chunk_idx == WAV_CHUNK_SIZE) {
            float pct = 100.0f * ((float) frame / (float) num_samples);
            printf("\r%.1lfs/%.1lfs (%.0f%%)", frame * time_per_frame, num_samples * time_per_frame, pct);
            fflush(stdout);
            
            fwrite(data, sizeof(short), WAV_CHUNK_SIZE, f);
            chunk_idx = 0;
        }
    }

    // write any remaining unwritten data
    fwrite(data, sizeof(short), chunk_idx, f);

    fclose(f);

    printf("\nwritten output to %s\n", filename);
    
    return 1;
}
