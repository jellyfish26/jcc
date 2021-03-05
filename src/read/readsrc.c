#include "read.h"

SourceLine *source_code;

void readfile(char *file_path) {
    FILE *fp;
    if ((fp = fopen(file_path, "r")) == NULL) {
        fprintf(stderr, "Failed to open the file\n");
        exit(1);
    }

    SourceLine *now_line = NULL;
    char now_c;
    int cnt;
    while (true) {
        now_c = fgetc(fp);
        if (now_c == '\n' || now_c == EOF) {
            SourceLine *new_line = calloc(1, sizeof(SourceLine));
            if (now_line) {
                now_line->next = new_line;
                new_line->number = now_line->number + 1;
            } else {
                source_code = new_line;
                new_line->number = 1;
            }
            new_line->len = cnt + 2;
            cnt = 0;
            now_line = new_line;
            if (now_c == EOF) {
                break;
            }
            continue;
        }
        cnt++;
    }
    fclose(fp);

    fp = fopen(file_path, "r");
    now_line = source_code;
    while (now_line) {
        now_line->str = calloc(now_line->len + 2, sizeof(char));
        fgets(now_line->str, now_line->len, fp);
        now_line = now_line->next;
    }

    fclose(fp);
}