#include <cstdio>
#include <cstdlib>
#include <algorithm>

const int maxn = 300003;

int n;
long long f[maxn], m;

int main() {
	n = 1 << 18;
	for (int i = 0; i < n; ++ i) {
		f[i] = rand();
	}	
	std::make_heap(f, f + n);
	std::sort(f, f + n);
}
