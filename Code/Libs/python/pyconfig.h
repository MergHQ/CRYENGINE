#pragma once

/* Never have _DEBUG defined during inclusion of this file */
/* Usually boost's python wrapper takes care of this */
#if defined(_DEBUG)
	#if defined(Py_BUILD_CORE)
		#undef _DEBUG
	#else
		#error Configuration error: _DEBUG was defined while including pyconfig.h, instead use: #include <boost/python.hpp>
	#endif
#endif

/* For VS2015, timemodule.c doesn't compile with default settings because of CRT changes */
/* However, CRT still has these functions with a leading underscore instead */
#if _MSC_VER >= 1900
	#include <sys/timeb.h>
	#define timezone _timezone
	#define daylight _daylight
	#define tzname _tzname
#endif

/* Otherwise just include the default options */
#include "../PC/pyconfig.h"
