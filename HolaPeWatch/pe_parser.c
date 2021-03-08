#include "pe_parser.h"

IMAGE_DOS_HEADER dos_header;
IMAGE_NT_HEADERS nt_headers;
IMAGE_FILE_HEADER file_header;
WORD number_of_sections;
IMAGE_OPTIONAL_HEADER32 optional_header;
IMAGE_SECTION_HEADER section_header[1000];
IMAGE_IMPORT_DESCRIPTOR import_desc[1000];
int import_desc_cnt;
IMAGE_EXPORT_DIRECTORY export_directory;

/*------------------------------error handler--------------------------------------------*/
struct error_struct error_head = { "HEAD", 1, NULL};

void add_error(char * reason, int level) {
	struct error_struct * new_error = (struct error_struct *)malloc(sizeof(struct error_struct));
	new_error->level = level;
	strcpy(new_error->reason, reason);
	// find the end of this link
	struct error_struct *tmp = &error_head;
	while (tmp->next != NULL) {
		tmp = tmp->next;
	}
	// insert
	tmp->next = &new_error;
}

int need_terminate() {
	struct error_struct *tmp = &error_head;
	do {
		if (tmp->level == 0) {
			return 1;
		}
		tmp = tmp->next;
	} while (tmp != NULL);
	return 0;
}

char ** get_key_error() {
	char * key_error[1000];
	int index = 0;
	struct error_struct *tmp = &error_head;
	do {
		if (tmp->level == 1) {
			key_error[index++] = tmp->reason;
		}
		tmp = tmp->next;
	} while (tmp != NULL);
	key_error[index] = "END";
	return key_error;
}

int get_error_cnt() {
	int cnt = 0;
	struct error_struct *tmp = &error_head;
	while (tmp != NULL) {
		cnt++;
		tmp = tmp->next;
	};
	return cnt;
}
/*------------------------------error handler end-----------------------------------------*/


// address translation
DWORD rva_to_raw(DWORD rva) {
	int sec_index = 0;
	for (; sec_index < (int)number_of_sections; sec_index++) {
		if (sec_index == number_of_sections - 1) {
			break;
		}
		else {
			DWORD section_rva = section_header[sec_index].VirtualAddress;
			DWORD section_rva_next = section_header[sec_index + 1].VirtualAddress;
			if (section_rva < rva && section_rva_next > rva) {
				break;
			}
		}
	}
	DWORD raw_addr = section_header[sec_index].PointerToRawData + (rva - section_header[sec_index].VirtualAddress);
	return raw_addr;
}

// get string from raw data, based on rva
char * get_str(DWORD rva, FILE * pfile) {
	char * buf = (char *)malloc(256);
	int index = 0;
	DWORD raw = rva_to_raw(rva);
	fseek(pfile, raw, SEEK_SET);
	do {
		fread(&buf[index++], sizeof(char), 1, pfile);
	} while (buf[index - 1] != 0x00);
	return buf;
}

int which_section(DWORD rva) {
	int sec_index = 0;
	for (; sec_index < (int)number_of_sections; sec_index++) {
		if (sec_index == number_of_sections - 1) {
			break;
		}
		else {
			DWORD section_rva = section_header[sec_index].VirtualAddress;
			DWORD section_rva_next = section_header[sec_index + 1].VirtualAddress;
			if (section_rva < rva && section_rva_next > rva) {
				break;
			}
		}
	}
	return sec_index;
}

