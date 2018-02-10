// BatchCompress.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BatchCompress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

map<CString, char> allowed_ext;
CString cmd_line, my_path, my_dir, dir, ffmpeg_path;
int nRetCode = 0;
int run_minimized = 1;
CString est;

// The one and only application object

CWinApp theApp;

using namespace std;

__int64 FileSize(CString fname)
{
	HANDLE hFile = CreateFile(fname, GENERIC_READ,
		FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return -1; // error condition, could call GetLastError to find out more

	LARGE_INTEGER size;
	if (!GetFileSizeEx(hFile, &size))
	{
		CloseHandle(hFile);
		return -1; // error condition, could call GetLastError to find out more
	}

	CloseHandle(hFile);
	return size.QuadPart;
}

CString dir_from_path(CString path)
{
	string::size_type pos = string(path).find_last_of("\\/");
	CString path2 = string(path).substr(0, pos).c_str();
	return path2;
}

bool dirExists(CString dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return true;   // this is a directory!

	return false;    // this is not a directory!
}

bool fileExists(CString dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	if (ftyp & FILE_ATTRIBUTE_DIRECTORY)
		return false;   // this is a directory!

	return true;    // this is not a directory!
}

CString noext_from_path(CString path)
{
	string::size_type pos2 = string(path).find_last_of("./");
	CString path2 = string(path).substr(0, pos2).c_str();
	return path2;
}

// Start process, wait for some time. If process did not finish, this is an error
int RunTimeout(CString path, CString par, int delay) {
	DWORD ecode;
	SHELLEXECUTEINFO sei = { 0 };
	sei.cbSize = sizeof(SHELLEXECUTEINFO);
	sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_FLAG_NO_UI;
	sei.hwnd = NULL;
	sei.lpVerb = NULL;
	sei.lpFile = path;
	sei.lpParameters = par;
	sei.lpDirectory = NULL;
	if (run_minimized) sei.nShow = SW_SHOWMINNOACTIVE;
	else sei.nShow = SW_SHOWNORMAL;
	sei.hInstApp = NULL;
	ShellExecuteEx(&sei);
	if (WaitForSingleObject(sei.hProcess, delay) == WAIT_TIMEOUT) {
		cout << path + " " + par + ": Timeout waiting for process\n";
		return 100;
	}
	if (!GetExitCodeProcess(sei.hProcess, &ecode)) ecode = 102;
	if (ecode != 0 && ecode != STILL_ACTIVE) { // 259
		cout << "Exit code " << ecode << ": " + path + " " + par + "\n";
		return ecode;
	}
	return 0;
}

void ProcessFile(path path1) {
	CString ext = path1.extension().string().c_str();
	CString fname = path1.string().c_str();
	ext.MakeLower();
	if (!allowed_ext[ext]) {
		cout << "- Ignore ext: " << path1 << "\n";
		return;
	}
	long long size1 = FileSize(fname);
	if (size1 < 100) {
		cout << "Ignore small size: " << path1 << ": " << size1 << "\n";
		return;
	}
	if (fname.Find("-converted.mp4") != -1) {
		cout << "- Ignore converted: " << path1 << "\n";
		return;
	}
	// Run
	cout << "+ Process: " << path1 << "\n";
	CString fname2 = noext_from_path(fname) + "-converted.mp4";
	if (fileExists(fname2)) {
		cout << "! Overwriting file: " + fname2 << "\n";
	}
	CString par;
	par.Format("-y -v quiet -i \"%s\" -preset slow -crf 20 -b:a 128k \"%s\"",
		fname, fname2);
	int ret = RunTimeout(ffmpeg_path, par, 10*24*60*60*1000);
	if (ret) {
		cout << "! Error during running conversion: " << ret << "\n";
	}
	if (!fileExists(fname2)) {
		cout << "! File not found: " + fname2 << "\n";
		nRetCode = 13;
		return;
	}
	long long size2 = FileSize(fname2);
	if (size2 < 100) {
		cout << "! Resulting size too small: " << size2 << "\n";
		nRetCode = 14;
		return;
	}
	if (size2 < size1) {
		cout << "+ Compressed to " << round(size2 * 100.0 / size1) << "% from " << round(size1 / 1024 / 1024) << " Mb\n";
		DeleteFile(fname);
	}
}

void process() {
	for (recursive_directory_iterator i(dir.GetBuffer()), end; i != end; ++i)
		if (!is_directory(i->path())) {
			ProcessFile(i->path());
			if (nRetCode) return;
		}
	// i->path().filename()
}

void ParseCommandLine() {
	cmd_line = ((CWinApp*)::AfxGetApp())->m_lpCmdLine;
	//cout << cmd_line << "\n";
	CString st, st2;
	st = cmd_line;
	st.Trim();
	int pos;
	// Get my path
	st = st.Mid(st.Find('"') + 1);
	my_path = st.Left(st.Find('"'));
	my_dir = dir_from_path(my_path);
	st = st.Mid(st.Find('"') + 1);
	st.Trim();
	// Get dir
	dir = st;
	dir.Replace("\"", "");
	cout << "This application compresses all files in folder recursively and removes source files if compressed to smaller size\n";
	cout << "Usage: [folder]\n";
	cout << "Program path: " << my_path << "\n";
	cout << "Program dir: " << my_dir << "\n";
	cout << "Target dir: " << dir << "\n";
	ffmpeg_path = my_dir + "\\ffmpeg.exe";
	// Check exists
	if (!fileExists(ffmpeg_path)) {
		cout << "Not found file " << ffmpeg_path << "\n";
		nRetCode = 10;
		return;
	}
	if (!dirExists(dir)) {
		cout << "Not found dir " << dir << "\n";
		nRetCode = 11;
		return;
	}
}

void Init() {
	allowed_ext[".wmv"] = 1;
	allowed_ext[".avi"] = 1;
	allowed_ext[".flv"] = 1;
	allowed_ext[".mp4"] = 1;
	allowed_ext[".asf"] = 1;
	allowed_ext[".mov"] = 1;
	allowed_ext[".3gp"] = 1;
	allowed_ext[".m4v"] = 1;
}

int main() {

  HMODULE hModule = ::GetModuleHandle(nullptr);

  if (hModule != nullptr) {
    // initialize MFC and print and error on failure
    if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
      // TODO: change error code to suit your needs
      wprintf(L"Fatal Error: MFC initialization failed\n");
      nRetCode = 1;
    }
    else {
			Init();
			ParseCommandLine();
			if (!nRetCode) process();
    }
  }
  else {
    // TODO: change error code to suit your needs
    wprintf(L"Fatal Error: GetModuleHandle failed\n");
    nRetCode = 1;
  }

#if defined(_DEBUG)
	cout << "Press any key to continue... ";
	_getch();
#endif
	return nRetCode;
}
