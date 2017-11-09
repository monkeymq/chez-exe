
#include <elf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BITS (__SIZEOF_POINTER__ * __CHAR_BIT__)

#if BITS == 64
	typedef Elf64_Ehdr Elf_Ehdr;
	typedef Elf64_Shdr Elf_Shdr;
	typedef Elf64_Off Elf_Off;
#elif BITS == 32
	typedef Elf32_Ehdr Elf_Ehdr;
	typedef Elf32_Shdr Elf_Shdr;
	typedef Elf32_Off Elf_Off;
#else
# error "Unable to determine machine bits"
#endif

const char *bootname = "chezschemebootfile";
const char *srcname = "schemesource";

char *read_section_header_names(FILE *fp, Elf_Ehdr ehdr) {
	Elf_Shdr strings;
	char *section_header_names;
	fseek(fp, ehdr.e_shoff + sizeof(Elf_Shdr) * ehdr.e_shstrndx, SEEK_SET);
	fread(&strings, sizeof strings, 1, fp);
	section_header_names = calloc(strings.sh_size, 1);
	fseek(fp, strings.sh_offset, SEEK_SET);
	fread(section_header_names, 1, strings.sh_size, fp);

	return section_header_names;
}

size_t extract_elf_section(FILE *fp, uint16_t shnum, Elf_Off shoff, const char *names, const char *name, char **body) {
	Elf_Shdr shdr;
	int count = 0;
	size_t size = 0;
	*body = 0;

	fseek(fp, shoff, SEEK_SET);
	while(count++ < shnum) {
		fread(&shdr, sizeof shdr, 1, fp);
		if (strcmp(&names[shdr.sh_name], name) == 0) {
			size = shdr.sh_size;
			*body = malloc(size);
			fseek(fp, shdr.sh_offset, SEEK_SET);
			fread(*body, 1, size, fp);
			break;
		}
	}

	return size;
}

int main(int argc, char **argv) {
	FILE *fp;
	Elf_Ehdr ehdr;
	char *s_names;
	char *boot;
	size_t bootsize;
	char *scheme;
	size_t schemesize;

	fp = fopen("/proc/self/exe", "rb");
	fread(&ehdr, sizeof ehdr, 1, fp);
	s_names = read_section_header_names(fp, ehdr);
	bootsize = extract_elf_section(fp, ehdr.e_shnum, ehdr.e_shoff, s_names, bootname, &boot);
	schemesize = extract_elf_section(fp, ehdr.e_shnum, ehdr.e_shoff, s_names, srcname, &scheme);

	if (bootsize) printf("Found boot data of size %lu\n", bootsize);
	else printf("No boot data found\n");
	if (schemesize) printf("Found scheme source of size %lu\n", schemesize);
	else printf("No scheme source found\n");

	if (s_names) free(s_names);
	if (boot) free(boot);
	if (scheme) free(scheme);

	fclose(fp);

	return 0;
}
