#include <cstdio>
#include <cstdlib>
#include <gmp.h>

int main(int argc, char **argv)
{
    if (argc != 3) {
        printf("Usage: pow a b\n");
        return 1;
    }
    int a = atoi(argv[1]);
    int b = atoi(argv[2]);
    mpz_t val;
    mpz_init(val);
    mpz_ui_pow_ui(val, a, b);
    printf("%s\n", mpz_get_str(NULL, 10, val));
    return 0;
}
