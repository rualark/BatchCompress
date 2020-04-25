#pragma once

int RemoveReadonlyAndDelete(CString dir_name_ext);
bool cstring_regex_match(const CString &st, const CString &pattern);
smatch cstring_regex_search(const CString& st, const CString& pattern);
bool isMatch(const string &str, const string &pattern);
uintmax_t FileSize2(CString dir_name_ext);
__int64 FileSize(CString dir_name_ext);
CString dir_from_path(CString path);
void AppendLineToFile(CString dir_name_ext, CString st);
bool dirExists(CString dirName_in);
bool fileExists(CString dirName_in);
bool fileOrDirExists(CString dirName_in);
void read_file_sv(CString dir_name_ext, vector<CString> &sv);
CString noext_from_path(CString path);
CString ext_from_path(CString path);
CString fname_from_path(CString path);
CString bname_from_path(CString path);
vector<CString> Tokenize(const CString& s, const CString& delim);
