#include "pyconfig.h"

/* This file checks for CRYENGINE specific configuration expectations */

#if defined(Py_DEBUG)
#error Py_DEBUG breaks the Python ABI, Python must be built without this flag set
#endif

#if !defined(Py_ENABLE_SHARED)
#error Python must be built as a dynamic library
#endif
