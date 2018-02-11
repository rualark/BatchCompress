// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// BatchCompress.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BatchCompress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

map<CString, char> video_ext, jpeg_ext, image_ext, remove_ext;
CString cmd_line, my_path, my_dir, dir, ffmpeg_path, magick_path;
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

void AppendLineToFile(CString fname, CString st)
{
	ofstream outfile;
	outfile.open(fname, ios_base::app);
	outfile << st;
	outfile.close();
}

void WriteLog(CString st) {
	cout << st;
	AppendLineToFile(my_dir + "\\BatchCompress.log", st);
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
	// Remove file
	if (remove_ext[ext]) {
		cout << "+ Remove file: " << path1 << "\n";
		DeleteFile(fname);
		return;
	}
	if (!video_ext[ext] && !image_ext[ext] && !jpeg_ext[ext]) {
		cout << "- Ignore ext: " << path1 << "\n";
		return;
	}
	long long size1 = FileSize(fname);
	if (size1 < 100) {
		cout << "Ignore small size: " << path1 << ": " << size1 << "\n";
		return;
	}
	if (fname.Find("-conv.mp4") != -1 || fname.Find("-converted.mp4") != -1 ||
		fname.Find("-conv.jpg") != -1) {
		cout << "- Ignore converted: " << path1 << "\n";
		return;
	}
	if (fname.Find("-noconv.mp4") != -1 || fname.Find("-noconv.jpg") != -1) {
		cout << "- Ignore noconv: " << path1 << "\n";
		return;
	}
	// Run
	cout << "+ Process: " << path1 << "\n";
	CString fname2 = noext_from_path(fname) + "-conv";
	if (video_ext[ext]) fname2 += ".mp4";
	if (jpeg_ext[ext]) fname2 += ".jpg";
	if (image_ext[ext]) fname2 += ".jpg";
	CString fname3 = noext_from_path(fname) + "-noconv" + ext;
	if (fileExists(fname2)) {
		cout << "! Overwriting file: " + fname2 << "\n";
	}
	CString par;
	int ret;
	if (video_ext[ext]) {
		par.Format("-y -i \"%s\" -preset slow -crf 20 -b:a 128k \"%s\"",
			fname, fname2);
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (image_ext[ext]) {
		par.Format("convert \"%s\" -resize 1920 \"%s\"",
			fname, fname2);
		ret = RunTimeout(magick_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (jpeg_ext[ext]) {
		par.Format("-y -i \"%s\" -q:v 2 -vf scale='min(1920,iw)':-2 \"%s\"",
			fname, fname2);
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
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
		DeleteFile(fname2);
		rename(fname, fname3);
		return;
	}
	if (size2 < size1) {
		est.Format("+ Compressed %s to %.0lf%% from %.1lf Mb\n",
			fname, round(size2 * 100.0 / size1), round(size1 / 1024 / 1024));
		WriteLog(est);
		DeleteFile(fname);
	}
	else {
		est.Format("- Could not compress %s better (%.0lf%% from %.1lf Mb)\n",
			fname, round(size2 * 100.0 / size1), round(size1 / 1024 / 1024));
		cout << est;
		DeleteFile(fname2);
		rename(fname, fname3);
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
	CString st;
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
	// Get current dir
	if (dir == "") {
		TCHAR buffer[MAX_PATH];
		GetCurrentDirectory(MAX_PATH, buffer);
		dir = string(buffer).c_str();
	}
	cout << "This application compresses all video and jpeg files in folder recursively and removes source files if compressed to smaller size\n";
	cout << "Usage: [folder]\n";
	cout << "If started without parameter, will process current folder\n";
	cout << "Program path: " << my_path << "\n";
	cout << "Program dir: " << my_dir << "\n";
	cout << "Target dir: " << dir << "\n";
	ffmpeg_path = my_dir + "\\ffmpeg.exe";
	magick_path = my_dir + "\\magick.exe";
	// Check exists
	if (!fileExists(ffmpeg_path)) {
		cout << "Not found file " << ffmpeg_path << "\n";
		nRetCode = 10;
		return;
	}
	if (!fileExists(magick_path)) {
		cout << "Not found file " << magick_path << "\n";
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
	video_ext[".wmv"] = 1;
	video_ext[".avi"] = 1;
	video_ext[".flv"] = 1;
	video_ext[".mp4"] = 1;
	video_ext[".asf"] = 1;
	video_ext[".mov"] = 1;
	video_ext[".3gp"] = 1;
	video_ext[".m4v"] = 1;
	video_ext[".mpg"] = 1;

	image_ext[".cr2"] = 1;
	image_ext[".crw"] = 1;
	image_ext[".arw"] = 1;
	image_ext[".mrw"] = 1;
	image_ext[".nef"] = 1;
	image_ext[".orf"] = 1;
	image_ext[".raf"] = 1;
	image_ext[".x3f"] = 1;

	jpeg_ext[".jpg"] = 1;
	jpeg_ext[".jpeg"] = 1;

	remove_ext[".ithmb"] = 1;
	remove_ext[".videocache"] = 1;
	remove_ext[".dat"] = 1;
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
