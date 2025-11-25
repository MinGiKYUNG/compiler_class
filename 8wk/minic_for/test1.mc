int sum(int a, int b) {
  int s = a + b;
  return s;
}

int main() {
  int x = 10, y = 20;
  while (x < y) {
    x = -x;
  }
  if (x == y) return sum(x, y);
  return 0;
}
