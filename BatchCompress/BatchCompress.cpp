// BatchCompress.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BatchCompress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CString cmd_line, my_path, my_dir, path1, path2, ffmpeg_path;
int nRetCode = 0;

// The one and only application object

CWinApp theApp;

using namespace std;

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

void process() {
	for (recursive_directory_iterator i(path1.GetBuffer()), end; i != end; ++i)
		if (!is_directory(i->path()))
			cout << i->path() << "\n";
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
	// Get first path
	if (st.Find(' ') < st.Find('"') || st.Find('"') == -1) {
		path1 = st.Left(st.Find(' '));
		st = st.Mid(st.Find(' '));
	}
	else {
		st = st.Mid(st.Find('"') + 1);
		path1 = st.Left(st.Find('"'));
		st = st.Mid(st.Find('"') + 1);
	}
	// Get second path
	st.Trim();
	path2 = st;
	path1.Replace("\"", "");
	path2.Replace("\"", "");
	cout << "Program path: " << my_path << "\n";
	cout << "Program dir: " << my_dir << "\n";
	cout << "Path1: " << path1 << "\n";
	cout << "Path2: " << path2 << "\n";
	ffmpeg_path = my_dir + "\\ffmpeg.exe";
	// Check exists
	if (!fileExists(ffmpeg_path)) {
		cout << "Not found file " << ffmpeg_path << "\n";
		nRetCode = 10;
		return;
	}
	if (!dirExists(path1)) {
		cout << "Not found dir " << path1 << "\n";
		nRetCode = 11;
		return;
	}
	if (!dirExists(path2)) {
		cout << "Not found dir " << path2 << "\n";
		nRetCode = 12;
		return;
	}
	/*
	// Detect switches
	while (st.GetLength() && st[0] == '-') {
		pos = st.Find(' ');
		st2 = st.Left(pos);
		st = st.Right(st.GetLength() - pos - 1);
		if (st2.Find("-test") == 0) {
			CGLib::m_testing = 1;
			if (st2.GetLength() > 6) CGLib::m_test_sec = atoi(st2.Right(st2.GetLength() - 6));
		}
		if (st2.Find("-job") == 0) {
			CGLib::m_testing = 2;
			if (st2.GetLength() > 5) CGLib::m_test_sec = atoi(st2.Right(st2.GetLength() - 5));
		}
	}
	*/
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
