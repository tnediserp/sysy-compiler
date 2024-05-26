int fib(int n) {
  if (n <= 2) {
    return 1;
  } else {
    return fib(n - 1) + fib(n - 2);
  }
}

int main() {
  int input = getint();
  int input = 10;
  putint(fib(input));
  putch(10);
  return 0;
}