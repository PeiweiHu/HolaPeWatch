#include "pe_parser.h"


// global variable
WNDCLASSEX MainWnd;
HWND hMainWnd;
HWND hEdit;
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
		700,
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
	TVINSERTSTRUCT ts = { 0 };
	ts.item.mask = TVIF_TEXT;
	ts.hParent = parent;
	ts.hInsertAfter = after;
	ts.item.pszText = txt;
	return TreeView_InsertItem(hTreeView, &ts);
}


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


LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
	case WM_CREATE: {
		// create right edit area
		HFONT hfDefault;
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "",
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
			205, 0, 470, 735, hwnd, (HMENU)IDC_EDIT, GetModuleHandle(NULL), NULL);
		if (hEdit == NULL)
			MessageBox(hwnd, "Could not create edit area.", "Error", MB_OK | MB_ICONERROR);
		// set font
		hfDefault = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

		// create left tree view
		hTreeView = CreateWindowEx(WS_EX_CLIENTEDGE, WC_TREEVIEW, "",
			WS_VISIBLE | WS_CHILD | TVS_HASLINES | TVS_HASBUTTONS | TVS_LINESATROOT,
			0, 0, 200, 735, hwnd, (HMENU)IDC_TREEVIEW, GetModuleHandle(NULL), NULL);
		if (hTreeView == NULL)

			MessageBox(hwnd, "Could not create treeview.", "Error", MB_OK | MB_ICONERROR);
	}
	break;
	case WM_SIZE: {
	}
	break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND: // HIWORD(wParam) is event, LOWORD(wParam) is component id. 
		/*
		switch (HIWORD(wParam)) {
			case EN_SETFOCUS: {
				if (LOWORD(wParam) == IDC_EDIT) {
					SetWindowText(hEdit, "Hello\r\nWorld!");
				}
				//MessageBox(hwnd, "Focus.", "Error", MB_OK | MB_ICONERROR);
			}
		}
		*/
		switch (LOWORD(wParam)) {
			case ID_FILE_OPEN: {
				OPENFILENAME ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = hwnd;
				ofn.lpstrFilter = "Exe File (*.exe)\0*.exe\0Dll File (*.dll)\0*.dll\0All Files (*.*)\0*.*\0";
				ofn.lpstrFile = filename;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
				ofn.lpstrDefExt = "exe";
				if (!GetOpenFileName(&ofn)) {
					char buf[MAX_PATH];
					wsprintf(buf, "%d", GetLastError());
					my_error[error_index++] = buf;
				}
				// pass file to functional part
				assign_var(filename);
				char ** res = get_first_level();
				char * pos = strrchr(filename, '\\');
				char name[MAX_PATH];
				strcpy(name, pos+1);
				HTREEITEM root = add_node(name, NULL, TVI_FIRST);
				int i = 0;
				while (res[i] != "END") {
					add_node(res[i++], root, TVI_LAST);
				}
			}
			break;
		} /*switch (LOWORD(wParam))*/
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}