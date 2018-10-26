#include "FileUtil.h"

FILE* FileUtil::CryOpenFile(const string& filename, const char* mode)
{
	wstring widePath;
	Unicode::Convert(widePath, filename);
	return _wfopen(widePath.c_str(), L"rb");
}

void FileUtil::MakeWritable(const char* filename)
{
	SetFileAttributesA(filename, FILE_ATTRIBUTE_ARCHIVE);
}

void FileUtil::MakeWritable(const wchar_t* filename)
{
	SetFileAttributesW(filename, FILE_ATTRIBUTE_ARCHIVE);
}

void FileUtil::MakeReadOnly(const char* filename)
{
	SetFileAttributesA(filename, FILE_ATTRIBUTE_READONLY);
}

