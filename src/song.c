#include "song.h"

int parse_int(char *s, int *i);
int parse_note(char *s, fm_note *note);

fm_song fm_new_song(int num_parts, int bpm) {
    fm_song song;
    
    song.bpm = bpm;
    song.beats_per_bar = 4;
    song.num_parts = num_parts;
    song.parts = malloc(sizeof(fm_song_part) * num_parts);

    for (int i = 0; i < num_parts; i++) {
        song.parts[i].num_notes = 0;
    }

    return song;
}

int fm_parse_song(char *filename, fm_song *song) {
    FILE *fp;

    fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("could not open file\n");
        return 0;
    }

    song->parts = NULL;
    song->num_parts = 0;

    char *line = malloc(256 * sizeof(char));
    int i, state = 0, linum = 0, part_num = -1, note_num = -1;
    fm_note note;

    while ((line[0] = fgetc(fp)) != EOF) {
        linum++;
        
        for (i = 1; (line[i] = fgetc(fp)) != '\n'; i++);
        line[i] = '\0';

        // get to the first non-whitespace character.
        fm_parse_spaces(&line);

        switch (state) {
        case -1:
            // everything is finished, no input expected
            if (!fm_parse_eol(&line)) goto error;
            break;
            
        case 0:
            // needs bpm
            if (!fm_parse_string(&line, "bpm")) goto error;
            if (!fm_parse_spaces(&line)) goto error;
            if (!fm_parse_int(&line, &song->bpm)) goto error;
            if (!fm_parse_eol(&line)) goto error;
            
            state = 1;
            break;

        case 1:
            // needs num. parts
            if (!fm_parse_string(&line, "num_parts")) goto error;
            if (!fm_parse_spaces(&line)) goto error;
            if (!fm_parse_int(&line, &song->num_parts)) goto error;
            if (!fm_parse_eol(&line)) goto error;
            
            song->parts = malloc(sizeof(fm_song_part) * song->num_parts);
            state = 2;
            break;

        case 2:
            // got bpm and num. parts, expecting a part definition
            if (!fm_parse_string(&line, "part")) goto error;
            if (!fm_parse_eol(&line)) goto error;

            part_num++;
            if (part_num == song->num_parts) {
                // got all the parts we need
                state = -1;
            } else {
                // otherwise, parse the next part
                state = 3;
            }
            
            break;

        case 3:
            // inside a part definition, expecting num. notes
            if (!fm_parse_string(&line, "num_notes")) goto error;
            if (!fm_parse_spaces(&line)) goto error;
            if (!fm_parse_int(&line, &song->parts[part_num].num_notes)) goto error;
            if (!fm_parse_eol(&line)) goto error;

            song->parts[part_num].notes = malloc(sizeof(fm_note) * song->parts[part_num].num_notes);
            note_num = 0;
            state = 4;
            break;

        case 4:
            // inside a part definition, got num. notes. expecting a note
            if (!fm_parse_int(&line, &note.beat)) goto error;
            if (!fm_parse_string(&line, ":")) goto error;
            if (!fm_parse_int(&line, &note.division)) goto error;
            if (!fm_parse_spaces(&line)) goto error;
            if (!fm_parse_int(&line, &note.pitch)) goto error;
            if (!fm_parse_spaces(&line)) goto error;
            if (!fm_parse_int(&line, &note.duration)) goto error;
            if (!fm_parse_spaces(&line)) goto error;
            if (!fm_parse_float(&line, &note.velocity)) goto error;
            if (!fm_parse_eol(&line)) goto error;

            song->parts[part_num].notes[note_num++] = note;

            if (note_num == song->parts[part_num].num_notes) {
                state = 5;
            }
            
            break;

        case 5:
            // got all of the notes for one part, expecting an "end"
            if (!fm_parse_string(&line, "end")) goto error;
            if (!fm_parse_eol(&line)) goto error;

            state = 2;

            break;
        }
    }

    return 1;

 error:
    printf("syntax error on line %d\n", linum);
    return 0;
}

double fm_song_duration(fm_song *song) {
    double d = 0;

    for (int i = 0; i < song->num_parts; i++) {
        fm_song_part *part = &song->parts[i];
        fm_note last_note = part->notes[part->num_notes - 1];
        double part_dur = fm_note_get_end_time(&last_note, (double) song->bpm / 60);
        if (part_dur > d) d = part_dur;
    }

    return d;
}

/*
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
*/