void assign_var(char * file_path) {
	// load file
	FILE * pfile = fopen(file_path, "rb+");
	if (pfile == NULL) {
		add_error("Fail to load file in function assign_var, pe_parser.c", 1);
		return;
	}
	// get dos header
	fread(&dos_header, sizeof(IMAGE_DOS_HEADER), 1, pfile);
	if (dos_header.e_magic != IMAGE_DOS_SIGNATURE) { // check dos signature
		add_error("The selected file has invalid dos signature.", 1);
		return;
	}
	// get nt header
	LONG nt_offset = dos_header.e_lfanew;
	fseek(pfile, dos_header.e_lfanew, SEEK_SET);
	fread(&nt_headers, sizeof(IMAGE_NT_HEADERS), 1, pfile);
	if (nt_headers.Signature != IMAGE_NT_SIGNATURE) { // check nt signature
		add_error("The selected file has invalid nt signature.", 1);
		return;
	}
	// get file header and optional header
	file_header = nt_headers.FileHeader;
	optional_header = nt_headers.OptionalHeader;
	number_of_sections = file_header.NumberOfSections;
	// analyse section header
	for (int i = 0; i < (int)number_of_sections; i++) {
		fread(&section_header[i], sizeof(IMAGE_SECTION_HEADER), 1, pfile);
	}
	// anaylse import descriptor
	DWORD import_dir_rva = optional_header.DataDirectory[1].VirtualAddress;
	int sec_index = which_section(import_dir_rva);
	DWORD raw_addr = section_header[sec_index].PointerToRawData + (import_dir_rva - section_header[sec_index].VirtualAddress);
	fseek(pfile, raw_addr, SEEK_SET);
	import_desc_cnt = 0;
	do {
		fread(&import_desc[import_desc_cnt++], sizeof(IMAGE_IMPORT_DESCRIPTOR), 1, pfile);
	} while (import_desc[import_desc_cnt - 1].Name != NULL);
	import_desc_cnt--;
	// analyse export directory
	DWORD export_rva = optional_header.DataDirectory[0].VirtualAddress;
	sec_index = which_section(export_rva);
	raw_addr = section_header[sec_index].PointerToRawData + (export_rva - section_header[sec_index].VirtualAddress);
	fseek(pfile, raw_addr, SEEK_SET);
	fread(&export_directory, sizeof(IMAGE_EXPORT_DIRECTORY), 1, pfile);
	fclose(pfile);
	return;
}

//-----------------comm with GUI----------------

long get_filesize(char * path) {
	FILE * pfile = fopen(path, "rb+");
	if (pfile == NULL) {
		add_error("Fail to load file in function get_filesize, pe_parser.c", 1);
		return -1;
	}

	// get file size
	fseek(pfile, 0, SEEK_END);
	long sz = ftell(pfile);
	fclose(pfile);
	return sz;
}

unsigned char * all_content(char * path) {
	FILE * pfile = fopen(path, "rb+");
	if (pfile == NULL) {
		add_error("Fail to load file in function all_content, pe_parser.c", 1);
		return NULL;
	}

	// get file size
	fseek(pfile, 0, SEEK_END);
	long sz = ftell(pfile);
	fseek(pfile, 0, SEEK_SET);

	// malloc
	unsigned char * buf = (unsigned char *)malloc(sz);
	if (buf == NULL) {
		add_error("Fail to malloc memory in function all_content, pe_parser.c", 1);
		fclose(pfile);
		return NULL;
	}

	// read
	if (fread(buf, sizeof(char), sz, pfile) != sz) {
		add_error("Fail to read ideal number of byte from file in function all_content, pe_parser.c", 1);
		fclose(pfile);
		return NULL;
	}
	else {
		fclose(pfile);
		return buf;
	}
}


char ** get_first_level() {
	char ** first_level = (char **)malloc(100 * sizeof(char*));
	first_level[0] = "IMAGE_DOS_HEADER";
	first_level[1] = "DOS Stub";
	first_level[2] = "IMAGE_NT_HEADERS";
	for (int i = 0; i < number_of_sections; i++) {
		char * prefix = (char *)malloc(MAX_PATH);
		strcpy(prefix, "IMAGE_SECTION_HEADER ");
		first_level[3 + i] = strcat(prefix, (char*)section_header[i].Name);
		//free(prefix);
	}
	first_level[3 + number_of_sections] = "END";
	return first_level;
}