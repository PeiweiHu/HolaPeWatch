#include "pe_parser.h"


// global variable
WNDCLASSEX MainWnd;
HWND hMainWnd;
HWND hListView;
HWND hTreeView;
MSG msg;

char filename[MAX_PATH] = "";

// error related
char * my_error[1000];
int error_index = 0;

// function decl
LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


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
		WS_EX_CLIENTEDGE,
		"mainwnd",
		"HolaPeWatch",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1400,
		800,
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


// add tree node to left area 
HTREEITEM add_node(LPSTR txt, HTREEITEM parent, HTREEITEM after) {
	TVINSERTSTRUCT ts = { 0 }; // TreeView Insert Struct
	ts.item.mask = TVIF_TEXT;
	ts.hParent = parent;
	ts.hInsertAfter = after;
	ts.item.pszText = txt;
	return TreeView_InsertItem(hTreeView, &ts);
}

//

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow) {
	register_window();
	create_window(nCmdShow);

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return 0;
}

// cols is a string array which ends with "END", e.g. char * buf[] = {"col1", "col2", "END" };
void add_cols(char * cols[]) {
	LV_COLUMN lvc = { 0 };
	lvc.cchTextMax = MAX_PATH;
	lvc.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_FMT;
	lvc.cx = 150;
	lvc.fmt = LVCFMT_LEFT;
	int index = 0;
	while (strcmp(*cols, "END") != 0) {
		lvc.pszText = *cols;
		cols++;
		ListView_InsertColumn(hListView, index++, &lvc);
	}
	return;
}

// add item to list view, both row and col start from 0. (you'd better invoke this function in order [0,0] [0,1] [1,0] ... )
void add_item(char * txt, int row, int col) {
	LVITEM lvt = { 0 };
	lvt.mask = LVIF_TEXT;
	lvt.pszText = txt;
	lvt.cchTextMax = MAX_PATH;
	lvt.iItem = row;
	lvt.iSubItem = col;
	if (col == 0) {
		ListView_InsertItem(hListView, &lvt);
	}
	else {
		ListView_SetItem(hListView, &lvt);
	}
}


// clear both items and columns in list view
void clear_listview() {
	ListView_DeleteAllItems(hListView);
	HWND hHeader = ListView_GetHeader(hListView);
	int col_cnt = Header_GetItemCount(hHeader);
	col_cnt--;
	for (; col_cnt >= 0; col_cnt--) {
		ListView_DeleteColumn(hListView, col_cnt);
	}
}


// when user clicks filename
int click_filename() {
	// return directly if none filename is assigned
	if (strcmp(filename, "") == 0) {
		return -1;
	}
	// get file content and size
	unsigned char * content = all_content(filename);
	long sz = get_filesize(filename);
	if (content == NULL) {
		return -3;
	}
	if (sz == -1) {
		return -2;
	}
	// construct list view
	// columns
	clear_listview();
	char * cols[] = { "offset", "data", "ascii", "END" };
	add_cols(cols);
	// row content
	long offset = 0;
	long row = 0;

	while (offset < sz - 16) {
		char data[100] = { 0 };
		wsprintf(data, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x", content[offset], content[offset + 1], content[offset + 2], \
			content[offset + 3], content[offset + 4], content[offset + 5], content[offset + 6], content[offset + 7], content[offset + 8], \
			content[offset + 9], content[offset + 10], content[offset + 11], content[offset + 12], content[offset + 13], content[offset + 14], \
			content[offset + 15] );
		offset += 16;

		char tmp[1000] = { 0 };
		wsprintf(tmp, "%08x", offset);
		add_item(tmp, row, 0);
		add_item(data, row, 1);
		row++;
		break;
	}
	return 0;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE: {
		// create right list view
		hListView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_LISTVIEW, "",
			WS_CHILD | WS_VISIBLE | LVS_REPORT,
			205, 0, 1170, 735, hwnd, (HMENU)IDC_LISTVIEW, GetModuleHandle(NULL), NULL);
		ListView_SetExtendedListViewStyle(hListView, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

		// create left tree view
		hTreeView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
			WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
			0, 0, 200, 735, hwnd, (HMENU)IDC_TREEVIEW, GetModuleHandle(NULL), NULL);
	}
	break;
	case WM_SIZE: {}
	break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_NOTIFY: {
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

				// get the file name in the full path (filename)
				char * pos = strrchr(filename, '\\');
				char name[MAX_PATH];
				strcpy(name, pos + 1);

				// response
				if (strcmp(buf, name) == 0) {
					int rtn = click_filename();
					char buf[100];
					wsprintf(buf, "%d", rtn);
					MessageBox(hwnd, buf, "cap", NULL);
				} else if (strcmp(buf, "IMAGE_DOS_HEADER") == 0) {

				} else if (strcmp(buf, "IMAGE_NT_HEADERS") == 0) {

				}
			}
		}
	}
	break;
	case WM_COMMAND: // HIWORD(wParam) is event, LOWORD(wParam) is component id. 
		switch (LOWORD(wParam)) {
			case ID_FILE_OPEN: {
				OPENFILENAME ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hwnd;
				ofn.lpstrFilter = "Dll File (*.dll)\0*.dll\0Exe File (*.exe)\0*.exe\0All Files (*.*)\0*.*\0";
				ofn.lpstrFile = filename;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				ofn.lpstrDefExt = "exe";
				if (GetOpenFileName(&ofn)) {
					// pass file to functional part
					assign_var(filename);

					/*char * buf = all_content(filename);
					if (buf != NULL) {
						MessageBox(hwnd, buf, NULL, 0);
					}
					else {
						MessageBox(hwnd, "Fail", NULL, 0);
					}*/

					char ** res = get_first_level();
					char * pos = strrchr(filename, '\\');
					char name[MAX_PATH];
					strcpy(name, pos + 1);
					HTREEITEM root = add_node(name, NULL, TVI_FIRST);
					int i = 0;
					while (res[i] != "END") {
						add_node(res[i++], root, TVI_LAST);
					}
				}
			}
			break;
		} /*switch (LOWORD(wParam))*/
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}