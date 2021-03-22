#include "pe_parser.h"

#define MAINWND_WIDTH 950
#define MAINWND_HEIGHT 605
#define LEFT_WIDTH 250
#define LEFT_HEIGHT 538
#define RIGHT_X_START 255
#define RIGHT_WIDTH 670
#define RIGHT_HEIGHT 538

#define ADDRESS_WIDTH 80
#define DATA_WIDTH 408
#define ASCII_WIDTH 160


typedef struct col_style_s {
	int width;
	char * name;
	int position;
} col_style;

col_style g_col_style1[4] = {
	{ ADDRESS_WIDTH, "address", LVCFMT_CENTER },
	{ DATA_WIDTH, "data", LVCFMT_CENTER },
	{ ASCII_WIDTH, "ascii", LVCFMT_CENTER },
	{ 0, "END", 0 }
};

col_style g_col_style2[4] = {
	{ ADDRESS_WIDTH, "address", LVCFMT_CENTER },
	{ DATA_WIDTH - 250, "data", LVCFMT_CENTER },
	{ ASCII_WIDTH + 250, "description", LVCFMT_LEFT },
	{ 0, "END", 0 }
};

// global variable
WNDCLASSEX MainWnd;
HWND hMainWnd;
HWND hListView;
HWND hTreeView;
MSG msg;

char g_filepath[MAX_PATH] = "";
char g_filename[MAX_PATH] = "HolaPeWatch_waiting_for_file";
char g_last_click[MAX_PATH] = "";    // last clicked item in tree view
unsigned char * g_content;           // the whole content of the seleced file
long g_sz;                           // size of the selected file
char *g_goto_buf = NULL;


// function declaration
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK GotoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam);

