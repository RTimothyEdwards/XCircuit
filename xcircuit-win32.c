#include <windows.h>

#define CMDLINE_SIZE 4096
/*
#define WISH_EXE L"C:/Software/TclTk/bin/wish84.exe"
#define XCLIBDIR L"C:/Software/xcircuit/xcircuit-3.4"
*/

wchar_t *parse_args(LPWSTR *wargv, int argc)
{
	int i;
	static wchar_t buffer[1024];

	buffer[0] = L'\0';
	for (i=1; i<argc; i++) {
		if (_waccess(wargv[i], 0) == 0) {
			wchar_t *c;
			while ((c=wcschr(wargv[i], L'\\')) != NULL)
				*c = L'/';
			wcscat(buffer, L"\\\"");
			wcscat(buffer, wargv[i]);
			wcscat(buffer, L"\\\" ");
		} else {
			/*wcscat(buffer, wargv[i]);
			wcscat(buffer, L" ");*/
		}
	}

	return buffer;
}

static wchar_t wish_exe[1024] = {0};
static wchar_t lib_path[1024] = {0};

int look_paths()
{
	HKEY hKey;
	DWORD bufLen = 1024 * sizeof(wchar_t);
	wchar_t buf[1100];
	int i;

	if (RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\App Paths\\xcircuit-dev-win32.exe", &hKey))
		return -1;
	if (RegQueryValueExW(
				hKey,
				L"WishExe",
				NULL,
				NULL,
				(LPBYTE)wish_exe,
				&bufLen)) {
		RegCloseKey(hKey);
		return -1;
	}
	bufLen = 1024 * sizeof(wchar_t);
	if (RegQueryValueExW(
				hKey,
				L"LibPath",
				NULL,
				NULL,
				(LPBYTE)lib_path,
				&bufLen)) {
		RegCloseKey(hKey);
		return -1;
	}
	for (i=0; lib_path[i] != 0; i++)
		if (lib_path[i] == L'\\')
			lib_path[i] = L'/';
	_snwprintf(buf, 1100, L"XCIRCUIT_LIB_DIR=%ws", lib_path);
	_wputenv(buf);
	RegCloseKey(hKey);
	return 0;
}

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int cmdShow)
{
	LPWSTR *wargv;
	wchar_t *cmdLine, *arg_str;
	int argc, i;
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;

	wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
	if (wargv != NULL) {
		cmdLine = (wchar_t*)malloc(sizeof(wchar_t)*CMDLINE_SIZE);
		arg_str = parse_args(wargv, argc);
		if (look_paths())
			goto error;
		_snwprintf(cmdLine, CMDLINE_SIZE,
				L"\"%ws\" \"%ws/tkcon.tcl\" -eval \"source \"\"%ws/console.tcl\"\"\" -slave \"package require Tk; set argv [list %ws]; set argc [llength argv]; source \"\"%ws/xcircuit.tcl\"\"\"",
				wish_exe, lib_path, lib_path, arg_str, lib_path);
		/*MessageBoxW(NULL, cmdLine, L"Info", MB_OK);*/
		ZeroMemory(&si, sizeof(STARTUPINFOW));
		si.cb = sizeof(STARTUPINFOW);
		if (!CreateProcessW(NULL,
				    cmdLine,
				    NULL,
				    NULL,
				    FALSE,
				    0,
				    NULL,
				    NULL,
				    &si,
				    &pi)) {
			goto error;
		}
		goto success;

error:
		MessageBox(NULL, "Unable to start xcircuit process", "Error", MB_OK);

success:
		LocalFree(wargv);
		free(cmdLine);
	} 

	return 1;
}
