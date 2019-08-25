// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// BatchCompress.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BatchCompress.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

map<CString, int> audio_ext, video_ext, jpeg_ext, image_ext, remove_ext, ignore_match;
CString cmd_line, my_path, my_dir, dir, ffmpeg_path, magick_path, lnkedit_path, exiftool_path;
int nRetCode = 0;
int run_minimized = 1;
int ignore_2 = 1;
int conv_error_noconv = 0;
CString est;
int parameter_found = 0;

// Config
CString ffmpeg_par_audio = "-y -vn -ar 44100 -ac 2 -f mp3 -ab 192k";
CString ffmpeg_par_video = "-y -preset slow -crf 20 -b:a 128k";
CString ffmpeg_par_image = "-y -q:v 2 -vf scale='min(1920,iw)':-2";
CString magick_par_image = "-resize 1920";
int process_links = 0;
int save_exif = 0;
int rename_xmp = 0;
int strip_tocut = 1;
int shorten_filenames_to = 0;

long long space_release = 0;

// The one and only application object

CWinApp theApp;

using namespace std;

bool isMatch(const string &str, const string &pattern) 
{
	vector<vector<bool>> bool_array(str.size() + 1, vector<bool>(pattern.size() + 1, false));
	//initialize boolean array to false.
	for (int i = 0; i <= str.size(); ++i)
	{
		for (int j = 0; j <= pattern.size(); ++j)
		{
			bool_array[i][j] = 0;
		}
	}
	// base case
	bool_array[0][0] = true;
	for (int i = 1; i <= str.size(); i++)
	{
		for (int j = 1; j <= pattern.size(); j++)
		{
			if (str[i - 1] == pattern[j - 1] || pattern[j - 1] == '?')
				bool_array[i][j] = bool_array[i - 1][j - 1];

			else if (pattern[j - 1] == '*')
				bool_array[i][j] = bool_array[i][j - 1] || bool_array[i - 1][j];
		}
	}
	return bool_array[str.size()][pattern.size()];
}

uintmax_t FileSize2(CString dir_name_ext) {
	path pth = dir_name_ext.GetString();
	uintmax_t sz = -1;
	try {
		sz = file_size(pth);
	}
	catch (filesystem_error& e) {
		std::cout << e.what() << '\n';
	}
	return sz;
}

