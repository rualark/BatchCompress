#pragma once

class FileName {
public:
	void FromPath(const path & pth);
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

