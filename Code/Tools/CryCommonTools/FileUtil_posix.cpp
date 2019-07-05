#include "FileUtil.h"

FILE* FileUtil::CryOpenFile(const string& filename, const char* mode)
{
	return fopen(filename.c_str(), mode);
}

void FileUtil::MakeWritable(const char* filename)
{
	chmod(filename, S_IRWXU);
}

void FileUtil::MakeWritable(const wchar_t* filename)
{
	string narrow_filename;
	Unicode::Convert(narrow_filename, filename);
	chmod(narrow_filename, S_IRWXU);
}

void FileUtil::MakeReadOnly(const char* filename)
{
	chmod(filename, S_IRUSR | S_IXUSR);
}
