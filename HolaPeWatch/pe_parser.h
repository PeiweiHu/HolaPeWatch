#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "windows.h"
#include <CommCtrl.h>

#include "resource.h"

struct error_struct {
	char * reason;
	int level; // 0 - must terminate 1 - can skip
	struct error_struct * next;
};

struct tree_node {
	char * name;
	struct tree_node *sub_node;
	struct tree_node *sibling;
};


// var decl
// for pe handle
extern IMAGE_DOS_HEADER dos_header;
extern IMAGE_NT_HEADERS nt_headers;
extern IMAGE_FILE_HEADER file_header;
extern WORD number_of_sections;
extern IMAGE_OPTIONAL_HEADER32 optional_header;
extern IMAGE_SECTION_HEADER section_header[1000];
extern IMAGE_IMPORT_DESCRIPTOR import_desc[1000];
extern int import_desc_cnt;
extern IMAGE_EXPORT_DIRECTORY export_directory;
// for error handle
extern struct error_struct error_head;

extern HWND hMainWnd;

// func decl
DWORD rva_to_raw(DWORD rva);
char * get_str(DWORD rva, FILE * pfile);
int which_section(DWORD rva);
char ** get_first_level();
void assign_var(char * file_path);
unsigned char * all_content(char * path);
long get_filesize(char * path);
void add_error(char * reason, int level);
int need_terminate();
char ** get_key_error();
int get_error_cnt();
struct tree_node * get_tree_view();