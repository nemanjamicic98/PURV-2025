#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <elf.h>

// Helper function to print ELF class
void print_elf_class(unsigned char ei_class) {
    switch (ei_class) {
        case ELFCLASS32: printf("ELF32\n"); break;
        case ELFCLASS64: printf("ELF64\n"); break;
        default: printf("Invalid class\n"); break;
    }
}

// Helper function to print ELF data encoding
void print_elf_data(unsigned char ei_data) {
    switch (ei_data) {
        case ELFDATA2LSB: printf("2's complement, little endian\n"); break;
        case ELFDATA2MSB: printf("2's complement, big endian\n"); break;
        default: printf("Invalid data encoding\n"); break;
    }
}

// Helper function to print ELF file type
void print_elf_type(uint16_t e_type) {
    switch (e_type) {
        case ET_NONE: printf("No file type\n"); break;
        case ET_REL: printf("Relocatable file\n"); break;
        case ET_EXEC: printf("Executable file\n"); break;
        case ET_DYN: printf("Shared object file\n"); break;
        case ET_CORE: printf("Core file\n"); break;
        default: printf("Unknown type\n"); break;
    }
}

// Helper function to print ELF machine type
void print_elf_machine(uint16_t e_machine) {
    switch (e_machine) {
        case EM_386: printf("Intel 80386\n"); break;
        case EM_X86_64: printf("x86-64\n"); break;
        default: printf("Unknown machine\n"); break;
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Open the ELF file
    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    // Read the ELF header
    Elf64_Ehdr ehdr; // Use Elf32_Ehdr for 32-bit ELF files
    if (fread(&ehdr, 1, sizeof(ehdr), file) != sizeof(ehdr)) {
        fprintf(stderr, "Error reading ELF header\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    // Check if it is a valid ELF file by verifying the magic number
    if (memcmp(ehdr.e_ident, ELFMAG, SELFMAG) != 0) {
        fprintf(stderr, "Not a valid ELF file\n");
        fclose(file);
        return EXIT_FAILURE;
    }

    // Print ELF header information
    printf("ELF Header:\n");
    //Print first 16 bytes of the file representing the unique identifier of the file type
    printf("  Magic:   ");
    for (int i = 0; i < EI_NIDENT; i++) {
        printf("%02x ", ehdr.e_ident[i]);
    }
    printf("\n");
	//Print architecture of the system for which it was compiled
    printf("  Class:                             ");
    print_elf_class(ehdr.e_ident[EI_CLASS]);
	//Print the method of data representation and the method of its placement on the system
    printf("  Data:                              ");
    print_elf_data(ehdr.e_ident[EI_DATA]);
	//Print version of ELF format
    printf("  Version:                           %d (current)\n", ehdr.e_ident[EI_VERSION]);
	//Print used operating system and ABI (Aplication Binary Interface)
    printf("  OS/ABI:                            ");
    switch (ehdr.e_ident[EI_OSABI]) {
        case ELFOSABI_SYSV: printf("UNIX - System V\n"); break;
        case ELFOSABI_LINUX: printf("UNIX - Linux\n"); break;
        default: printf("Other\n"); break;
    }
	//Print type of file
    printf("  Type:                              ");
    print_elf_type(ehdr.e_type);
	//Print architecture of the computer system on which the file is executed
    printf("  Machine:                           ");
    print_elf_machine(ehdr.e_machine);
    //Print version of object file
    printf("  Version:                           0x%d\n", ehdr.e_version);
	//Print program entry point, the virtual address from which program execution begins
    printf("  Entry point address:               0x%lx\n", (unsigned long)ehdr.e_entry);
	//Print number of bytes (offset) after which the program headers start
    printf("  Start of program headers:          %lu (bytes into file)\n", (unsigned long)ehdr.e_phoff);
	//Print number of bytes (offset) after which the section headers start    
    printf("  Start of section headers:          %lu (bytes into file)\n", (unsigned long)ehdr.e_shoff);
    //Print specific flags related to the file whose header is being viewed
    printf("  Flags:                             0x%x\n", ehdr.e_flags);
    //Print size of elf header in bytes
    printf("  Size of this header:               %u (bytes)\n", ehdr.e_ehsize);
    //Print size of the program header input table in bytes
    printf("  Size of program headers:           %u (bytes)\n", ehdr.e_phentsize);
    //Print number of previous headers
    printf("  Number of program headers:         %u\n", ehdr.e_phnum);
    //Print size of the program section input table in bytes
    printf("  Size of section headers:           %u (bytes)\n", ehdr.e_shentsize);
    //Print number of previous sections
    printf("  Number of section headers:         %u\n", ehdr.e_shnum);
    //Print index of the input section table containing the header section string tables
    printf("  Section header string table index: %u\n", ehdr.e_shstrndx);

    // Close the file
    fclose(file);

    return 0;
}
