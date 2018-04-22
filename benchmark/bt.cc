#include <cstdio>
#include <cstring>
#include <algorithm>
#include <map>

using namespace std;

typedef long long dint;
#ifdef WIN32
#define lld "%I64d"
#else
#define lld "%lld"
#endif

dint t[100003];
int n, m;

void btChg(int p, int v) {
	for (; p <= n; p += (p & -p))
		t[p] += v;
}
dint btQry(int p) {
	dint s(0);
	for (; p; p -= (p & -p))
		s += t[p];
	return s;
}

int main() {
	n = 100000;
	m = 100000;
	while (m --) {
		int opt, l, r, v;
		opt = rand() & 1;
		l = rand() % n + 1;
		if (opt == 0) {
			r = l + rand() % (n - l + 1);
			v = rand();
			btChg(l, v);
			btChg(r + 1, -v);
		}
		else
			btQry(l);
	}
}

