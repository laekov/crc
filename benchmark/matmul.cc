#include <cstring>
#include <cstdlib>

const int n = 5;

int a[n][n], b[n][n], c[n][n];

int main() {
	for (int i = 0; i < n; ++ i) {
		for (int j = 0; j < n; ++ j) {
			a[i][j] = rand();
			b[i][j] = rand();
			c[i][j] = 0;
		}
	}
	for (int k = 0; k < n; ++ k) {
		for (int i = 0; i < n; ++ i) {
			for (int j = 0; j < n; ++ j) {
				c[i][j] += a[i][k] * b[k][j];
			}
		}
	}
}
