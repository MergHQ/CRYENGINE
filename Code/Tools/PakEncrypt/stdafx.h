// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#define EAAS_KEY_FILE "key.dat"
#define EAAS_H_FILE "key.h"
#define EAAS_PUB_SIZE 140
#define EAAS_PRIV_SIZE 610

#if WIN32
	#include "targetver.h"
	#include <tchar.h>
#else
	#include <unistd.h>

	// Since MS uses wchar_t if you #define UNICODE (which we do), we need
	// to bodge these back to using regular characters + char-based functions.
	#define _trename  rename
	#define TCHAR     char
	#define _TCHAR    char
	#define _T(str)   str
	#define _tunlink  unlink
	#define _tcslen   strlen
	#define _tcscmp   strcmp
	#define _tmain    main
	#define _ftprintf fprintf
	#define _tprintf  printf

	#define _tfopen_s(pFile,filename,mode)               ((*(pFile))=fopen((filename),(mode)))==NULL

	// Even though we could use memcpy here (as in context, the memory regions
	// won't overlap, play it safe in case changes are made layer.
	#define wcstombs_s(_, dst, dst_len, src, src_len)    (void)_; memmove(dst, src, src_len)

	#define _get_errno(ptr)                (*(ptr) = errno)
#endif

#include <stdio.h>



// TODO: reference additional headers your program requires here
