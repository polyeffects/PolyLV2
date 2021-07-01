/*
  Common definitions.
  Copyright 2011 David Robillard
  Copyright 2002 Mike Rawes

  This is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef blop_common_h
#define blop_common_h

#include "math_func.h"

/* Handy constants and macros */

#ifndef SMALLEST_FLOAT
/** Smallest generated non-zero float, used for pre-empting denormals */
#define SMALLEST_FLOAT (1.0 / (float)0xFFFFFFFF)
#endif

/*
 * Clip without branch (from http://musicdsp.org)
 */

static inline float
f_min (float x, float a)
{
	return a - (a - x + FABSF (a - x)) * 0.5f;
}

static inline float
f_max (float x, float b)
{
	return (x - b + FABSF (x - b)) * 0.5f + b;
}

static inline float
f_clip (float x, float a, float b)
{
	return 0.5f * (FABSF (x - a) + a + b - FABSF (x - b));
}

#endif /* blop_common_h */
