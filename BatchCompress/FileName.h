#pragma once

#include "lib.h"

class FileName {
public:
	void FromPath(const path & pth);
	void SetDir(const CString& new_dir) {
		dir_ = new_dir;
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
	// Get
	const CString &dir() const { return dir_; }
	const CString &name() const { return name_; }
	const CString &ext() const { return ext_; }
	const CString dir_name_ext() const { return dir_name_ext_; }
	const CString &dir_name() const { return dir_name_; }
	const CString &name_ext() const { return name_ext_; }
	const vector<CString> dirs() {
		return Tokenize(dir_, "\\");
	}

private:
	void UpdateAggregates() {
		dir_name_ = dir_ + "\\" + name_;
		name_ext_ = name_ + ext_;
		dir_name_ext_ = dir_ + "\\" + name_ + ext_;
	}
	CString dir_, name_, ext_;
	CString dir_name_;
	CString name_ext_;
	CString dir_name_ext_;
};

