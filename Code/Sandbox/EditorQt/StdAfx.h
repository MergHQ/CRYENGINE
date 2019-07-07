// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

#pragma once

#if _MSC_VER == 1900
// Disabling warnings related to member variables of classes that are exported with SANDBOX_API not being exported as well.
	#pragma warning(disable: 4251)
// Disabling warnings related to base classes of classes that are exported with SANDBOX_API not being exported as well.
	#pragma warning(disable: 4275)
#endif

#include <CryCore/Project/CryModuleDefs.h>
#define eCryModule eCryM_Editor
#undef  RWI_NAME_TAG
#define RWI_NAME_TAG "RayWorldIntersection(Editor)"
#undef  PWI_NAME_TAG
#define PWI_NAME_TAG "PrimitiveWorldIntersection(Editor)"
#include <CryCore/Platform/platform.h>

#ifdef USE_PCH
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QColor>
#include <QComboBox>
#include <QDir>
#include <QDrag>
#include <QFileDialog>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QMimeData>
#include <QMouseEvent>
#include <QPointer>
#include <QPushButton>
#include <QRadioButton>
#include <QSpacerItem>
#include <QStackedWidget>
#include <QStringList>
#include <QStyle>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QToolTip>
#include <QValidator>
#include <QVariant>
#include <QVBoxLayout>
#include <QVector>
#endif // USE_PCH

#define CRY_USE_XT
#define CRY_USE_ATL
#define CRY_SUPPRESS_CRYENGINE_WINDOWS_FUNCTION_RENAMING // Because we depend on having wrappers for GetObjectA/W and LoadLibraryA/W variants.
#include <CryCore/Platform/CryAtlMfc.h>

#include <CryCore/Project/ProjectDefines.h>

#ifdef USE_PCH
// Shell Extensions.
#include <Shlwapi.h>

#include "Util/BoostPythonHelpers.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cerrno>
#include <cfenv>
#include <cfloat>
#include <chrono>
#include <cinttypes>
#include <climits>
#include <clocale>
#include <cmath>
#include <codecvt>
#include <complex>
#include <condition_variable>
#include <csetjmp>
#include <csignal>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwchar>
#include <cwctype>
#include <deque>
#include <exception>
#include <forward_list>
#include <fstream>
#include <functional>
#include <future>
#include <initializer_list>
#include <iomanip>
#include <ios>
#include <iosfwd>
#include <iostream>
#include <istream>
#include <iterator>
#include <limits>
#include <list>
#include <locale>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <numeric>
#include <ostream>
#include <queue>
#include <random>
#include <ratio>
#include <regex>
#include <scoped_allocator>
#include <set>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <streambuf>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <valarray>
#include <vector>
#endif // USE_PCH

// Resource includes
#include "Resource.h"

//////////////////////////////////////////////////////////////////////////
// Main Editor include.
//////////////////////////////////////////////////////////////////////////
#include "IPlugin.h"
#include "EditorDefs.h"
#include "PluginManager.h"

// Do not remove - linker error
#include "Util/Variable.h"
