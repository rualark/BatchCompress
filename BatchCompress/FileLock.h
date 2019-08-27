#pragma once
class FileLock {
public:
	~FileLock();
	int Lock(CString fname, CString st = "");
	void Unlock();
	CString &get_fname() { return m_fname;  }
private:
	CString m_fname;
	HANDLE hFile = INVALID_HANDLE_VALUE;
};

