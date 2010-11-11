// Counts number of ways to color vertices of n-by-n grid graph with q colors.
//
// As an option, count those colorings subject to the restriction that vertices (i,j)
// and (k,l) vertices have the same, fixed color (http://forums.topcoder.com/?module=Thread&threadID=692834)
//
// Prints a hexadecimal number.
//
#include <stdint.h>
#include <cassert>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <string>
#include <map>
#include <vector>
using namespace std;

struct ParamsStruct {
    int N, Q, i, j, k, l;
} Params;

string StringPrintf(const char *format, ...) {
    va_list va;
    va_start(va, format);
    char *buf = NULL;
    if (vasprintf(&buf, format, va) == -1)
        buf = NULL;
    string res(buf == NULL ? "" : buf);
    va_end(va);
    return res;
}

// An array of 4-bit integers which represent equal subsets of colors in a row of N cells.
// In canonical representation:
// Value 0 always refers to color 0.
// Value i for i>0 represents i-th used non-zero color value when scanning the row from left to right.
struct Config {
    uint64_t repr;

    int operator[](int i) const {
        return Get(i);
    }

    int Get(int i) const {
        return (repr >> (4 * i)) & 15;
    }

    void Set(int i, int value) {
        repr &= ~(uint64_t(15) << (4 * i));
        repr |= uint64_t(value) << (4 * i);
    }

    inline void Canonize() {
        static unsigned char mp[16];
        memset(mp, 0xff, sizeof(mp));

        mp[0] = 0;

        int next = 1;

        for (int i = 0; i < Params.N; i++) {
            int x = Get(i);
            if (mp[x] == 0xff)
                mp[x] = next++;
            Set(i, mp[x]);
        }
    }

    // verifies canonicity. checks that no two colors are adjacent, except possibly at brk-1 and brk
    void Verify(int brk) const {
        int m = 0;
        for (int i = 0; i < Params.N; i++) {
            if (Get(i) > m) {
                assert(Get(i) == m + 1);
                m++;
            }
            assert(i == 0 || i == brk || Get(i-1) != Get(i));
        }
    }

    // Fills all except first num_filled cells with some garbage so
    // that it can be Canonized()d (no two adjacent cells may have same color
    // in the added values)
    inline void Canonize2(int num_filled) {
        int c = 0;
        if (num_filled > 0)
           c = Get(num_filled - 1) == 0 ? 1 : 0;

        for (int i = num_filled; i < Params.N; i++) {
            Set(i, c);
            c ^= 1;
        }

        Canonize();
    }

    string ToString() const {
        string res(Params.N, ' ');
        for (int i = 0; i < Params.N; i++)
            res[i] = (Get(i) < 10 ? '0' : ('A' - 10)) + Get(i);
        return res;
    }

    bool operator <(Config other) const {
        return repr < other.repr;
    }
    
    bool operator ==(Config other) const {
        return repr == other.repr;
    }

    static Config Zero() {
        Config c;
        c.repr = 0;
        return c;
    }
};

// Represents values of a univariate polynomial at a single global point x=Params.Q
template<int N>
class NumEntry {
    uint32_t a[N];  // least-endian

  public:
    void Set(int num) {
        memset(a, 0, sizeof(a));
        a[0] = num;
    }

    bool IsZero() const {
        for (int i = 0; i < N; i++)
            if (a[i] != 0)
                return false;
        return true;
    }

    string ToString() const {
        int i = N - 1;
        while (i > 0 && a[i] == 0) i--;
        string res = StringPrintf("0x%.X", a[i]);
        while (i-- > 0)
            res += StringPrintf("%.8X", a[i]);
        return res;
    }

    void Add(const NumEntry<N> &e) {
        uint64_t acc = 0;
        for (int i = 0; i < N; i++) {
            a[i] = (uint32_t)(acc += a[i] + (uint64_t)e.a[i]);
            acc >>= 32;
        }
        assert((a[N-1] >> 25) == 0);
    }

    // adds (x-c)*e
    void PolyMulAdd(uint32_t c, const NumEntry &e) {
        c = Params.Q - c;  // >= 0

        uint64_t acc = 0;
        for (int i = 0; i < N; i++) {
            a[i] = (uint32_t)(acc += a[i] + c * (uint64_t)e.a[i]);
            acc >>= 32;
        }
        assert((a[N-1] >> 25) == 0);
    }
};

// TODO: an alternative is to represents a polynomial by an array of coefficients
typedef NumEntry<22> Entry;

vector<Config> Configs;
Entry *CurValues, *NextValues;

size_t GenConfigs(Config c, int k, int max_used, int break_idx, bool dry_run) {
    if (k == Params.N) {
        //printf("%s\n", c.ToString().c_str());
        c.Verify(break_idx);
        if (!dry_run)
            Configs.push_back(c);
        return 1;
    } else {
        size_t res = 0;
        for (int x = 0; x < Params.Q && x <= max_used + 1; x++) {
            if (k > 0 && x == c.Get(k-1)) {
                if (break_idx == -1) {
                    // allow at most one place where two colors are adjacent
                    c.Set(k, x);
                    res += GenConfigs(c, k+1, max(max_used, x), k, dry_run);
                }
            } else {
                c.Set(k, x);
                res += GenConfigs(c, k+1, max(max_used, x), break_idx, dry_run);
            }
        }
        return res;
    }
}

