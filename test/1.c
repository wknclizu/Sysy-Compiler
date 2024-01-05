int a;
int func() {
  return 2;
}

int main() {
  int a;
  a = 3;
  a = func();
  a = a + a + a;
  a = a;
  if (a < 3) {
    a = a - 1;
  } else {
    a = a + 1;
  }
  while (a > 3) {
    while (a > 5) {
      a = a - 2;
      continue;
    }
    a = a - 4;
    break;
  }

  return a;
}
