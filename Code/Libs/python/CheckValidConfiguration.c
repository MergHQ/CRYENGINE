#include "pyconfig.h"

/* This file checks for CRYENGINE specific configuration expectations */

#if !defined(Py_ENABLE_SHARED)
#error Python must be built as a dynamic library
#endif
