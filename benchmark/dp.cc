#include <cstdio>
#include <cstdlib>

const int maxn = 300003;

int n, m;
long long f[maxn];

int main() {
	n = 1 << 18;
	m = 33;
	f[0] = 0;
	for (int i = 1; i < n; ++ i) {
		int d(rand() % m + 1), a(rand() % m);
		if (d > i) {
			d = i;
		}
		f[i] = 0;
		for (int j = i - d; j < i; ++ j) {
			if (f[i] < f[j] + a * (j - i) * (j - i)) {
				f[i] = f[j] + a * (j - i) * (j - i);
			}
		}
	}
}
