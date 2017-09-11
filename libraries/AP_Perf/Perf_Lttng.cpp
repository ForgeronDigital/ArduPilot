/*
 * Copyright (C) 2016-2017  Intel Corporation. All rights reserved.
 *
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifdef HAVE_LTTNG_UST

#define TRACEPOINT_CREATE_PROBES
#define TRACEPOINT_DEFINE

#include <string.h>

#include "Perf_Lttng_TracePoints.h"
#include "Perf_Lttng.h"

void Perf_Lttng::begin(const char *name)
{
    tracepoint(ardupilot, begin, name);
}

void Perf_Lttng::end(const char *name)
{
    tracepoint(ardupilot, end, name);
}

void Perf_Lttng::count(const char *name, uint64_t val)
{
    tracepoint(ardupilot, count, name, val);
}

#endif
