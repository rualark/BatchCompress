// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "stdafx.h"
#include "FileLock.h"
#include "lib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

FileLock::~FileLock() {
	Unlock();
}

int FileLock::Lock(CString fname, CString st) {
	// Do not relock same file
	if (fname == m_fname && hFile != INVALID_HANDLE_VALUE)
		return 0;
	// Unlock if other file is locked
	Unlock();
	m_fname = fname;
	hFile = CreateFile(m_fname, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
	if (hFile == INVALID_HANDLE_VALUE) return 1;
	if (!st.IsEmpty()) {
		DWORD dwWritten; // number of bytes written to file
		WriteFile(hFile, st.GetBuffer(), st.GetLength(), &dwWritten, 0);
	}
	return 0;
}

void FileLock::Unlock() {
	if (!m_fname.IsEmpty() && hFile != INVALID_HANDLE_VALUE) {
		CloseHandle(hFile);
		RemoveReadonlyAndDelete(m_fname);
	}
}
