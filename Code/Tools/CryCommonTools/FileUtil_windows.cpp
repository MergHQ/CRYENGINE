#include "FileUtil.h"

FILE* FileUtil::CryOpenFile(const string& filename, const char* mode)
{
	wstring widePath;
	Unicode::Convert(widePath, filename);
	return _wfopen(widePath.c_str(), L"rb");
}