__int64 FileSize(CString dir_name_ext) {
	HANDLE hFile = CreateFile(dir_name_ext, GENERIC_READ,
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

void AppendLineToFile(CString dir_name_ext, CString st) 
{
	ofstream outfile;
	outfile.open(dir_name_ext, ios_base::app);
	outfile << st;
	outfile.close();
}

void WriteLogShared(CString st) {
	cout << st;
	AppendLineToFile(dir + "\\BatchCompress.log", st);
}

void WriteLog(CString st) {
	cout << st;
	DWORD dwWritten; // number of bytes written to file
	HANDLE hFile;
	for (int i = 0; i < 2000; ++i) {
		hFile = CreateFile(dir + "\\BatchCompress.log", FILE_APPEND_DATA, // FILE_GENERIC_WRITE
			FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
		if (hFile != INVALID_HANDLE_VALUE) break;
		// Sleep if cannot open file
		//cout << "Waiting for another process: " << i << endl;
		this_thread::sleep_for(chrono::milliseconds(1));
	}
	if (hFile == INVALID_HANDLE_VALUE) {
		cout << "Error writing log file\n";
		abort();
	}
	//SetFilePointer(hFile, 0, NULL, FILE_END);
	//WriteFile(hFile, buffer, sizeof(buffer), &dwWritten, 0);
	WriteFile(hFile, st.GetBuffer(), st.GetLength(), &dwWritten, 0);
	//SetEndOfFile(hFile);
	CloseHandle(hFile);
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

bool fileOrDirExists(CString dirName_in)
{
	DWORD ftyp = GetFileAttributesA(dirName_in);
	if (ftyp == INVALID_FILE_ATTRIBUTES)
		return false;  //something is wrong with your path!

	return true;    // this is not a directory!
}

void read_file_sv(CString dir_name_ext, vector<CString> &sv) {
	// Load file
	ifstream fs;
	if (!fileExists(dir_name_ext)) {
		cout << "Cannot find file " + dir_name_ext + "\n";
		return;
	}
	fs.open(dir_name_ext);
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
	string::size_type pos2 = string(path).find_last_of('.');
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

int RemoveReadonlyAndDelete(CString dir_name_ext) {
	// Remove read-only attribute for all files, because it prevents file deletion
	SetFileAttributes(dir_name_ext,
		GetFileAttributes(dir_name_ext) & ~FILE_ATTRIBUTE_READONLY);
	// returns 0 if error deleting
	return DeleteFile(dir_name_ext);
}

class FileName {
public:
	void FromPath(const path &pth) {
		dir_ = pth.parent_path().string().c_str();
		ext_ = pth.extension().string().c_str();
		name_ = noext_from_path(pth.filename().string().c_str());
		ext_.MakeLower();
		UpdateAggregates();
	}
	void SetName(const CString &new_name) {
		name_ = new_name;
		UpdateAggregates();
	}
	void SetExt(const CString &new_ext) {
		ext_ = new_ext;
		UpdateAggregates();
	}
	void UpdateAggregates() {
		dir_name_ = dir_ + "\\" + name_;
		name_ext_ = name_ + ext_;
		dir_name_ext_ = dir_ + "\\" + name_ + ext_;
	}
	// Get
	const CString &dir() { return dir_; }
	const CString &name() { return name_; }
	const CString &ext() { return ext_; }
	const CString dir_name_ext() { return dir_name_ext_; }
	const CString &dir_name() { return dir_name_; }
	const CString &name_ext() { return name_ext_; }
private:
	CString dir_, name_, ext_;
	CString dir_name_;
	CString name_ext_;
	CString dir_name_ext_;
};

class FileLock {
public:
	~FileLock() {
		Unlock();
	}
	int Lock(CString fname) {
		// Do not relock same file
		if (fname == m_fname && hFile != nullptr && hFile != INVALID_HANDLE_VALUE)
			return 0;
		Unlock();
		m_fname = fname;
		hFile = CreateFile(m_fname, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
		if (hFile == INVALID_HANDLE_VALUE) return 1;
		else return 0;
	}
	void Unlock() {
		if (!m_fname.IsEmpty() && hFile != nullptr && hFile != INVALID_HANDLE_VALUE) {
			CloseHandle(hFile);
			RemoveReadonlyAndDelete(m_fname);
		}
	}
private:
	CString m_fname;
	HANDLE hFile;
};

int LockFile(FileLock &lck, FileName &f) {
	if (lck.Lock(f.dir_name() + ".lck")) {
		cout << "! File is locked with a lock file from other BatchCompress.exe process. Skipping file: " +
			f.dir_name_ext() + "\n";
		return 0;
	}
	return 1;
}

int RenameFile(FileName &f, FileName &f2) {
	// returns 0 if successfully renamed
	return rename(f.dir_name_ext(), f2.dir_name_ext());
}

// Rename file with XMP tags (if tags are in a separate file)
int RenameExt(FileName &f, FileName &f2, CString ext) {
	int res = 0;
	if (rename_xmp && fileExists(f.dir_name() + ext)) {
		if (fileExists(f2.dir_name() + ext)) {
			RemoveReadonlyAndDelete(f2.dir_name() + ext);
			res = rename(f.dir_name() + ext, f2.dir_name() + ext);
			cout << "+ Overwriting " + ext + " file: " + f.dir_name() + ext << "\n";
		}
		else {
			res = rename(f.dir_name() + ext, f2.dir_name() + ext);
			cout << "+ Renamed " + ext + " file: " + f.dir_name() + ext << "\n";
		}
	}
	return res;
}

void ProcessFile(const path &path1) {
	FileLock lck;
	CString par, st, st2, st3;
	int ret;
	FileName f;
	try {
		f.FromPath(path1);
	}
	catch (...) {
		cout << "Warning! Next file is unreadable probably because long file name length. Please correct." << endl;
		getchar();
		return;
	}
	// Remove old lock (can exist if previous BatchCompress process was killed or if another process is running - in this case we will not be able to remove file)
	if (f.ext() == ".lck") {
		if (RemoveReadonlyAndDelete(f.dir_name_ext())) {
			cout << "* Removed old lock file: " + f.dir_name_ext() + "\n";
		}
		else {
			cout << "* Cannot remove lock file because it is probably locked: " + f.dir_name_ext() + "\n";
		}
		return;
	}
	if (strip_tocut && f.name().Find("[cut") != -1) {
		int pos, pos2;
		FileName f2 = f;
		pos = f.name().Find(" [to cut");
		pos2 = f.name().Find("]", pos);
		if (pos != -1 && pos2 != -1) {
			if (!LockFile(lck, f)) return;
			f2.SetName(f.name().Left(pos) + f.name().Mid(pos2 + 1));
			if (fileOrDirExists(f2.dir_name_ext())) {
				WriteLog("! Cannot remove [to cut] tag from file with [cut] tag because inode exists: " + 
					f.dir_name_ext() + " to: " + f2.name() + "\n");
			}
			else {
				if (!RenameFile(f, f2)) {
					RenameExt(f, f2, ".xmp");
					WriteLog("+ Removed [to cut] tag from file with [cut] tag: " +
						f.dir_name_ext() + " to: " + f2.name() + "\n");
					f = f2;
					if (!LockFile(lck, f)) return;
				}
				else {
					WriteLog("! Cannot remove [to cut] tag from file with [cut] tag: " +
						f.dir_name_ext() + " to: " + f2.name() + "\n");
				}
			}
		}
		// Same process without space
		pos = f.name().Find("[to cut");
		pos2 = f.name().Find("]", pos);
		if (pos != -1 && pos2 != -1) {
			if (!LockFile(lck, f)) return;
			f2.SetName(f.name().Left(pos) + f.name().Mid(pos2 + 1));
			if (fileOrDirExists(f2.dir_name_ext())) {
				WriteLog("! Cannot remove [to cut] tag from file with [cut] tag because inode exists: " +
					f.dir_name_ext() + " to: " + f2.name() + "\n");
			}
			else {
				if (!RenameFile(f, f2)) {
					RenameExt(f, f2, ".xmp");
					WriteLog("+ Removed [to cut] tag from file with [cut] tag: " +
						f.dir_name_ext() + " to: " + f2.name() + "\n");
					f = f2;
					if (!LockFile(lck, f)) return;
				}
				else {
					WriteLog("! Cannot remove [to cut] tag from file with [cut] tag: " +
						f.dir_name_ext() + " to: " + f2.name() + "\n");
				}
			}
		}
	}
	if (shorten_filenames_to && f.name().GetLength() > shorten_filenames_to) {
		if (!LockFile(lck, f)) return;
		// Save 4 characters for added non-duplication id (like "_123")
		// Save 7 characters for "-conv" or "-noconv"
		FileName f2 = f;
		f2.SetName(f.name().Left(shorten_filenames_to - 11));
		if (f.name().Find("-conv") != -1) {
			f2.SetName(f2.name() + "-conv");
		}
		if (f.name().Find("-noconv") != -1) {
			f2.SetName(f2.name() + "-noconv");
		}
		if (fileOrDirExists(f2.dir_name_ext())) {
			int suffix = 1;
			int found = 0;
			FileName f3 = f2;
			for (int i = 0; i < 1000; ++i) {
				suffix++;
				st.Format("%s%d",	f3.name() + "_", suffix);
				f3.SetName(st);
				if (!fileOrDirExists(f3.dir_name_ext())) {
					found = 1;
					break;
				}
			}
			if (!found) {
				WriteLog("! Cannot shorten file, because too many similar files already exist: " + 
					f.dir_name_ext() + "\n");
			}
			else {
				f2 = f3;
			}
		}
		if (!RenameFile(f, f2)) {
			RenameExt(f, f2, ".xmp");
			WriteLog("+ Shortened file: " + f.dir_name_ext() + " to: " + f2.name() + "\n");
			f = f2;
			if (lck.Lock(f.dir_name() + ".lck")) {
				cout << "! File is locked from other BatchCompress.exe process. Skipping file: " + f.dir_name_ext();
				return;
			}
		}
		else {
			WriteLog("! Cannot rename (shorten) file: " + f.dir_name_ext() + " to: " + f2.name() + "\n");
		}
	}
	// Fix link
	if (f.ext() == ".lnk" && process_links == 1) {
		if (!LockFile(lck, f)) return;
		// Read link
		cout << "Detected link: " << f.dir_name_ext() << "\n";
		//cout << "Noext: " << f.dir_name() << "\n";
		CString lnk_path = f.dir_name() + ".lnk-nfo";
		par.Format("\"%s\" \"%s\" \"%s\"", lnkedit_path, f.dir_name_ext(), lnk_path);
		//cout << "Run: " << my_dir + "\\lnkedit2.bat " << par << "\n";
		ret = RunTimeout(my_dir + "\\lnkedit2.bat", par, 60 * 1000);
		if (ret) {
			cout << "! Error during reading lnk: " << ret << "\n";
			space_release += FileSize(lnk_path);
			RemoveReadonlyAndDelete(lnk_path);
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
			space_release += FileSize(lnk_path);
			RemoveReadonlyAndDelete(lnk_path);
			return;
		}
		space_release += FileSize(lnk_path);
		RemoveReadonlyAndDelete(lnk_path);
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
			par.Format("\"%s\" \"%s\" \"%s\"", f.dir_name_ext(), st3, fname_from_path(st2));
			//cout << "Run: " << lnkedit_path << " " << par << "\n";
			ret = RunTimeout(lnkedit_path, par, 60 * 1000);
			return;
		}
		st2 = noext_from_path(st) + "-noconv." + ext_from_path(st);
		if (fileExists(st2)) {
			cout << "Target exists noconv: " + st2 + "\n";
			par.Format("\"%s\" \"%s\" \"%s\"", f.dir_name_ext(), st3, fname_from_path(st2));
			//cout << "Run: " << lnkedit_path << " " << par << "\n";
			ret = RunTimeout(lnkedit_path, par, 60 * 1000);
			return;
		}
		st2 = noext_from_path(st) + "-conv.jpg";
		if (fileExists(st2)) {
			cout << "Target exists conv: " + st2 + "\n";
			par.Format("\"%s\" \"%s\" \"%s\"", f.dir_name_ext(), st3, fname_from_path(st2));
			//cout << "Run: " << lnkedit_path << " " << par << "\n";
			ret = RunTimeout(lnkedit_path, par, 60 * 1000);
			return;
		}
		st2 = noext_from_path(st) + "-noconv.jpg";
		if (fileExists(st2)) {
			cout << "Target exists noconv: " + st2 + "\n";
			par.Format("\"%s\" \"%s\" \"%s\"", f.dir_name_ext(), st3, fname_from_path(st2));
			//cout << "Run: " << lnkedit_path << " " << par << "\n";
			ret = RunTimeout(lnkedit_path, par, 60 * 1000);
			return;
		}
		cout << "Could not detect similar file\n";
		//abort();
		return;
	}
	// Copy file to link
	if (f.ext() == ".lnk" && process_links == 2) {
		if (!LockFile(lck, f)) return;
		// Read link
		cout << "Detected link: " << f.dir_name_ext() << "\n";
		//cout << "Noext: " << f.dir_name() << "\n";
		CString lnk_path = f.dir_name() + ".lnk-nfo";
		par.Format("\"%s\" \"%s\" \"%s\"", lnkedit_path, f.dir_name_ext(), lnk_path);
		//cout << "Run: " << my_dir + "\\lnkedit2.bat " << par << "\n";
		ret = RunTimeout(my_dir + "\\lnkedit2.bat", par, 60 * 1000);
		if (ret) {
			cout << "! Error during reading lnk: " << ret << "\n";
			space_release += FileSize(lnk_path);
			RemoveReadonlyAndDelete(lnk_path);
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
			space_release += FileSize(lnk_path);
			RemoveReadonlyAndDelete(lnk_path);
			return;
		}
		space_release += FileSize(lnk_path);
		RemoveReadonlyAndDelete(lnk_path);
		if (fileExists(st)) {
			cout << "Target exists: " + st + "\n";
			st3 = f.dir_name_ext();
			st3.Replace(".lnk", "");
			space_release += FileSize(f.dir_name_ext());
			space_release -= FileSize(st);
			copy(st.GetBuffer(), st3.GetBuffer());
			RemoveReadonlyAndDelete(f.dir_name_ext());
			return;
		}
		// Do nothing if link is bad
		return;
	}
	// Remove file
	if (remove_ext[f.ext()]) {
		WriteLog("+ Remove file: " + f.dir_name_ext() + "\n");
		space_release += FileSize(f.dir_name_ext());
		RemoveReadonlyAndDelete(f.dir_name_ext());
		return;
	}
	// Detects "-conv" substring in filename or dirname
	if (f.dir_name_ext().Find("-conv") != -1) {
		cout << "- Ignore converted: " << f.dir_name_ext() << "\n";
		return;
	}
	// Detects "-noconv" substring in filename or dirname
	if (f.dir_name_ext().Find("-noconv") != -1) {
		cout << "- Ignore noconv: " << f.dir_name_ext() << "\n";
		return;
	}
	// Ignore match
	for (const auto &ppr : ignore_match) {
		string s1(f.name_ext());
		string s2(ppr.first);
		if (isMatch(s1, s2)) {
			cout << "- Ignore match: " << f.dir_name_ext() << "\n";
			return;
		}
	}
	if (ignore_2 && (f.dir_name_ext().Find("_2.jpg") != -1 || f.dir_name_ext().Find("_2.JPG") != -1 ||
		f.dir_name_ext().Find("_3.jpg") != -1 || f.dir_name_ext().Find("_3.JPG") != -1 || 
		f.dir_name_ext().Find("_4.jpg") != -1 || f.dir_name_ext().Find("_4.JPG") != -1 || 
		f.dir_name_ext().Find("_5.jpg") != -1 || f.dir_name_ext().Find("_5.JPG") != -1)) {
		cout << "- Ignore manually processed files: " << f.dir_name_ext() << "\n";
		return;
	}
	if (!video_ext[f.ext()] && !image_ext[f.ext()] && !jpeg_ext[f.ext()] && !audio_ext[f.ext()]) {
		cout << "- Ignore ext: " << f.dir_name_ext() << "\n";
		return;
	}
	if (!fileExists(f.dir_name_ext())) {
		cout << "! File does not exist, although was listed by iterator: " + f.dir_name_ext() + "\n";
		return;
	}
	long long size1 = FileSize(f.dir_name_ext());
	if (size1 < 100) {
		cout << "Ignore small size: " << f.dir_name_ext() << ": " << size1 << "\n";
		return;
	}
	// Run
	if (!LockFile(lck, f)) return;
	cout << "+ Process: " << f.dir_name_ext() << "\n";
	FileName fc = f;
	fc.SetName(fc.name() + "-conv");
	if (video_ext[f.ext()]) fc.SetExt(".mp4");
	if (jpeg_ext[f.ext()]) fc.SetExt(".jpg");
	if (image_ext[f.ext()]) fc.SetExt(".jpg");
	if (audio_ext[f.ext()]) fc.SetExt(".mp3");
	FileName fn = f;
	fn.SetName(fn.name() + "-noconv");
	if (fileExists(fc.dir_name_ext())) {
		cout << "! Overwriting file: " + fc.dir_name_ext() << "\n";
	}
	if (video_ext[f.ext()]) {
		CString par = ffmpeg_par_video;
		par.Replace("%ifname%", f.dir_name_ext());
		par.Replace("%ofname%", fc.dir_name_ext());
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (image_ext[f.ext()]) {
		CString par = magick_par_image;
		par.Replace("%ifname%", f.dir_name_ext());
		par.Replace("%ofname%", fc.dir_name_ext());
		ret = RunTimeout(magick_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (jpeg_ext[f.ext()]) {
		CString par = ffmpeg_par_image;
		par.Replace("%ifname%", f.dir_name_ext());
		par.Replace("%ofname%", fc.dir_name_ext());
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (audio_ext[f.ext()]) {
		CString par = ffmpeg_par_audio;
		par.Replace("%ifname%", f.dir_name_ext());
		par.Replace("%ofname%", fc.dir_name_ext());
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (ret) {
		est.Format("! Error during running file " + f.dir_name_ext() + " conversion: %d\n", ret);
		WriteLog(est);
		if (!conv_error_noconv) return;
		RemoveReadonlyAndDelete(fc.dir_name_ext());
		RemoveReadonlyAndDelete(fn.dir_name_ext());
		if (RenameFile(f, fn)) {
			est.Format("! Cannot rename file to " + fn.dir_name_ext() + "\n");
		}
		RenameExt(f, fn, ".xmp");
		return;
	}
	if (!fileExists(fc.dir_name_ext())) {
		est.Format("! File not found: " + f.dir_name_ext() + "\n");
		WriteLog(est);
		if (!conv_error_noconv) return;
		RemoveReadonlyAndDelete(fc.dir_name_ext());
		RemoveReadonlyAndDelete(fn.dir_name_ext());
		if (RenameFile(f, fn)) {
			est.Format("! Cannot rename file to " + fn.dir_name_ext() + "\n");
		}
		RenameExt(f, fn, ".xmp");
		return;
	}
	long long size2 = FileSize(fc.dir_name_ext());
	if (size2 < 100) {
		est.Format("! Resulting size of file " + f.dir_name_ext() + " too small: %lld\n", size2);
		WriteLog(est);
		if (!conv_error_noconv) return;
		RemoveReadonlyAndDelete(fc.dir_name_ext());
		RemoveReadonlyAndDelete(fn.dir_name_ext());
		if (RenameFile(f, fn)) {
			est.Format("! Cannot rename file to " + fn.dir_name_ext() + "\n");
		}
		RenameExt(f, fn, ".xmp");
		return;
	}
	if (size2 < size1) {
		space_release += FileSize(f.dir_name_ext());
		space_release -= FileSize(fc.dir_name_ext());
		est.Format("+ Compressed %s to %.0lf%% from %.1lf Mb (TOTAL FREE %.1lf Mb)\n",
			f.dir_name_ext(), size2 * 100.0 / size1, size1 / 1024.0 / 1024,
			space_release / 1024.0 / 1024.0);
		WriteLog(est);
		RenameExt(f, fc, ".xmp");
		// Copy exif, XMP and other tags inside file
		if (jpeg_ext[f.ext()] && save_exif) {
			par.Format("-tagsFromFile \"%s\" \"%s\"",
				f.dir_name_ext(), fc.dir_name_ext());
			ret = RunTimeout(exiftool_path, par, 10 * 24 * 60 * 60 * 1000);
			if (ret) {
				est.Format("! Error copying exif tags: %d\n", ret);
				cout << est;
				RemoveReadonlyAndDelete(fc.dir_name_ext());
				return;
			}
			if (!fileExists(fc.dir_name_ext() + "_original")) {
				cout << "! File not found: " + fc.dir_name_ext() << "_original\n";
				RemoveReadonlyAndDelete(fc.dir_name_ext());
				return;
			}
			RemoveReadonlyAndDelete(fc.dir_name_ext() + "_original");
		}
		RemoveReadonlyAndDelete(f.dir_name_ext());
	}
	else {
		est.Format("- Could not compress %s better (%.0lf%% from %.1lf Mb)\n",
			f.dir_name_ext(), size2 * 100.0 / size1, size1 / 1024.0 / 1024);
		cout << est;
		RemoveReadonlyAndDelete(fc.dir_name_ext());
		RemoveReadonlyAndDelete(fn.dir_name_ext());
		if (RenameFile(f, fn)) {
			est.Format("! Cannot rename file to " + fn.dir_name_ext() + "\n");
		}
		RenameExt(f, fn, ".xmp");
	}
}

void test_exclusive_log() {
	for (int i = 1; i < 1000; ++i) {
		CString est;
		est.Format("%d\n", i);
		WriteLog(est);
		this_thread::sleep_for(chrono::milliseconds(20));
	}
}

void process() {
	long long i = 0;
	CString scan_dir = dir;
	for (recursive_directory_iterator it(scan_dir.GetBuffer()), end; it != end; ++it) {
		path lpath = it->path();
		if (!is_directory(lpath)) {
			++i;
			ProcessFile(lpath);
			if (nRetCode) return;
		}
	}
	CString est;
	est.Format("+ Processed %lld files. Released %.1lf Mb\n",
		i, space_release / 1024.0 / 1024.0);
	WriteLog(est);
}

void ParseCommandLine() {
	cmd_line = ((CWinApp*)::AfxGetApp())->m_lpCmdLine;
	//cout << cmd_line << "\n";
	CString st;
	st = cmd_line;
	st.Trim();
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
	TCHAR buffer[MAX_PATH];
	GetCurrentDirectory(MAX_PATH, buffer);
	CString cur_dir = string(buffer).c_str();
	if (dir.IsEmpty()) {
		dir = cur_dir;
	}
	cout << "This application compresses all video and jpeg files in folder recursively and removes source files if compressed to smaller size\n";
	cout << "Usage: [folder]\n";
	cout << "If started without parameter, will process current folder\n";
	cout << "Program path: " << my_path << "\n";
	cout << "Program dir: " << my_dir << "\n";
	cout << "Current dir: " << cur_dir << "\n";
	cout << "Target dir: " << dir << "\n";
	ffmpeg_path = my_dir + "\\ffmpeg.exe";
	magick_path = my_dir + "\\magick.exe";
	lnkedit_path = my_dir + "\\lnkedit.exe";
	exiftool_path = my_dir + "\\exiftool.exe";
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
	CString st, st2, st3;
	ifstream fs;
	CString dir_name_ext = dir + "\\BatchCompress.pl";
	if (fileExists(dir_name_ext)) {
		WriteLog(CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S") + " Detected local config file: " + 
			dir_name_ext + "\n");
	}
	else {
		WriteLog(CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S") + " Using global config file\n");
		dir_name_ext = my_dir + "\\BatchCompress.pl";
	}
	// Check file exists
	if (!fileExists(dir_name_ext)) {
		WriteLog("LoadConfig cannot find file: " + dir_name_ext + "\n");
		nRetCode = 3;
		return;
	}
	fs.open(dir_name_ext);
	char pch[2550];
	int pos = 0;
	int i = 0;
	while (fs.good()) {
		i++;
		// Get line
		fs.getline(pch, 2550);
		st = pch;
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
			if (st3[0] == '"' && st3[st3.GetLength() - 1] == '"') {
				st3 = st3.Mid(1, st3.GetLength() - 2);
			}
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
			if (st2 == "ignore_match") {
				ignore_match[st3] = 1;
				++parameter_found;
			}
			if (st2 == "audio_ext") {
				audio_ext[st3] = 1;
				++parameter_found;
			}
			LoadVar(&st2, &st3, "ffmpeg_par_audio", &ffmpeg_par_audio);
			LoadVar(&st2, &st3, "ffmpeg_par_video", &ffmpeg_par_video);
			LoadVar(&st2, &st3, "ffmpeg_par_image", &ffmpeg_par_image);
			LoadVar(&st2, &st3, "magick_par_image", &magick_par_image);
			LoadVar(&st2, &st3, "ignore_2", &ignore_2);
			LoadVar(&st2, &st3, "process_links", &process_links);
			LoadVar(&st2, &st3, "save_exif", &save_exif);
			LoadVar(&st2, &st3, "rename_xmp", &rename_xmp);
			LoadVar(&st2, &st3, "strip_tocut", &strip_tocut);
			LoadVar(&st2, &st3, "shorten_filenames_to", &shorten_filenames_to);
			if (!parameter_found) {
				WriteLog("Unrecognized parameter '" + st2 + "' = '" + st3 + "' in file " + dir_name_ext + "\n");
			}
			if (nRetCode) break;
		}
	}
	fs.close();
	//est.Format("LoadConfig loaded %d lines from %s", i, f.dir_name_ext());
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
