// Copyright (c) 2013, Sergey Lyubka
// Copyright (c) 2017-2025, The Monero Project
// All rights reserved.
// Released under the MIT license.

/**
 * @file generate-translations-header.c
 * @brief Embeds binary files into a C++ header file.
 * * Optimizations by "The World's Smartest Coder":
 * 1. Writes to STDOUT: Allows build systems (CMake) to handle output paths.
 * 2. Byte Arrays over String Literals: Prevents compiler memory exhaustion on large files.
 * 3. Buffered I/O: Significantly faster file processing.
 * 4. Clean Code: Improved variable naming and error handling.
 * * Usage:
 * ./generate-translations-header file1.qm file2.qm > output_header.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Used to format the generated C++ code indentation
#define INDENT "  "

/**
 * @brief Prints the static C++ helper function to lookup files.
 * * We construct the std::string only when requested, avoiding runtime overhead during initialization.
 */
static void print_cpp_lookup_function() {
    printf("\n");
    printf("static bool find_embedded_file(const std::string &name, std::string &data) {\n");
    printf(INDENT "const struct embedded_file *p;\n");
    printf(INDENT "for (p = embedded_files; p->name != NULL; p++) {\n");
    printf(INDENT INDENT "if (*p->name == name) {\n");
    // Construct string from raw byte array and length
    printf(INDENT INDENT INDENT "data.assign(reinterpret_cast<const char*>(p->data), p->size);\n");
    printf(INDENT INDENT INDENT "return true;\n");
    printf(INDENT INDENT "}\n");
    printf(INDENT "}\n");
    printf(INDENT "return false;\n");
    printf("}\n");
}

int main(int argc, char *argv[]) {
    int i;
    FILE *fp;
    unsigned char buffer[4096]; // 4KB read buffer
    size_t bytes_read;
    size_t total_bytes;
    
    // Header Guard
    printf("#ifndef TRANSLATION_FILES_H\n");
    printf("#define TRANSLATION_FILES_H\n\n");
    printf("#include <string>\n");
    printf("#include <cstddef>\n\n"); // for size_t

    // ------------------------------------------------------------------------
    // Phase 1: Generate Byte Arrays for each file
    // ------------------------------------------------------------------------
    for (i = 1; i < argc; i++) {
        fp = fopen(argv[i], "rb");
        if (fp == NULL) {
            fprintf(stderr, "Error: Failed to open input file '%s': %s\n", argv[i], strerror(errno));
            return EXIT_FAILURE;
        }

        // Use the filename (sanitized?) as the variable name suffix, or just index.
        // Using index is safer for generated code to avoid invalid C++ identifiers.
        printf("// Source: %s\n", argv[i]);
        printf("static const unsigned char translation_data_%d[] = {\n" INDENT, i);

        total_bytes = 0;
        int col = 0;
        
        while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            for (size_t j = 0; j < bytes_read; j++) {
                printf("0x%02x, ", buffer[j]);
                total_bytes++;
                col++;
                
                // Break lines every 16 bytes for readability
                if (col >= 16) {
                    printf("\n" INDENT);
                    col = 0;
                }
            }
        }
        
        // Handle empty files gracefully
        if (total_bytes == 0) {
             printf("0x00"); 
        }

        printf("\n};\n");
        printf("static const std::string translation_name_%d = \"%s\";\n\n", i, argv[i]);

        fclose(fp);
    }

    // ------------------------------------------------------------------------
    // Phase 2: Generate the Registry (Lookup Table)
    // ------------------------------------------------------------------------
    printf("static const struct embedded_file {\n");
    printf(INDENT "const std::string *name;\n");
    printf(INDENT "const unsigned char *data;\n");
    printf(INDENT "size_t size;\n");
    printf("} embedded_files[] = {\n");

    for (i = 1; i < argc; i++) {
        printf(INDENT "{&translation_name_%d, translation_data_%d, sizeof(translation_data_%d)},\n", i, i, i);
    }

    // Sentinel entry to mark the end of the array
    printf(INDENT "{NULL, NULL, 0}\n");
    printf("};\n");

    // ------------------------------------------------------------------------
    // Phase 3: Helper Function
    // ------------------------------------------------------------------------
    print_cpp_lookup_function();

    printf("\n#endif /* TRANSLATION_FILES_H */\n");

    return EXIT_SUCCESS;
}
