#include <stdio.h>
#include <string.h>
#include <ctype.h>

enum { LOOK_AHEAD = 4, BUF_SIZE = 1024 };
enum { CODE = 0, COMMENT = 1 };

int buffer[BUF_SIZE], *head = buffer, *tail = buffer, line_no = 1;
long long stats[2] = { 0, 0 };
FILE *fp;

void consume(int n, int account) {
    while (n > 0 || tail != buffer + BUF_SIZE) {
        while (tail < buffer + BUF_SIZE)
            *tail++ = getc(fp);

        while (tail - head > LOOK_AHEAD && n > 0) {
            int c = *head++;
            n--;

            if (!isspace(c))
                stats[account]++;

            if (c == '\n')
                line_no++;

            //printf(account == COMMENT ? "\033[7m%c\033[m" : "%c", c);
        }

        if (tail - head == LOOK_AHEAD) {
            memmove(buffer, head, LOOK_AHEAD * sizeof(buffer[0]));
            head = buffer;
            tail = buffer + LOOK_AHEAD;
        }
    }
}

int match(const char *str) {
    int n;
    for (n = 0; str[n]; n++) {
        if (head[n] != str[n])
            return 0;
    }
    return 1;
}

int main(int argc, char *argv[])
{
    const char *filename;
    if (argc == 1) {
        filename = "<stdin>";
        fp = stdin;
    } else if (argc == 2) {
        filename = argv[1];
        fp = fopen(argv[1], "rb");
        if (fp == NULL) {
            fprintf(stderr, "Can't open file \"%s\".\n", filename);
            return 1;
        }
    } else {
        fprintf(stderr, "Usage: %s [filename]\n", argv[0]);
        return 1;
    }

    consume(0, CODE);
    while (*head != EOF) {
        if (match("//")) {            //-style comments
            consume(2, COMMENT);
            while (*head != EOF) {
                if (match("\\\r\n")) {
                    consume(3, COMMENT);
                } else if (match("\\\n")) {
                    consume(2, COMMENT);
                } else if (*head == '\n') {
                    consume(1, COMMENT);
                    break;
                } else {
                    consume(1, COMMENT);
                }
            }
        } else if (match("/*")) {
            consume(2, COMMENT);
            while (*head != EOF) {
                if (match("*/")) {
                    consume(2, COMMENT);
                    break;
                } else {
                    consume(1, COMMENT);
                }
            }
        } else if (*head == '"' || *head == '\'') {
            int quote = *head;
            consume(1, CODE);
            while (*head != EOF) {
                if (head[0] == '\\' && head[1] != EOF) {
                    consume(2, CODE);
                } else if (*head == quote) {
                    consume(1, CODE);
                    break;
                } else {
                    if (*head == '\n') {
                        fprintf(stderr, "Warning: newline in a quoted string at %s:%d\n", filename, line_no);
                        break;
                    }
                    consume(1, CODE);
                }
            }
        } else {
            consume(1, CODE);
        }
    }

    printf("%lld %lld\n", stats[COMMENT], stats[CODE]);
    return 0;
}
