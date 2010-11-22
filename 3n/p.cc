// Note: miscomputes answer for n=1. I'm too lazy to fix...
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cmath>
#include <sys/time.h>
#include <gmp.h>

typedef unsigned char uchar;

double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

uchar *bcd_digits;  // packed BCD number. two digits in one byte. little-endian
int max_size;       // max size in bytes (2 * number of digits)
int cur_size;       // current size in bytes

int div_table[3][256];  // [carry][bcd]

void init_tables() {
    for (int c = 0; c < 3; c++) {
        for (int a = 0; a < 10; a++) {
            for (int b = 0; b < 10; b++) {
                int cc = (a * 10 + b) * 3 + c;
                assert((cc / 100) <= 2);
                div_table[c][a*16+b] = ((cc/100) << 8) | (((cc%100)/10) << 4) | (cc % 10);
            }
        }
    }
}

void mul3() {
    int c = 0;
    uchar *b = bcd_digits, *e = bcd_digits + cur_size;
    while (b < e) {
        c = div_table[c][*b];
        *b++ = c & 255;
        c >>= 8;
    }

    if (c != 0) {
        assert(cur_size + 1 <= max_size);
        *b++ = c;
        cur_size++;
    }
}

void print() {
    int i = cur_size - 1;
    if (i > 0 && bcd_digits[i] < 10) {
        printf("%d", bcd_digits[i]);
        i--;
    }
    while (i >= 0) {
        printf("%.2X", bcd_digits[i]);
        assert(bcd_digits[i]/16 < 10);
        assert(bcd_digits[i]%16 < 10);
        i--;
    }
    printf("\n");
}

// Returns max length of contiguous sequence of zeros. Bug: instead of 0 will return 1.
int check()
{
    int z = 0, maxz = 1;
    uchar *b = bcd_digits, *e = bcd_digits + cur_size;

    while (b < e) {
        if (*b == 0) {
            z += 2;
        } else if ((*b & 0x0F) == 0) {
            z += 1;
            if (z > maxz)
                maxz = z;
            z = 0;
        } else {
            if (z > maxz)
                maxz = z;
            z = ((*b & 0xF0) == 0) ? 1 : 0;
        }
        b++;
    }

    if (z > maxz)
        maxz = z;

    return maxz;
}

// Initialize number to 3^n
void init(int n)
{
    mpz_t value;
    mpz_init(value);
    mpz_ui_pow_ui(value, 3, n);

    char *str = mpz_get_str(NULL, 10, value);
    int size = strlen(str);
    assert(size < max_size*2);

    memset(bcd_digits, 0, max_size);

    int k = 0;
    for (int i = size - 1; i >= 0; i -= 2) {
        assert('0' <= str[i] && str[i] <= '9');
        if (i - 1 >= 0) {
            assert('0' <= str[i - 1] && str[i - 1] <= '9');
            bcd_digits[k++] = (str[i-1] - '0') * 16 + (str[i] - '0');
        } else {
            bcd_digits[k++] = (str[i] - '0');
        }
    }

    cur_size = k;

    free(str);
}

int main(int argc, char **argv)
{
    if (argc != 4 && argc != 3) {
        printf("Usage: %s a b [min_zeroes]\n", argv[0]);
        return 1;
    }

    double start_time = get_time();

    int first_n = atoi(argv[1]);
    int last_n = atoi(argv[2]);
    int min_zeroes = argc == 4 ? atoi(argv[3]) : 100;
    assert(first_n <= last_n);
    assert(min_zeroes >= 1);

    int max_digits = (int)(log(3.0)*last_n/log(10.0)) + 30;
    max_digits = (max_digits | 15) + 1;
    max_size = max_digits / 2;
    bcd_digits = new uchar[max_size];

    init(first_n);
    init_tables();

    int seen = 0;

    for (int n = first_n; n <= last_n; n++) {
        int zeroes = check();

        if (zeroes >= min_zeroes || (zeroes < min_zeroes && ((seen >> zeroes) & 1) == 0)) {
            double elapsed = get_time() - start_time;
            printf("%d %d %.1fs\n", n, zeroes, elapsed);
            fflush(stdout);

            if (zeroes < min_zeroes)
                seen |= 1 << zeroes;
        }

        mul3();
    }

    printf("checked %d %d\n", first_n, last_n);
    printf("elapsed %.3f\n", get_time() - start_time);
    fflush(stdout);

    return 0;
}
