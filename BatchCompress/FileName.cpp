// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "stdafx.h"
#include "FileName.h"
#include "lib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void FileName::FromPath(const path &pth) {
	try {
		dir_ = pth.parent_path().string().c_str();
		ext_ = pth.extension().string().c_str();
		name_ = noext_from_path(pth.filename().string().c_str());
		ext_.MakeLower();
		UpdateAggregates();
	}
	catch (...) {
		cout << "Problem filename: " << pth.u8string() << endl;
		throw;
	}
}
