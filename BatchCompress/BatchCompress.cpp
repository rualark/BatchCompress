// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
// BatchCompress.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BatchCompress.h"
#include "lib.h"
#include "FileName.h"
#include "FileLock.h"
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

struct MaskPar {
	MaskPar(CString m, CString p) : mask(m), par(p) {}
	CString mask;
	CString par;
};

enum class ConvertType {none, audio, video, image};

map<CString, int> audio_ext, video_ext, image_ext, remove_ext, ignore_match;
CString cmd_line, my_path, my_dir, dir, ffmpeg_path, magick_path, lnkedit_path;
int nRetCode = 0;
int run_minimized = 1;
int ignore_2 = 1;
int conv_error_noconv = 0;
CString est;
int parameter_found = 0;

// Config
vector<MaskPar> ffmpeg_par_audio;
vector<MaskPar> ffmpeg_par_video;
vector<MaskPar> magick_par_image;
int rename_xmp = 0;
int rename_srt = 0;
int strip_tocut = 1;
int shorten_filenames_to = 0;
int video_convert_only_if_size_decreases = 1;
int bcid = 0;
FileLock bcid_lock;
const int MAX_BCID = 10;
atomic<int> close_flag = 0;

long long space_before_conv_noconv = 0; // Space of converted files or renamed to noconv (before processing)
long long space_before_conv = 0; // Space of converted files (before processing)
long long space_release = 0;

// The one and only application object

CWinApp theApp;

using namespace std;

int RenameFile(FileName &f, FileName &f2) {
	// returns 0 if successfully renamed
	return rename(f.dir_name_ext(), f2.dir_name_ext());
}

void WriteLogShared(CString st) {
	cout << st;
	AppendLineToFile(dir + "\\BatchCompress.log", st);
}

