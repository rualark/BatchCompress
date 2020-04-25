// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "stdafx.h"
#include "lib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

int RemoveReadonlyAndDelete(CString dir_name_ext) {
	// Remove read-only attribute for all files, because it prevents file deletion
	SetFileAttributes(dir_name_ext,
		GetFileAttributes(dir_name_ext) & ~FILE_ATTRIBUTE_READONLY);
	// returns 0 if error deleting
	return DeleteFile(dir_name_ext);
}

bool cstring_regex_match(const CString& st, const CString& pattern) {
	regex txt_regex(pattern);
	string s(st);
	return regex_match(s, txt_regex);
}

smatch cstring_regex_search(const CString& st, const CString& pattern) {
	regex txt_regex(pattern);
	string s(st);
	smatch match;
	regex_search(s, match, txt_regex);
	return match;
}

// This probably does not work with '*' pattern
bool isMatch(const string &str, const string &pattern) {
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

vector<CString> Tokenize(const CString& s, const CString& delim) {
	vector<CString> tokens;
	CString s2 = s;
	int pos;
	int end;
	tokens.clear();

	while (!s2.IsEmpty()) {
		pos = s2.FindOneOf(delim);
		end = pos;
		if (end == -1) end = s2.GetLength();
		if (end) {
			tokens.push_back(s2.Left(end));
		}
		else {
			tokens.push_back("");
		}
		s2.Delete(0, end + 1);
		// If last token saved, but last character was delimiter, push empty token
		if (s2.IsEmpty() && pos != -1) tokens.push_back("");
	}
	return tokens;
}
