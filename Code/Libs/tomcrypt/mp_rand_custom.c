// Copyright 2001-2019 Crytek GmbH / Crytek Group. All rights reserved.

// bn_mp_rand.c from LibTomMath 1.1.0 detects Orbis as FreeBSD by mistake and fails compilation.
// Undefine __FreeBSD__ and let it fallback on //dev/urandom.
#undef __FreeBSD__
#include "bn_mp_rand.c"
