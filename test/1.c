int a;
// int func(int b) {
//   return 2;
// }

int func() {
  return 2;
}

int main() {
  int a;
  a = 3;
  a = func();
  a = a + 1;
  a = a + a + a;
  a = a;
  // int b[2];
  // 0 < 0;
  // a < a;
  if (a < 3) {
    a = a - 1;
  } else {
    a = a + 1;
  }
  if (a < 4) {
    
  }
  // a = a - 1;
  // break;
  // continue;
  // while (a < 3) {
  //   a = a - 1;
  //   continue;
  //   a = a - 1;
  //   break;
  // }
  while (a < 11) {
    a = a + 1;
    a = a + 3;
  }

  return 0;
}
