
int main(int argc, char ** argv)
{
  if(argc >= 3)
  {
    printf("Exporting log data from '%s' to '%s'\n", argv[1], argv[2]);

    FILE * in = fopen(argv[1], "rb");
    FILE * out = fopen(argv[2], "w");
  }
  else
  {
  }

  return 0;
}

