#pragma once
class FileLock {
public:
	~FileLock();
	int Lock(CString fname, CString st = "");
	void Unlock();
private:
	CString m_fname;
	HANDLE hFile = INVALID_HANDLE_VALUE;
};

