#ifdef __CLASSIC_C__
int main()
{
  int ac;
  char* av[];
#else
int main(int ac, char* av[])
{
#endif
  if (ac > 1000) {
    return *av[0];
  }
  return 0;
}
