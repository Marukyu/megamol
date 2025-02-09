/*
 * stdafx.h
 *
 * Copyright (C) 2006 - 2008 by Universitaet Stuttgart (VIS).
 * Alle Rechte vorbehalten.
 */

#ifndef MEGAMOLVIEWER_STDAFX_H_INCLUDED
#define MEGAMOLVIEWER_STDAFX_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */

#ifdef _WIN32
/* Windows includes */

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#undef min
#undef max

#else /* _WIN32 */
/* Linux includes */

#include <memory.h>

#ifndef NULL
#define NULL 0
#endif

#endif /* _WIN32 */

#include "vislib/types.h"

#endif /* MEGAMOLVIEWER_STDAFX_H_INCLUDED */
