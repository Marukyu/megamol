/*
 * PerformanceCounter.cpp  10.08.2006 (mueller)
 *
 * Copyright (C) 2006 by Universitaet Stuttgart (VIS). Alle Rechte vorbehalten.
 * Copyright (C) 2005 by Christoph Mueller (christoph.mueller@vis.uni-stuttgart.de). Alle Rechte vorbehalten.
 */

#ifdef _WIN32
#include <windows.h>
#else /* _WIN32 */
#include <sys/time.h>
#endif /* _WIN32 */

#include "vislib/assert.h"
#include "vislib/memutils.h"
#include "vislib/PerformanceCounter.h"
#include "vislib/SystemException.h"
#include "vislib/Trace.h"


/*
 * vislib::sys::PerformanceCounter::Query
 */
UINT64 vislib::sys::PerformanceCounter::Query(void) {
#ifdef _WIN32
	LARGE_INTEGER timerFreq, timerCount;

    if (!::QueryPerformanceFrequency(&timerFreq)) {
        TRACE(_T("QueryPerformanceFrequency failed in ")
            _T("vislib::sys::PerformanceCounter::Query\n"));
        throw SystemException(::GetLastError(), __FILE__, __LINE__);
	}
    
    if (!::QueryPerformanceCounter(&timerCount)) {
        TRACE(_T("QueryPerformanceCounter failed in ")
            _T("vislib::sys::PerformanceCounter::Query\n"));
        throw SystemException(::GetLastError(), __FILE__, __LINE__);
    }

	return (timerCount.QuadPart * 1000) / timerFreq.QuadPart;

#else /* _WIN32 */
    struct timeval t;
    ::gettimeofday(&t, NULL);
    return static_cast<UINT64>((t.tv_sec * 1e6 + t.tv_usec) / 1000.0);

#endif /* _WIN32 */
}


/*
 * vislib::sys::PerformanceCounter::operator =
 */
vislib::sys::PerformanceCounter& vislib::sys::PerformanceCounter::operator =(
        const PerformanceCounter& rhs) {
    if (this != &rhs) {
        this->mark = rhs.mark;
    }

    return *this;
}
