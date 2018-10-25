#include "FileUtil.h"

FILE* FileUtil::CryOpenFile(const string& filename, const char* mode)
{
	return fopen(filename.c_str(), mode);
}