// function defination
void register_window() {
	// register MainWnd
	MainWnd.cbSize = sizeof(WNDCLASSEX);
	MainWnd.style = 0;
	MainWnd.lpfnWndProc = MainWndProc;
	MainWnd.cbClsExtra = 0;
	MainWnd.cbWndExtra = 0;
	MainWnd.hInstance = GetModuleHandle(NULL);
	MainWnd.hIcon = NULL;
	MainWnd.hCursor = NULL;
	MainWnd.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	MainWnd.lpszMenuName = MAKEINTRESOURCE(IDM_MENU);
	MainWnd.lpszClassName = "mainwnd";
	MainWnd.hIconSm = NULL;
	if (!RegisterClassEx(&MainWnd)) {
		MessageBox(NULL, "MainWnd registration fails!", "Error", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
}

void create_window(int nCmdShow) {
	// create main window
	hMainWnd = CreateWindowEx(
		WS_EX_CLIENTEDGE ,
		"mainwnd",
		"HolaPeWatch",
		WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		MAINWND_WIDTH,
		MAINWND_HEIGHT,
		NULL,
		NULL,
		GetModuleHandle(NULL),
		NULL
		);
	if (hMainWnd == NULL) {
		MessageBox(NULL, "Main Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return;
	}
	ShowWindow(hMainWnd, nCmdShow);
	UpdateWindow(hMainWnd);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	register_window();
	create_window(nCmdShow);

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

// add tree node to left area 
HTREEITEM add_node(LPSTR txt, HTREEITEM parent, HTREEITEM after) {
	TVINSERTSTRUCT ts = { 0 }; // TreeView Insert Struct
	ts.item.mask = TVIF_TEXT;
	ts.hParent = parent;
	ts.hInsertAfter = after;
	ts.item.pszText = txt;
	return TreeView_InsertItem(hTreeView, &ts);
}

void add_cols(col_style cols[]) {
	// delete old cols
	HWND hHeader = ListView_GetHeader(hListView);
	int col_cnt = Header_GetItemCount(hHeader);
	col_cnt--;
	for (; col_cnt >= 0; col_cnt--) {
		ListView_DeleteColumn(hListView, col_cnt);
	}
	// add new
	col_cnt = 0;
	while (strcmp(cols[col_cnt].name, "END") != 0) {
		LV_COLUMN lvc = { 0 };
		lvc.cchTextMax = MAX_PATH;
		lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
		lvc.cx = cols[col_cnt].width;
		lvc.fmt = cols[col_cnt].position;
		//lvc.fmt = LVCFMT_LEFT;
		lvc.pszText = cols[col_cnt].name;
		ListView_InsertColumn(hListView, col_cnt, &lvc);
		col_cnt++;
	}
	return;
}

char ascii_convert(unsigned char _ch) {
	long ch = (long)_ch;
	if (ch >= 32 && ch <= 126) {
		return (int)ch;
	}
	else {
		return '.';
	}
}

// check g_goto_buf
BOOL go() {
	if (g_goto_buf == NULL) return FALSE;
	// first check format
	// remove 0x 0X
	if (g_goto_buf[0] == '0' && (g_goto_buf[1] == 'x' || g_goto_buf[1] == 'X')) {
		g_goto_buf[0] = '0';
		g_goto_buf[1] = '0';
	}
	int buf_len = strlen(g_goto_buf);
	for (int i = 0; i < buf_len; i++) {
		int ele = *(g_goto_buf + i);
		if (ele < 0 || ele > 255) {
			MessageBox(hMainWnd, "Invalid address", "Error", NULL);
			return FALSE;
		}
		if (!isxdigit(ele)) {
			MessageBox(hMainWnd, "Invalid address", "Error", NULL);
			return FALSE;
		}
	}

	// then check current list view
	// do it later

	long addr;
	sscanf(g_goto_buf, "%x", &addr);
	addr = addr / 16;

	long last_row = g_sz / 16;
	if (addr > last_row || addr < 0) {
		MessageBox(hMainWnd, "Invalid address", "Error", NULL);
		return FALSE;
	}
	ListView_EnsureVisible(hListView, addr, 0);
	free(g_goto_buf);
	g_goto_buf = NULL;
}

void rec_tree_gen(struct tree_node * node, HTREEITEM parent) {
	if (node == NULL) return;
	HTREEITEM hNode = add_node(node->name, parent, TVI_LAST);
	if (node->sub_node != NULL) {
		rec_tree_gen(node->sub_node, hNode);
	}
	if (node->sibling != NULL) {
		rec_tree_gen(node->sibling, parent);
	}
}

void do_create(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	// create right list view
	hListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
		WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_OWNERDATA,
	    RIGHT_X_START, 0, RIGHT_WIDTH, RIGHT_HEIGHT, hwnd, (HMENU)IDC_LISTVIEW, GetModuleHandle(NULL), NULL);
	ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	// set font, copy from [http://winprog dot org/tutorial/zh/fonts_cn dot html]
	// maybe need to be released in destroy
	HFONT hf;
	HDC hdc;
	long lfHeight;

	hdc = GetDC(NULL);
	lfHeight = -MulDiv(11, GetDeviceCaps(hdc, LOGPIXELSY), 72);
	ReleaseDC(NULL, hdc);

	hf = CreateFont(lfHeight, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Consolas");
	SendMessage(hListView, WM_SETFONT, (WPARAM)hf, (LPARAM)TRUE);
	// create left tree view
	hTreeView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
		WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
		0, 0, LEFT_WIDTH, LEFT_HEIGHT, hwnd, (HMENU)IDC_TREEVIEW, GetModuleHandle(NULL), NULL);
}

void do_notify(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	LPNMHDR lpnmh = (LPNMHDR)lParam;
	if (NM_CLICK == lpnmh->code) {
		// click from tree view
		if (lpnmh->hwndFrom == hTreeView) {
			// find the clicked item in tree view
			DWORD dwPos = GetMessagePos();
			POINT pt;
			pt.x = LOWORD(dwPos);
			pt.y = HIWORD(dwPos);
			ScreenToClient(lpnmh->hwndFrom, &pt);
			TVHITTESTINFO ht = { 0 };
			ht.pt = pt;
			ht.flags = TVHT_ONITEM;
			HTREEITEM hItem = TreeView_HitTest(lpnmh->hwndFrom, &ht);
			TVITEM ti = { 0 };
			ti.mask = TVIF_HANDLE | TVIF_TEXT;
			char buf[MAX_PATH] = { 0 };
			ti.cchTextMax = MAX_PATH;
			ti.pszText = buf;
			ti.hItem = hItem;
			TreeView_GetItem(lpnmh->hwndFrom, &ti);
			// save the clicked item to g_last_click
			if (strlen(buf) != 0) {
				//MessageBox(hMainWnd, buf, buf, NULL);
				strcpy(g_last_click, buf);
			}
			// response
			if (strcmp(buf, g_filename) == 0) {
				add_cols(g_col_style1);
				ListView_SetItemCountEx(hListView, g_sz / 16 + 1, NULL);
			}
			else if (strcmp(buf, "IMAGE_DOS_HEADER") == 0) {
				add_cols(g_col_style2);
				ListView_SetItemCountEx(hListView, 31, NULL);
			}
			else if (strcmp(buf, "IMAGE_NT_HEADERS") == 0) {

			}
		}
	} /*if (NM_CLICK == lpnmh->code) end*/
	else if (LVN_GETDISPINFO == lpnmh->code) {
		// getdispinfo for right area
		NMLVDISPINFO* plvdi = (NMLVDISPINFO*)lpnmh;
		int row = plvdi->item.iItem;
		int col = plvdi->item.iSubItem;
		int offset = row * 16; // magic number 16 : one line contains 16 bytes
		// when click file name (tree view root) - maybe a little bit messy, reconstruct later
		if (strcmp(g_last_click, g_filename) == 0) {
			char addr[50];
			char data[100];
			char ascii[100];
			// last line
			if (offset + (16 - 1) > g_sz) {
				int left = g_sz - offset;
				char left_data[100] = "";
				char left_tmp[30] = "";
				char left_ascii[100] = "";
				// I don't like this part, refine later
				for (int i = 0; i < left; i++) {
					wsprintf(left_tmp, "%02x ", g_content[offset + i]);
					strcat(left_data, left_tmp);
					wsprintf(left_tmp, "%01c", ascii_convert(g_content[offset + i]));
					strcat(left_ascii, left_tmp);
				}
				// fill to fix width
				while (strlen(left_data) < 48) {
					strcat(left_data, " ");
				}
				while (strlen(left_ascii) < 16) {
					strcat(left_ascii, " ");
				}
				// pass data
				if (col == 0) {
					wsprintf(addr, "%08x", row * 16);
					plvdi->item.pszText = strupr(addr);
				}
				else if (col == 1){
					plvdi->item.pszText = strupr(left_data);
				}
				else if (col == 2) {
					plvdi->item.pszText = left_ascii;
				}
			}
			// not last line
			else {
				if (col == 0) {
					wsprintf(addr, "%08x", row * 16);
					plvdi->item.pszText = strupr(addr);
				}
				else if (col == 1) {
					wsprintf(data, "%02x %02x %02x %02x %02x %02x %02x %02x  %02x %02x %02x %02x %02x %02x %02x %02x", g_content[offset], g_content[offset + 1], g_content[offset + 2],
						g_content[offset + 3], g_content[offset + 4], g_content[offset + 5], g_content[offset + 6], g_content[offset + 7], g_content[offset + 8],
						g_content[offset + 9], g_content[offset + 10], g_content[offset + 11], g_content[offset + 12], g_content[offset + 13], g_content[offset + 14],
						g_content[offset + 15]);
					plvdi->item.pszText = strupr(data);
				}
				else if (col == 2) {
					wsprintf(ascii, "%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c%01c", ascii_convert(g_content[offset]), ascii_convert(g_content[offset + 1]), ascii_convert(g_content[offset + 2]),
						ascii_convert(g_content[offset + 3]), ascii_convert(g_content[offset + 4]), ascii_convert(g_content[offset + 5]), ascii_convert(g_content[offset + 6]), ascii_convert(g_content[offset + 7]), ascii_convert(g_content[offset + 8]),
						ascii_convert(g_content[offset + 9]), ascii_convert(g_content[offset + 10]), ascii_convert(g_content[offset + 11]), ascii_convert(g_content[offset + 12]), ascii_convert(g_content[offset + 13]), ascii_convert(g_content[offset + 14]),
						ascii_convert(g_content[offset + 15]));
					plvdi->item.pszText = ascii;
				}
			}
		}
		else if (strcmp(g_last_click, "IMAGE_DOS_HEADER") == 0) {
			char addr[50];
			char data[100];
			//char desc[100];
			if (col == 0) {
				long laddr = row * 2;
				wsprintf(addr, "%08x", laddr);
				plvdi->item.pszText = strupr(addr);
			}
			else if (col == 1) {
				if (row <= 29) { // magic number - 29 : corresponding to struct _IMAGE_DOS_HEADER
					wsprintf(data, "%02x%02x", g_content[row * 2 + 1], g_content[row * 2]);
					plvdi->item.pszText = strupr(data);
				}
				else if (row == 30) {
					wsprintf(data, "%02x%02x%02x%02x", g_content[row * 2 + 3], g_content[row * 2 + 2], g_content[row * 2 + 1], g_content[row * 2]);
					plvdi->item.pszText = strupr(data);
				}
			}
			else if (col == 2) {
				switch (row) {
				case 0:
					plvdi->item.pszText = "e_magic, dos signature";
					break;
				case 1:
					plvdi->item.pszText = "e_cblp, bytes on last page of file";
					break;
				case 2:
					plvdi->item.pszText = "e_cp, pages in file";
					break;
				case 3:
					plvdi->item.pszText = "e_crlc, relocations";
					break;
				case 4:
					plvdi->item.pszText = "e_cparhdr, size of header in paragraphs";
					break;
				case 5:
					plvdi->item.pszText = "e_minalloc, minimun extra paragraphs needs";
					break;
				case 6:
					plvdi->item.pszText = "e_maxalloc, maximun extra paragraphs needs";
					break;
				case 7:
					plvdi->item.pszText = "e_ss, intial (relative) ss value";
					break;
				case 8:
					plvdi->item.pszText = "e_sp, intial sp value";
					break;
				case 9:
					plvdi->item.pszText = "e_csum, checksum";
					break;
				case 10:
					plvdi->item.pszText = "e_ip, intial ip value";
					break;
				case 11:
					plvdi->item.pszText = "e_cs, intial (relative) cs value";
					break;
				case 12:
					plvdi->item.pszText = "e_lfarlc, file address of relocation table";
					break;
				case 13:
					plvdi->item.pszText = "e_ovno, overlay number";
					break;
				case 14:
					plvdi->item.pszText = "e_res[0], reserved words";
					break;
				case 15:
					plvdi->item.pszText = "e_res[1], reserved words";
					break;
				case 16:
					plvdi->item.pszText = "e_res[2], reserved words";
					break;
				case 17:
					plvdi->item.pszText = "e_res[3], reserved words";
					break;
				case 18:
					plvdi->item.pszText = "e_oemid, oem identifier (for e_oeminfo)";
					break;
				case 19:
					plvdi->item.pszText = "e_oeminfo, oem information, e_oemid specific";
					break;
				case 20:
					plvdi->item.pszText = "e_res2[0], reserved words";
					break;
				case 21:
					plvdi->item.pszText = "e_res2[1], reserved words";
					break;
				case 22:
					plvdi->item.pszText = "e_res2[2], reserved words";
					break;
				case 23:
					plvdi->item.pszText = "e_res2[3], reserved words";
					break;
				case 24:
					plvdi->item.pszText = "e_res2[4], reserved words";
					break;
				case 25:
					plvdi->item.pszText = "e_res2[5], reserved words";
					break;
				case 26:
					plvdi->item.pszText = "e_res2[6], reserved words";
					break;
				case 27:
					plvdi->item.pszText = "e_res2[7], reserved words";
					break;
				case 28:
					plvdi->item.pszText = "e_res2[8], reserved words";
					break;
				case 29:
					plvdi->item.pszText = "e_res2[9], reserved words";
					break;
				case 30:
					plvdi->item.pszText = "e_lfanew, offset to start of pe header";
					break;
				}
			}
		}
	}
}

void do_command(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {
	case ID_FILE_OPEN: {
		OPENFILENAME ofn;
		ZeroMemory(&ofn, sizeof(ofn));
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFilter = "Dll File (*.dll)\0*.dll\0Exe File (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
		ofn.lpstrFile = g_filepath;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
		ofn.lpstrDefExt = "exe";
		if (GetOpenFileName(&ofn)) {
			// extract file name from path
			char * pos = strrchr(g_filepath, '\\');
			strcpy(g_filename, pos + 1);

			// save selected file to g_content
			g_content = all_content(g_filepath);
			g_sz = get_filesize(g_filepath);

			// pass file to functional part
			assign_var(g_filepath);

			// generate tree view
			struct tree_node * node = get_tree_view();
			HTREEITEM root = add_node(g_filename, NULL, TVI_FIRST);
			rec_tree_gen(node->sub_node, root);


		}
	}
					   break;
	case ID_GOTO: {
		int ret = DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_GOTO), hwnd, GotoDlgProc);
	}
				  break;
	} /*switch (LOWORD(wParam))*/
}


BOOL CALLBACK GotoDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	switch (Message)
	{
	case WM_INITDIALOG:
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_GOTO_OK: {
			// go
			HWND hEdit = GetDlgItem(hwnd, IDC_GOTO_EDIT);
			size_t buffer_len = (GetWindowTextLength(hEdit) + 1) * sizeof(char);
			g_goto_buf = (char *)malloc(buffer_len);
			if (g_goto_buf)
			{
				GetDlgItemText(hwnd, IDC_GOTO_EDIT, (LPTSTR)g_goto_buf, buffer_len);
				//MessageBox(0, szText, TEXT("title"), 0);
				//free(szText);
				//szText = NULL;
			}
			go();
			EndDialog(hwnd, 1);
		}
			break;
		}
		break;
	case WM_SYSCOMMAND:
		if (SC_CLOSE == wParam) {
			EndDialog(hwnd, 0);
		}
		// go to default so user can move dialog window
		return FALSE;
	default:
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE:
		do_create(hwnd, msg, wParam, lParam);
		break;
	case WM_SIZE:
		break;
	case WM_NOTIFY:
		do_notify(hwnd, msg, wParam, lParam);
		break;
	case WM_COMMAND:
		do_command(hwnd, msg, wParam, lParam);
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