void WriteLog(CString st) {
	CString st2 = CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S") + " " + st;
	//CString st3 = CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S") + " " + dir.Left(2) + " " + st;
	DWORD dwWritten; // number of bytes written to file
	HANDLE hFile;
	for (int i = 0; i < 2000; ++i) {
		hFile = CreateFile(dir + "\\BatchCompress.log", FILE_APPEND_DATA, // FILE_GENERIC_WRITE
			FILE_SHARE_READ, 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);
		if (hFile != INVALID_HANDLE_VALUE) break;
		// Sleep if cannot open file
		//cout << "Waiting for another process: " << i << endl;
		this_thread::sleep_for(chrono::milliseconds(1));
	}
	cout << st2;
	if (hFile == INVALID_HANDLE_VALUE) {
		cout << "Error writing log file\n";
		abort();
	}
	//SetFilePointer(hFile, 0, NULL, FILE_END);
	//WriteFile(hFile, buffer, sizeof(buffer), &dwWritten, 0);
	WriteFile(hFile, st2.GetBuffer(), st2.GetLength(), &dwWritten, 0);
	//SetEndOfFile(hFile);
	CloseHandle(hFile);
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

// Return 1 if file is locked successfully or 0 if not
int LockFile(FileLock &lck, FileName &f) {
	// Cannot lock non-existing file
	if (!fileExists(f.dir_name_ext())) {
		cout << "! File not found: " + f.dir_name_ext() + "\n";
		return 0;
	}
	if (lck.Lock(f.dir_name() + ".lck", bcid_lock.get_fname())) {
		// Cannot lock because file is already locked
		cout << "# Lock detected: " +
			f.dir_name_ext() + "\n";
		return 0;
	}
	// File locked successfully
	return 1;
}

// Rename file with different extension
// Returns 0 if successful or does not exist
int RenameExt(FileName &f, FileName &f2, CString ext) {
	int res = 0;
	if (fileExists(f.dir_name() + ext)) {
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

// Rename file with XMP tags (if tags are in a separate file)
// Returns 0 if successful or file does not exist
int RenameXMP(FileName &f, FileName &f2) {
	if (!rename_xmp) return 0;
	return RenameExt(f, f2, ".xmp");
}

// Rename file with SRT subtitles
// Returns 0 if successful or file does not exist
int RenameSRT(FileName &f, FileName &f2) {
	if (!rename_srt) return 0;
	return RenameExt(f, f2, ".srt");
}

void GetBCID() {
	CString bcid_st = CTime::GetCurrentTime().Format("%Y-%m-%d %H:%M:%S") + " " + dir;
	bcid = 0;
	for (int i = 1; i < MAX_BCID; ++i) {
		CString fname;
		fname.Format(dir + "\\BatchCompress.%d", i);
		if (bcid_lock.Lock(fname, bcid_st)) continue;
		bcid = i;
		break;
	}
	if (!bcid) {
		cout << "Cannot allocate BCID. Probably need to increase MAX_BCID = " << MAX_BCID << "\n";
		abort();
	}
}

void CleanBCID() {
	for (int i = 1; i < MAX_BCID; ++i) {
		CString fname;
		fname.Format(dir + "\\BatchCompress.%d", i);
		RemoveReadonlyAndDelete(fname);
	}
}

void CleanBCIDThread() {
	while (1) {
		this_thread::sleep_for(chrono::milliseconds(5000));
		CleanBCID();
	}
}

void InitBCID() {
	CleanBCID();
	GetBCID();
	thread(CleanBCIDThread).detach();
}

CString GetParByMask(CString fname, vector<MaskPar> &mp) {
	for (int i = 0; i < mp.size(); ++i) {
		if (cstring_regex_match(fname, mp[i].mask)) return mp[i].par;
		cout << "# No match of " + fname + " with " + mp[i].mask + "\n";
	}
	return "";
}

void ProcessFile(const path &path1) {
	ConvertType convert_type = ConvertType::none;
	FileLock lck;
	CString par, st, st2, st3;
	int ret;
	FileName f;
	try {
		f.FromPath(path1);
	}
	catch (...) {
		WriteLog("WARNING! Next file is unreadable probably because long file name length. Please correct.\n");
		getchar();
		return;
	}
	// Remove old lock (can exist if previous BatchCompress process was killed or if another process is running - in this case we will not be able to remove file)
	if (f.ext() == ".lck") {
		if (RemoveReadonlyAndDelete(f.dir_name_ext())) {
			cout << "* Removed old lock file: " + f.dir_name_ext() + "\n";
		}
		else {
			//cout << "* Cannot remove lock file because it is probably locked: " + f.dir_name_ext() + "\n";
		}
		return;
	}
	if (strip_tocut && f.name().Find("[cut") != -1) {
		int pos, pos2, pos3, pos4;
		FileName f2 = f;
		pos = f.name().Find(" [to cut");
		if (pos != -1 && pos < f.name().Find("[cut")) {
			pos2 = f.name().Find("]", pos);
			pos3 = f.name().Find("[", pos + 2);
			if (pos2 == -1 && pos3 > 0) pos4 = pos3 - 1;
			else if (pos2 > 0 && pos3 == -1) pos4 = pos2;
			else if (pos2 > 0 && pos3 > 0) pos4 = min(pos2, pos3 - 1);
			else pos4 = -1;
			if (pos4 != -1) {
				if (!LockFile(lck, f)) return;
				f2.SetName(f.name().Left(pos) + f.name().Mid(pos4 + 1));
				if (fileOrDirExists(f2.dir_name_ext())) {
					WriteLog("! Cannot remove [to cut] tag from file with [cut] tag because inode exists: " +
						f.dir_name_ext() + " to: " + f2.name() + "\n");
				}
				else {
					if (!RenameFile(f, f2)) {
						RenameXMP(f, f2);
						RenameSRT(f, f2);
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
		// Same process without space
		pos = f.name().Find("[to cut");
		if (pos != -1 && pos < f.name().Find("[cut")) {
			pos2 = f.name().Find("]", pos);
			pos3 = f.name().Find("[", pos + 1);
			if (pos2 == -1 && pos3 > 0) pos4 = pos3 - 1;
			else if (pos2 > 0 && pos3 == -1) pos4 = pos2;
			else if (pos2 > 0 && pos3 > 0) pos4 = min(pos2, pos3 - 1);
			else pos4 = -1;
			if (pos4 != -1) {
				if (!LockFile(lck, f)) return;
				f2.SetName(f.name().Left(pos) + f.name().Mid(pos4 + 1));
				if (fileOrDirExists(f2.dir_name_ext())) {
					WriteLog("! Cannot remove [to cut] tag from file with [cut] tag because inode exists: " +
						f.dir_name_ext() + " to: " + f2.name() + "\n");
				}
				else {
					if (!RenameFile(f, f2)) {
						RenameXMP(f, f2);
						RenameSRT(f, f2);
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
	}
	if (shorten_filenames_to && f.name().GetLength() > shorten_filenames_to) {
		if (!LockFile(lck, f)) return;
		// Save 4 characters for added non-duplication id (like "_123")
		// Save 7 characters for "-conv" or "-noconv"
		FileName f2 = f;
		int bpos = f.name().Find('[');
		int tocut = f.name().GetLength() - shorten_filenames_to + 11;
		if (bpos == -1 || bpos < shorten_filenames_to / 2 || bpos <= tocut - 5) {
			// If there is no bracket or bracket is too early
			f2.SetName(f.name().Left(shorten_filenames_to - 11));
		}
		else {
			// If there is late bracket
			f2.SetName(f.name().Left(bpos - tocut) + f.name().Mid(bpos));
		}
		if (f.name().Find("-conv") != -1 && f2.name().Find("-conv") == -1) {
			f2.SetName(f2.name() + "-conv");
		}
		if (f.name().Find("-noconv") != -1 && f2.name().Find("-noconv") == -1) {
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
			RenameXMP(f, f2);
			RenameSRT(f, f2);
			WriteLog("+ Shortened file: " + f.dir_name_ext() + " to: " + f2.name_ext() + "\n");
			f = f2;
			if (!LockFile(lck, f)) return;
		}
		else {
			WriteLog("! Cannot rename (shorten) file: " + f.dir_name_ext() + " to: " + f2.name() + "\n");
		}
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
		if (cstring_regex_match(f.name_ext(), ppr.first)) {
			cout << "- Ignore match: " << f.dir_name_ext() << "\n";
			return;
		}
	}
	if (ignore_2 && (f.ext() == ".jpg" || f.ext() == ".webp") && 
		f.name().Mid(f.name().GetLength() - 2, 1) == "_" &&
		atoi(f.name().Right(1)) > 1
		) {
		cout << "- Ignore manually processed files: " << f.dir_name_ext() << "\n";
		return;
	}
	if (!video_ext[f.ext()] && !image_ext[f.ext()] && !audio_ext[f.ext()]) {
		cout << "- Ignore ext: " << f.dir_name_ext() << "\n";
		return;
	}
	if (!fileExists(f.dir_name_ext())) {
		cout << "! File not found: " + f.dir_name_ext() + "\n";
		return;
	}
	long long size1 = FileSize(f.dir_name_ext());
	if (size1 < 100) {
		cout << "Ignore small size: " << f.dir_name_ext() << ": " << size1 << "\n";
		return;
	}
	// Run
	if (!LockFile(lck, f)) return;
	est.Format("* Process: " + f.dir_name_ext() + " (%.1lf Mb)\n",
		size1 / 1024.0 / 1024.0);
	cout << est;
	FileName fc = f;
	fc.SetName(fc.name() + "-conv");
	if (video_ext[f.ext()]) fc.SetExt(".mp4");
	if (image_ext[f.ext()]) fc.SetExt(".jpg");
	if (audio_ext[f.ext()]) fc.SetExt(".mp3");
	FileName fn = f;
	fn.SetName(fn.name() + "-noconv");
	if (fileExists(fc.dir_name_ext())) {
		cout << "! Overwriting file: " + fc.dir_name_ext() << "\n";
	}
	if (video_ext[f.ext()]) {
		convert_type = ConvertType::video;
		CString par = GetParByMask(f.name_ext(), ffmpeg_par_video);
		if (par.IsEmpty()) {
			cout << "! Will not process file because no mask match detected\n";
			return;
		}
		par.Replace("%ifname%", f.dir_name_ext());
		par.Replace("%ofname%", fc.dir_name_ext());
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (image_ext[f.ext()]) {
		convert_type = ConvertType::image;
		CString par = GetParByMask(f.name_ext(), magick_par_image);
		if (par.IsEmpty()) {
			cout << "! Will not process file because no mask match detected\n";
			return;
		}
		par.Replace("%ifname%", f.dir_name_ext());
		par.Replace("%ofname%", fc.dir_name_ext());
		ret = RunTimeout(magick_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (audio_ext[f.ext()]) {
		convert_type = ConvertType::audio;
		CString par = GetParByMask(f.name_ext(), ffmpeg_par_audio);
		if (par.IsEmpty()) {
			cout << "! Will not process file because no mask match detected\n";
			return;
		}
		par.Replace("%ifname%", f.dir_name_ext());
		par.Replace("%ofname%", fc.dir_name_ext());
		ret = RunTimeout(ffmpeg_path, par, 10 * 24 * 60 * 60 * 1000);
	}
	if (ret) {
		est.Format("! Error during running file " + f.dir_name_ext() + " conversion: %d\n", ret);
		WriteLog(est);
		if (!conv_error_noconv) return;
		space_before_conv_noconv += size1;
		RemoveReadonlyAndDelete(fc.dir_name_ext());
		RemoveReadonlyAndDelete(fn.dir_name_ext());
		if (RenameFile(f, fn)) {
			est.Format("! Cannot rename file to " + fn.dir_name_ext() + "\n");
		}
		RenameXMP(f, fn);
		RenameSRT(f, fn);
		return;
	}
	if (!fileExists(fc.dir_name_ext())) {
		est.Format("! File not found: " + f.dir_name_ext() + "\n");
		WriteLog(est);
		if (!conv_error_noconv) return;
		space_before_conv_noconv += size1;
		RemoveReadonlyAndDelete(fc.dir_name_ext());
		RemoveReadonlyAndDelete(fn.dir_name_ext());
		if (RenameFile(f, fn)) {
			est.Format("! Cannot rename file to " + fn.dir_name_ext() + "\n");
		}
		RenameXMP(f, fn);
		RenameSRT(f, fn);
		return;
	}
	long long size2 = FileSize(fc.dir_name_ext());
	if (size2 < 100) {
		est.Format("! Resulting size of file " + f.dir_name_ext() + " too small: %lld\n", size2);
		WriteLog(est);
		if (!conv_error_noconv) return;
		space_before_conv_noconv += size1;
		RemoveReadonlyAndDelete(fc.dir_name_ext());
		RemoveReadonlyAndDelete(fn.dir_name_ext());
		if (RenameFile(f, fn)) {
			est.Format("! Cannot rename file to " + fn.dir_name_ext() + "\n");
		}
		RenameXMP(f, fn);
		RenameSRT(f, fn);
		return;
	}
	if (size2 < size1 || 
		(convert_type == ConvertType::video && !video_convert_only_if_size_decreases)) {
		space_before_conv_noconv += size1;
		space_before_conv += size1;
		space_release += size1;
		space_release -= size2;
		// TF - Total free
		// CP - Converted percent = after conv / before conv
		// NCP - Nonconverted-converted percent = after conv / (before conv + before nonconv)
		est.Format("+ Compressed %s to %.0lf%% from %.1lf Mb (TF %.1lf Mb, CP %lld%%, NCP %lld%%)\n",
			f.dir_name_ext(), size2 * 100.0 / size1, size1 / 1024.0 / 1024.0,
			space_release / 1024.0 / 1024.0,
			(space_before_conv - space_release) * 100 / (space_before_conv + 1),
			(space_before_conv_noconv - space_release) * 100 / (space_before_conv_noconv + 1));
		WriteLog(est);
		RenameXMP(f, fc);
		RenameSRT(f, fc);
		RemoveReadonlyAndDelete(f.dir_name_ext());
	}
	else {
		space_before_conv_noconv += size1;
		est.Format("- Could not compress %s better (%.0lf%% from %.1lf Mb)\n",
			f.dir_name_ext(), size2 * 100.0 / size1, size1 / 1024.0 / 1024);
		cout << est;
		RemoveReadonlyAndDelete(fc.dir_name_ext());
		RemoveReadonlyAndDelete(fn.dir_name_ext());
		if (RenameFile(f, fn)) {
			est.Format("! Cannot rename file to " + fn.dir_name_ext() + "\n");
		}
		RenameXMP(f, fn);
		RenameSRT(f, fn);
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
	est.Format("+ Processed %lld files. TF %.1lf Mb, CP %lld%%, NCP %lld%%. " + scan_dir + "\n",
		i, space_release / 1024.0 / 1024.0,
		(space_before_conv - space_release) * 100 / (space_before_conv + 1),
		(space_before_conv_noconv - space_release) * 100 / (space_before_conv_noconv + 1));
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
	cout << "This application compresses audio, video and jpeg files in folder recursively and removes source files if compressed to smaller size\n";
	cout << "Usage: [folder]\n";
	cout << "If started without parameter, will process current folder\n";
	cout << "Program path: " << my_path << "\n";
	cout << "Program dir: " << my_dir << "\n";
	cout << "Current dir: " << cur_dir << "\n";
	cout << "Target dir: " << dir << "\n";
	ffmpeg_path = my_dir + "\\ffmpeg_bc.exe";
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

void LoadConfigMap(const CString &pname, const CString &pval, const CString &mname, map<CString, int> &mp) {
	if (pname == mname) {
		++parameter_found;
		if (pval == "@CLEAR") {
			mp.clear();
		}
		else {
			mp[pval] = 1;
		}
	}
}

void LoadPar(const CString &pname, const CString &pval, const CString &mname, vector<MaskPar> &mp) {
	if (pname == mname) {
		++parameter_found;
		if (pval == "@CLEAR") {
			mp.clear();
		}
		else {
			int pos = pval.Find("|");
			if (pos == -1) {
				cout << "Parameter " + pname + " should have mask and value, separated with | sign\n";
				nRetCode = 12;
				return;
			}
			CString mask = pval.Left(pos).Trim();
			CString par = pval.Mid(pos + 1).Trim();
			// Search for this mask
			for (int i = 0; i < mp.size(); ++i) {
				if (mp[i].mask == mask) {
					mp[i].par = par;
					return;
				}
			}
			mp.push_back(MaskPar(mask, par));
		}
	}
}

void LoadConfigFile(const CString &fname) {
	CString st, st2, st3;
	ifstream fs;
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
			LoadConfigMap(st2, st3, "image_ext", image_ext);
			LoadConfigMap(st2, st3, "video_ext", video_ext);
			LoadConfigMap(st2, st3, "audio_ext", audio_ext);
			LoadConfigMap(st2, st3, "remove_ext", remove_ext);
			LoadConfigMap(st2, st3, "ignore_match", ignore_match);
			LoadPar(st2, st3, "ffmpeg_par_audio", ffmpeg_par_audio);
			LoadPar(st2, st3, "ffmpeg_par_video", ffmpeg_par_video);
			LoadPar(st2, st3, "magick_par_image", magick_par_image);
			LoadVar(&st2, &st3, "ignore_2", &ignore_2);
			LoadVar(&st2, &st3, "rename_xmp", &rename_xmp);
			LoadVar(&st2, &st3, "rename_srt", &rename_srt);
			LoadVar(&st2, &st3, "strip_tocut", &strip_tocut);
			LoadVar(&st2, &st3, "video_convert_only_if_size_decreases", &video_convert_only_if_size_decreases);
			LoadVar(&st2, &st3, "shorten_filenames_to", &shorten_filenames_to);
			if (!parameter_found) {
				WriteLog("Unrecognized parameter '" + st2 + "' = '" + st3 + "' in file " + fname + "\n");
			}
			if (nRetCode) break;
		}
	}
	fs.close();
	//est.Format("LoadConfig loaded %d lines from %s", i, f.dir_name_ext());
	//WriteLog(est);
}

void LoadConfig() {
	// Always load global config file
	LoadConfigFile(my_dir + "\\BatchCompress.pl");
	// Load local config file if exists
	CString ldir = dir;
	int found = 0;
	do {
		cout << "Checking if config exists: " + ldir + "\\BatchCompress.pl\n";
		if (fileExists(ldir + "\\BatchCompress.pl")) {
			found = 1;
			WriteLog("Overriding global config with local config file: " + ldir + "\\BatchCompress.pl\n");
			LoadConfigFile(ldir + "\\BatchCompress.pl");
			break;
		}
		int pos = ldir.ReverseFind('\\');
		if (ldir.GetLength() > 3 && ldir.GetAt(1) == ':' && pos > 1) {
			ldir = ldir.Left(pos);
		}
		else break;
	} while (1);
	if (!found) {
		WriteLog("Using only global config file to process folder " + dir + "\n");
	}
}

int end_main(int ret_code = -1) {
	if (ret_code != -1) nRetCode = ret_code;
#if defined(_DEBUG)
	cout << "Press any key to continue... ";
	_getch();
#endif
	return nRetCode;
}

BOOL WINAPI ConsoleHandlerRoutine(DWORD dwCtrlType) {
	if (dwCtrlType == CTRL_CLOSE_EVENT || 
		dwCtrlType == CTRL_C_EVENT ||
		dwCtrlType == CTRL_BREAK_EVENT ||
		dwCtrlType == CTRL_LOGOFF_EVENT ||
		dwCtrlType == CTRL_SHUTDOWN_EVENT ||
		dwCtrlType == CTRL_CLOSE_EVENT) {
		close_flag.store(1);
		bcid_lock.Unlock();
		if (dwCtrlType == CTRL_CLOSE_EVENT)	WriteLog("! Program closed on CTRL_CLOSE_EVENT\n");
		if (dwCtrlType == CTRL_C_EVENT)	WriteLog("! Program closed on CTRL_C_EVENT\n");
		if (dwCtrlType == CTRL_BREAK_EVENT)	WriteLog("! Program closed on CTRL_BREAK_EVENT\n");
		// Next two events are disabled for non-service application
		if (dwCtrlType == CTRL_LOGOFF_EVENT)	WriteLog("! Program closed on CTRL_LOGOFF_EVENT\n");
		if (dwCtrlType == CTRL_SHUTDOWN_EVENT)	WriteLog("! Program closed on CTRL_SHUTDOWN_EVENT\n");
	}
	return FALSE;
}

int main() {
  HMODULE hModule = ::GetModuleHandle(nullptr);
	if (hModule == nullptr) {
		// TODO: change error code to suit your needs
		wprintf(L"Fatal Error: GetModuleHandle failed\n");
		return end_main(1);
	}
  // initialize MFC and print and error on failure
  if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0)) {
    // TODO: change error code to suit your needs
    wprintf(L"Fatal Error: MFC initialization failed\n");
		return end_main(1);
  }

	SetConsoleCtrlHandler(ConsoleHandlerRoutine, TRUE);
	Init();
	if (!nRetCode) ParseCommandLine();
	if (!nRetCode) LoadConfig();
	if (!nRetCode) InitBCID();
	if (!nRetCode) process();

	return end_main();
}
