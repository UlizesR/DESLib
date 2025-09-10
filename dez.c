#include "assembly_vm.h"

int main(int argc, char *argv[]) {
  int verbose = 0;
  char *filename = NULL;

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
      verbose = 1;
    } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
      printf("Usage: %s [options] <assembly_file>\n", argv[0]);
      printf("Options:\n");
      printf("  -v, --verbose    Show detailed execution information\n");
      printf("  -h, --help       Show this help message\n");
      printf("\nExample: %s program.asm\n", argv[0]);
      printf("         %s -v program.asm\n", argv[0]);
      return 0;
    } else if (argv[i][0] != '-') {
      filename = argv[i];
    }
  }

  if (!filename) {
    printf("Usage: %s [options] <assembly_file>\n", argv[0]);
    printf("Use -h or --help for more information\n");
    return 1;
  }

  // Create virtual machine
  vm_t *vm = vm_create();
  if (!vm) {
    printf("Error: Failed to create virtual machine\n");
    return 1;
  }

  // Set verbose mode
  vm_set_verbose(vm, verbose);

  // Load and run program
  if (vm_load_program(vm, filename)) {
    if (verbose) {
      printf("=== Running Assembly Program ===\n");
      vm_print_program(vm); // Show program disassembly
      vm_print_state(vm);
      printf("\n");
    }

    // Run the program continuously
    vm_run(vm);

    if (verbose) {
      printf("\n=== Final State ===\n");
      vm_print_state(vm);
    }
  }

  vm_destroy(vm);
  vm_cleanup_global_resources();
  return 0;
}
