// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// BatchCompress.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BatchCompress.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

map<CString, char> audio_ext, video_ext, jpeg_ext, image_ext, remove_ext;
CString cmd_line, my_path, my_dir, dir, ffmpeg_path, magick_path, lnkedit_path;
int nRetCode = 0;
int run_minimized = 1;
int ignore_2 = 1;
CString est;
int parameter_found = 0;

// Config
CString ffmpeg_par_audio = "-y -vn -ar 44100 -ac 2 -f mp3 -ab 192k";
CString ffmpeg_par_video = "-y -preset slow -crf 20 -b:a 128k";
CString ffmpeg_par_image = "-y -q:v 2 -vf scale='min(1920,iw)':-2";
CString magick_par_image = "-resize 1920";

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

void read_file_sv(CString fname, vector<CString> &sv) {
	// Load file
	ifstream fs;
	if (!fileExists(fname)) {
		cout << "Cannot find file " + fname + "\n";
		return;
	}
	fs.open(fname);
	char pch[2550];
	CString st2;
	sv.clear();
	while (fs.good()) {
		fs.getline(pch, 2550);
		st2 = pch;
		sv.push_back(st2);
	}
	fs.close();
}

CString noext_from_path(CString path)
{
	string::size_type pos2 = string(path).find_last_of(".");
	CString path2 = string(path).substr(0, pos2).c_str();
	return path2;
}

CString ext_from_path(CString path)
{
	string::size_type pos2 = string(path).find_last_of("./");
	CString path2 = string(path).substr(pos2 + 1, path.GetLength()).c_str();
	return path2;
}

CString fname_from_path(CString path)
{
	string::size_type pos = string(path).find_last_of("\\/");
	CString path2 = string(path).substr(pos + 1).c_str();
	return path2;
}

