int a;
int func() {
  return 2;
}
int main() {
int a;
  a = 3;
  a = func();
  a = a + 1;
  a = a + a;
  a = a;
  // int b[2];
  // 0 < 0;
  // a < a;
  while (a < 3) {
    a = a - 1;
  }
  return 1;
}
