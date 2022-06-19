#include "song.h"

int parse_int(char *s, int *i);
int parse_note(char *s, fm_note *note);

int parse_song(char *filename, fm_song *song) {
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("could not open file\n");
        return 0;
    }

    char line[256];
    int i;
    int bpm_done = 0, num_parts_done = 0, num_notes_done = 0;
    int part_index = -1, in_part = 0;
    int note_index = -1;

    while ((line[0] = fgetc(fp)) != EOF) {
        if (line[0] == '\n') continue;

        for (i = 1; (line[i] = fgetc(fp)) != '\n'; i++);
        line[i] = '\0';

        if (line[0] == '#') continue;

        if (strncmp(line, "bpm", 3) == 0) {
            if (!parse_int(line + 3, &song->bpm)) {
                printf("invalid syntax after bpm: expecting int\n");
                return 0;
            }

            printf("bpm: %d\n", song->bpm);
            bpm_done = 1;
        } else if (strncmp(line, "num_parts", 9) == 0) {
            if (!parse_int(line + 9, &song->num_parts)) {
                printf("invalid syntax after num_parts: expecting int\n");
                return 0;
            }
            
            song->parts = malloc(sizeof(fm_song_part) * song->num_parts);
            printf("%d parts\n", song->num_parts);
            num_parts_done = 1;
        } else if (strncmp(line, "part", 4) == 0) {
            if (!bpm_done || !num_parts_done) {
                printf("invalid syntax: part definitions should come after the preamble\n");
                return 0;
            }

            if (in_part) {
                printf("invalid syntax: parts cannot be embedded\n");
                return 0;
            }

            if (part_index >= song->num_parts) {
                printf("invalid syntax: too many parts!\n");
                return 0;
            }

            in_part = 1;
            num_notes_done = 0;
            part_index++;
        } else if (in_part) {
            fm_song_part *part = &song->parts[part_index];
            
            if (strncmp(line, "end", 3) == 0) {
                in_part = 0;
            } else if (strncmp(line, "num_notes", 9) == 0) {
                if (!parse_int(line + 9, &part->num_notes)) {
                    printf("invalid syntax after num_notes: expecting int\n");
                    return 0;
                }

                part->notes = malloc(sizeof(fm_note) * part->num_notes);
                printf("part %d: %d notes\n", part_index, part->num_notes);
                num_notes_done = 1;
                note_index = 0;
            } else {
                if (!num_notes_done) {
                    printf("invalid syntax: num_notes must be declared before any notes\n");
                    return 0;
                }

                if (note_index >= part->num_notes) {
                    printf("invalid syntax: too many notes!\n");
                    return 0;
                }
                
                fm_note *note = &part->notes[note_index];
                if (!parse_note(line, note)) {
                    printf("invalid syntax: could not parse a note\n");
                    printf("  '%s'\n", line);
                    return 0;
                }

                note_index++;
            }
        }
    }

    return 1;
}

int parse_int(char *s, int *i) {
    char *ep;
    long l = strtol(s, &ep, 10);
    if (*ep != 0 && *ep != ' ' && *ep != '\n') return 0;
    *i = (int) l;
    
    return 1;
}

int parse_note(char *s, fm_note *note) {
    char *ep;

    double start = strtod(s, &ep);
    if (ep == s) return 0;

    // skip whitespace
    while (*ep == ' ') ep++;

    char note_name = *(ep++);

    char accidental = 0;
    if (*ep == '#') {
        accidental = 1;
        ep++;
    } else if (*ep == 'b') {
        accidental = -1;
        ep++;
    }

    int octave = (int) strtol(ep, &ep, 10);
    if (ep == s) return 0;

    ep++;

    if (note_name < 'a' || note_name > 'g') {
        printf("syntax error: notes should be between 'a' and 'g'");
        return 0;
    }
    
    static int note_map[] = { 0, 2, 4, 5, 7, 9, 11 };
    int note_num = note_map[(7 + note_name - 'c') % 7] + accidental + octave * 12;
    float freq = C0 * powf(2.0f, (float) note_num / 12.0);

    double dur = strtod(ep, &ep);
    if (ep == s) return 0;

    ep++;

    double vel = strtod(ep, &ep);
    if (ep == s) return 0;

    note->freq = freq;
    note->start = start;
    note->duration = dur;
    note->velocity = (float) vel;

    return 1;
}