CString bname_from_path(CString path)
{
	string::size_type pos = string(path).find_last_of("\\/");
	string::size_type pos2 = string(path).find_last_of("./");
	CString path2 = string(path).substr(pos + 1, pos2 - pos - 1).c_str();
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
	CString par, st, st2, st3;
	int ret;
	CString ext = path1.extension().string().c_str();
	CString fname = path1.string().c_str();
	CString fnoext = noext_from_path(fname);
	ext.MakeLower();
	// Remove read-only attribute for all files, because it prevents file deletion
	SetFileAttributes(fname,
		GetFileAttributes(fname) & ~FILE_ATTRIBUTE_READONLY);
	// Process link
	if (ext == ".lnk") {
		// Read link
		cout << "Detected link: " << fname << "\n";
		//cout << "Noext: " << fnoext << "\n";
		CString lnk_path = fnoext + ".lnk-nfo";
		par.Format("\"%s\" \"%s\" \"%s\"", lnkedit_path, fname, lnk_path);
		//cout << "Run: " << my_dir + "\\lnkedit2.bat " << par << "\n";
		ret = RunTimeout(my_dir + "\\lnkedit2.bat", par, 60 * 1000);
		if (ret) {
			cout << "! Error during reading lnk: " << ret << "\n";
			DeleteFile(lnk_path);
			return;
		}
		vector <CString> sv;
		read_file_sv(lnk_path, sv);
		int found = 0;
		for (int i = 0; i < sv.size(); ++i) {
			if (sv[i].Find(":") == -1) continue;
			st = sv[i].Left(sv[i].Find(":"));
			st.Trim();
			if (st != "Target") continue;
			st = sv[i].Mid(sv[i].Find(":") + 1);
			st.Trim();
			cout << "Found target: " << st << "\n";
			found = 1;
			break;
		}
		if (!found) {
			cout << "! Error during reading lnk: no target detected\n";
			DeleteFile(lnk_path);
			return;
		}
		DeleteFile(lnk_path);
		if (fileExists(st)) {
			cout << "Target exists: " + st + "\n";
			return;
		}
		st3 = fname_from_path(st);
		st3.Replace("(", "\\(");
		st3.Replace(")", "\\)");
		st3.Replace("[", "\\[");
		st3.Replace("]", "\\]");
		st3.Replace("+", "\\+");
		st2 = noext_from_path(st) + "-conv." + ext_from_path(st);
		if (fileExists(st2)) {
			cout << "Target exists conv: " + st2 + "\n";
			par.Format("\"%s\" \"%s\" \"%s\"", fname, st3, fname_from_path(st2));
			//cout << "Run: " << lnkedit_path << " " << par << "\n";
			ret = RunTimeout(lnkedit_path, par, 60 * 1000);
			return;
		}
		st2 = noext_from_path(st) + "-noconv." + ext_from_path(st);
		if (fileExists(st2)) {
			cout << "Target exists noconv: " + st2 + "\n";
			par.Format("\"%s\" \"%s\" \"%s\"", fname, st3, fname_from_path(st2));
			//cout << "Run: " << lnkedit_path << " " << par << "\n";
			ret = RunTimeout(lnkedit_path, par, 60 * 1000);
			return;
		}
		st2 = noext_from_path(st) + "-conv.jpg";
		if (fileExists(st2)) {
			cout << "Target exists conv: " + st2 + "\n";
			par.Format("\"%s\" \"%s\" \"%s\"", fname, st3, fname_from_path(st2));
			//cout << "Run: " << lnkedit_path << " " << par << "\n";
			ret = RunTimeout(lnkedit_path, par, 60 * 1000);
			return;
		}
		st2 = noext_from_path(st) + "-noconv.jpg";
		if (fileExists(st2)) {
			cout << "Target exists noconv: " + st2 + "\n";
			par.Format("\"%s\" \"%s\" \"%s\"", fname, st3, fname_from_path(st2));
			//cout << "Run: " << lnkedit_path << " " << par << "\n";
			ret = RunTimeout(lnkedit_path, par, 60 * 1000);
			return;
		}
		cout << "Could not detect similar file\n";
		//abort();
		return;
	}
	// Remove file
	if (remove_ext[ext]) {
		cout << "+ Remove file: " << path1 << "\n";
		DeleteFile(fname);
		return;
	}
	if (fname.Find("_2.jpg") != -1 || fname.Find("_2.JPG") != -1 ||
		fname.Find("_3.jpg") != -1 || fname.Find("_3.JPG") != -1 || 
		fname.Find("_4.jpg") != -1 || fname.Find("_4.JPG") != -1 || 
		fname.Find("_5.jpg") != -1 || fname.Find("_5.JPG") != -1) {
		cout << "- Ignore result: " << path1 << "\n";
		return;
	}
	if (!video_ext[ext] && !image_ext[ext] && !jpeg_ext[ext] && !audio_ext[ext]) {
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
	if (fname.Find("-noconv.") != -1 || fname.Find("-noconv.") != -1) {
		cout << "- Ignore noconv: " << path1 << "\n";
		return;
	}
	// Run
	cout << "+ Process: " << path1 << "\n";
	CString fname2 = fnoext + "-conv";
	if (video_ext[ext]) fname2 += ".mp4";
	if (jpeg_ext[ext]) fname2 += ".jpg";
	if (image_ext[ext]) fname2 += ".jpg";
	if (audio_ext[ext]) fname2 += ".mp3";
	CString fname3 = fnoext + "-noconv" + ext;
	if (fileExists(fname2)) {
		cout << "! Overwriting file: " + fname2 << "\n";
	}
	if (video_ext[ext]) {
		par.Format("-i \"%s\" %s \"%s\"",
			fname, ffmpeg_par_video, fname2);
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (image_ext[ext]) {
		par.Format("convert \"%s\" %s \"%s\"",
			fname, magick_par_image, fname2);
		ret = RunTimeout(magick_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (jpeg_ext[ext]) {
		par.Format("-i \"%s\" %s \"%s\"",
			fname, ffmpeg_par_image, fname2);
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (audio_ext[ext]) {
		par.Format("-i \"%s\" %s \"%s\"",
			fname, ffmpeg_par_audio, fname2);
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (ret) {
		cout << "! Error during running conversion: " << ret << "\n";
		DeleteFile(fname2);
		rename(fname, fname3);
		return;
	}
	if (!fileExists(fname2)) {
		cout << "! File not found: " + fname2 << "\n";
		DeleteFile(fname2);
		rename(fname, fname3);
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
			fname, size2 * 100.0 / size1, size1 / 1024.0 / 1024);
		WriteLog(est);
		DeleteFile(fname);
	}
	else {
		est.Format("- Could not compress %s better (%.0lf%% from %.1lf Mb)\n",
			fname, size2 * 100.0 / size1, size1 / 1024.0 / 1024);
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
	lnkedit_path = my_dir + "\\lnkedit.exe";
	SetCurrentDirectory(my_dir);
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
	jpeg_ext[".jpg"] = 1;
	jpeg_ext[".jpeg"] = 1;
}

void LoadVar(CString * sName, CString * sValue, char* sSearch, int * Dest) {
	if (*sName == sSearch) {
		++parameter_found;
		*Dest = atoll(*sValue);
	}
}

void LoadVar(CString * sName, CString * sValue, char* sSearch, CString * Dest) {
	if (*sName == sSearch) {
		++parameter_found;
		*Dest = *sValue;
	}
}

void LoadConfig() {
	CString st, st2, st3, cur_child;
	ifstream fs;
	CString fname = dir + "\\BatchCompress.pl";
	if (fileExists(fname)) {
		WriteLog("Detected local config file: " + fname + "\n");
	}
	else {
		fname = my_dir + "\\BatchCompress.pl";
	}
	// Check file exists
	if (!fileExists(fname)) {
		WriteLog("LoadConfig cannot find file: " + fname + "\n");
		nRetCode = 3;
		return;
	}
	fs.open(fname);
	char pch[2550];
	int pos = 0;
	int i = 0;
	while (fs.good()) {
		i++;
		// Get line
		fs.getline(pch, 2550);
		st = pch;
		st.Replace("\"", "");
		// Remove unneeded
		pos = st.Find("#");
		// Check if it is first symbol
		if (pos == 0)	st = st.Left(pos);
		pos = st.Find(" #");
		// Check if it is after space
		if (pos > -1)	st = st.Left(pos);
		st.Trim();
		pos = st.Find("=");
		if (pos != -1) {
			// Get variable name and value
			st2 = st.Left(pos);
			st3 = st.Mid(pos + 1);
			st2.Trim();
			st3.Trim();
			st2.MakeLower();
			// Load general variables
			int idata = atoi(st2);
			float fdata = atof(st3);
			parameter_found = 0;
			if (st2 == "image_ext") {
				image_ext[st3] = 1;
				++parameter_found;
			}
			if (st2 == "video_ext") {
				video_ext[st3] = 1;
				++parameter_found;
			}
			if (st2 == "remove_ext") {
				remove_ext[st3] = 1;
				++parameter_found;
			}
			if (st2 == "audio_ext") {
				audio_ext[st3] = 1;
				++parameter_found;
			}
			LoadVar(&st2, &st3, "ffmpeg_par_audio", &ffmpeg_par_audio);
			LoadVar(&st2, &st3, "ffmpeg_par_audio", &ffmpeg_par_video);
			LoadVar(&st2, &st3, "ffmpeg_par_image", &ffmpeg_par_image);
			LoadVar(&st2, &st3, "magick_par_image", &magick_par_image);
			LoadVar(&st2, &st3, "ignore_2", &ignore_2);
			if (!parameter_found) {
				WriteLog("Unrecognized parameter '" + st2 + "' = '" + st3 + "' in file " + fname + "\n");
			}
			if (nRetCode) break;
		}
	}
	fs.close();
	//est.Format("LoadConfig loaded %d lines from %s", i, fname);
	//WriteLog(est);
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
			if (!nRetCode) ParseCommandLine();
			if (!nRetCode) LoadConfig();
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
