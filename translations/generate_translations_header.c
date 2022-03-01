// Copyright (c) 2013, Sergey Lyubka
// Copyright (c) 2017-2022, The Monero Project
// All rights reserved.
// Released under the MIT license.

// This program takes a list of files as an input, and produces C++ code that
// contains the contents of all these files as a collection of strings.
//
// Usage:
//   1. Compile this file:
//      cc -o generate-translations-header generate-translations-header.c
//
//   2. Convert list of files into single header:
//      ./generate-translations-header monero_fr.qm monero_it.qm > translations_files.h
//
//   3. In your application code, include translations_files.h, then you can
//      access the files using this function:
//      static bool find_embedded_file(const std::string &file_name, std::string &data);
//      std::string data;
//      find_embedded_file("monero_fr.qm", data);

#include <stdio.h>
#include <stdlib.h>

static const char *code =
  "static bool find_embedded_file(const std::string &name, std::string &data) {\n"
  "  const struct embedded_file *p;\n"
  "  for (p = embedded_files; p->name != NULL; p++) {\n"
  "    if (*p->name == name) {\n"
  "      data = *p->data;\n"
  "      return true;\n"
  "    }\n"
  "  }\n"
  "  return false;\n"
  "}\n";

int main(int argc, char *argv[]) {
  FILE *fp, *foutput;
  int i, j, ch;

  if((foutput = fopen("translation_files.h", "w")) == NULL) {
    exit(EXIT_FAILURE);
  }

  fprintf(foutput, "#ifndef TRANSLATION_FILES_H\n");
  fprintf(foutput, "#define TRANSLATION_FILES_H\n\n");
  fprintf(foutput, "#include <string>\n\n");

  for (i = 1; i < argc; i++) {
    if ((fp = fopen(argv[i], "rb")) == NULL) {
      fclose(foutput);
      exit(EXIT_FAILURE);
    } else {
      fprintf(foutput, "static const std::string translation_file_name_%d = \"%s\";\n", i, argv[i]);
      fprintf(foutput, "static const std::string translation_file_data_%d = std::string(", i);
      for (j = 0; (ch = fgetc(fp)) != EOF; j++) {
        if ((j % 16) == 0) {
          if (j > 0) {
            fprintf(foutput, "%s", "\"");
          }
          fprintf(foutput, "%s", "\n  \"");
        }
        fprintf(foutput, "\\x%02x", ch);
      }
      fprintf(foutput, "\",\n  %d);\n\n", j);
      fclose(fp);
    }
  }

  fprintf(foutput, "%s", "static const struct embedded_file {\n");
  fprintf(foutput, "%s", "  const std::string *name;\n");
  fprintf(foutput, "%s", "  const std::string *data;\n");
  fprintf(foutput, "%s", "} embedded_files[] = {\n");

  for (i = 1; i < argc; i++) {
    fprintf(foutput, "  {&translation_file_name_%d, &translation_file_data_%d},\n", i, i);
  }
  fprintf(foutput, "%s", "  {NULL, NULL}\n");
  fprintf(foutput, "%s", "};\n\n");
  fprintf(foutput, "%s\n", code);

  fprintf(foutput, "#endif /* TRANSLATION_FILES_H */\n");

  fclose(foutput);

  return EXIT_SUCCESS;
}
