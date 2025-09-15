#include "dez_assembler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_usage(const char *program_name) {
  printf("Dez VM Assembler\n");
  printf("Usage: %s [options] <input_file> [output_file]\n", program_name);
  printf("\nOptions:\n");
  printf("  -v, --verbose    Enable verbose output\n");
  printf("  -h, --help       Show this help message\n");
  printf("  -o <file>        Specify output file\n");
  printf("  --hex            Output in hex format (default: binary)\n");
  printf("\nExamples:\n");
  printf("  %s program.asm\n", program_name);
  printf("  %s -v program.asm program.dez\n", program_name);
  printf("  %s --hex program.asm program.hex\n", program_name);
}

int main(int argc, char *argv[]) {
  const char *input_file = NULL;
  const char *output_file = NULL;
  bool verbose = false;
  bool hex_output = false;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      print_usage(argv[0]);
      return 0;
    } else if (strcmp(argv[i], "-v") == 0 ||
               strcmp(argv[i], "--verbose") == 0) {
      verbose = true;
    } else if (strcmp(argv[i], "--hex") == 0) {
      hex_output = true;
    } else if (strcmp(argv[i], "-o") == 0) {
      if (i + 1 < argc) {
        output_file = argv[++i];
      } else {
        printf("Error: -o requires a filename\n");
        return 1;
      }
    } else if (argv[i][0] != '-') {
      if (input_file == NULL) {
        input_file = argv[i];
      } else if (output_file == NULL) {
        output_file = argv[i];
      } else {
        printf("Error: Too many input files\n");
        return 1;
      }
    } else {
      printf("Error: Unknown option '%s'\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
  }

  // Check for required input file
  if (input_file == NULL) {
    printf("Error: No input file specified\n");
    print_usage(argv[0]);
    return 1;
  }

  // Generate output filename if not specified
  if (output_file == NULL) {
    const char *ext = strrchr(input_file, '.');
    if (ext && strcmp(ext, ".asm") == 0) {
      // Replace .asm with .dez or .hex
      int len = ext - input_file;
      char *base = malloc(len + 5);
      strncpy(base, input_file, len);
      base[len] = '\0';
      strcat(base, hex_output ? ".hex" : ".dez");
      output_file = base;
    } else {
      // Append .dez or .hex
      output_file = hex_output ? "output.hex" : "output.dez";
    }
  }

  // Initialize assembler
  assembler_t assembler;
  assembler_init(&assembler, 8192, verbose); // 8K instruction capacity

  // Assemble file
  bool success = assembler_assemble_file(&assembler, input_file, output_file);

  // Cleanup
  assembler_cleanup(&assembler);

  return success ? 0 : 1;
}