inline size_t GetConfigIndex(Config c) {
    vector<Config>::iterator it = lower_bound(Configs.begin(), Configs.end(), c);
    assert(it != Configs.end() && *it == c);
    return it - Configs.begin();
}

void DoDPStep(int y, int x, int configIndex, bool mustBeZero) {
    Config cfg = Configs[configIndex];
    const Entry &cur_value = CurValues[configIndex];  // number of ways to color until (y,x) cell and end up with cfg

    if (cur_value.IsZero())
        return;

    int max_used = 0;
    if (y == 0) {
        for (int i = 0; i < x; i++)
            max_used = max(max_used, cfg.Get(i));
    } else {
        for (int i = 0; i < Params.N; i++)
            max_used = max(max_used, cfg.Get(i));
    }
    assert(max_used < Params.Q);

    for (int color = 0; color <= max_used && color < Params.Q; color++) {
        if (y > 0 && color == cfg.Get(x)) continue;
        if (x > 0 && color == cfg.Get(x-1)) continue;

        Config new_cfg = cfg;
        new_cfg.Set(x, color);

        if (y == 0)
            new_cfg.Canonize2(x+1);
        else
            new_cfg.Canonize();

        size_t new_idx = GetConfigIndex(new_cfg);
        NextValues[new_idx].Add(cur_value);

        if (mustBeZero)
            return;
    }

    if (max_used + 1 < Params.Q) {
        Config new_cfg = cfg;
        new_cfg.Set(x, max_used + 1);
        if (y == 0)
            new_cfg.Canonize2(x+1);
        else
            new_cfg.Canonize();

        size_t new_idx = GetConfigIndex(new_cfg);
        NextValues[new_idx].PolyMulAdd(max_used + 1, cur_value);   // += (Params.Q - (max_used+1)) * cur_value
    }
}

int main(int argc, char **argv)
{
    if (argc != 3 && argc != 7) {
        printf(
            "Usage:\n"
          "  %s N Q  -- counts q-colorings of n-by-n grid graph\n"
          "  %s N Q i j k l  -- counts colorings with the restriction that (i,j) and (k,l) have the same fixed color.\n",
          argv[0], argv[0]);
        return 1;
    }

    Params.N = atoi(argv[1]);
    Params.Q = atoi(argv[2]);
    Params.i = argc == 7 ? atoi(argv[3]) : -1;
    Params.j = argc == 7 ? atoi(argv[4]) : -1;
    Params.k = argc == 7 ? atoi(argv[5]) : -1;
    Params.l = argc == 7 ? atoi(argv[6]) : -1;

    if (Params.N >= 16 || Params.Q < 1) {  // only 16 integers fit in Config class
        printf("Invalid or unsupported N or Q\n");
        return 1;
    }

    if (log((double)(Params.Q)) * Params.N * Params.N / log(2.0) + 10 > sizeof(Entry)*8) {
        printf("Result is potentially too large, aborting.\n");
        return 1;
    }

    size_t cnt = GenConfigs(Config(), 0, 0, -1, true);
    fprintf(stderr,
        "Found %lld configurations. Memory requirements: %.1lf Mb (using %d-byte entries)\n",
        (long long)cnt,
        cnt / 1048576.0 * (2 * sizeof(Entry) + sizeof(Config)),
        (int)sizeof(Entry)
    );

    CurValues = new Entry[cnt];
    NextValues = new Entry[cnt];

    Configs.reserve(cnt);
    GenConfigs(Config::Zero(), 0, 0, -1, false);
    assert(Configs.size() == cnt);
    sort(Configs.begin(), Configs.end());
    assert(unique(Configs.begin(), Configs.end()) == Configs.end());

    memset(&CurValues[0], 0, sizeof(Entry) * cnt);
    CurValues[0].Set(1);  // any index will do, we're ignoring garbage values at y=0

    for (int y = 0; y < Params.N; y++) {
        for (int x = 0; x < Params.N; x++) {
            memset(&NextValues[0], 0, sizeof(Entry) * cnt);

            bool mustBeZero = (y == Params.i && x == Params.j) || (y == Params.k && x == Params.l);
            for (size_t i = 0; i < cnt; i++)
                DoDPStep(y, x, i, mustBeZero);

            swap(CurValues, NextValues);

            fprintf(stderr, "\r%.0lf%% completed (last completed cell: y=%d x=%d)", (y*Params.N+x+1)*100.0/Params.N/Params.N, y, x);
        }
    }
    fprintf(stderr, "\n");

    // Now we have CurValues[i] = number of ways to color the grid and end up with i-th config in the bottom row.
    // Answer is the sum of all that numbers.
    Entry sum;
    memset(&sum, 0, sizeof(sum));
    for (size_t i = 0; i < cnt; i++)
        sum.Add(CurValues[i]);

    printf("%s\n", sum.ToString().c_str());
    return 0;
}
