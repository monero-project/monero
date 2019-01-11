/* Round toward nearest, breaking ties away from zero.
   Copyright (C) 2007, 2010-2019 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License along
   with this program; if not, see <https://www.gnu.org/licenses/>.  */

/* Written by Ben Pfaff <blp@gnu.org>, 2007.
   Based heavily on code by Bruno Haible. */

/* Specification.  */

#include <cstdint>
#include <cfloat>
#include <limits>

/* -0.0.  See minus-zero.h.  */
#if defined __hpux || defined __sgi || defined __ICC
# define MINUS_ZERO (-DBL_MIN * DBL_MIN)
#else
# define MINUS_ZERO -0.0
#endif

/* MSVC with option -fp:strict refuses to compile constant initializers that
   contain floating-point operations.  Pacify this compiler.  */
#ifdef _MSC_VER
# pragma fenv_access (off)
#endif

static_assert(std::numeric_limits<double>::is_iec559, "We require IEEE standard compliant doubles.");

double
loki_round (double x)
{
  /* 2^(DBL_MANT_DIG-1).  */
  static const double TWO_MANT_DIG =
    /* Assume DBL_MANT_DIG <= 5 * 31.
       Use the identity
       n = floor(n/5) + floor((n+1)/5) + ... + floor((n+4)/5).  */
    (double) (1U << ((DBL_MANT_DIG - 1) / 5))
    * (double) (1U << ((DBL_MANT_DIG - 1 + 1) / 5))
    * (double) (1U << ((DBL_MANT_DIG - 1 + 2) / 5))
    * (double) (1U << ((DBL_MANT_DIG - 1 + 3) / 5))
    * (double) (1U << ((DBL_MANT_DIG - 1 + 4) / 5));

  /* The use of 'volatile' guarantees that excess precision bits are dropped at
     each addition step and before the following comparison at the caller's
     site.  It is necessary on x86 systems where double-floats are not IEEE
     compliant by default, to avoid that the results become platform and
     compiler option dependent.  'volatile' is a portable alternative to gcc's
     -ffloat-store option.  */
  volatile double y = x;
  volatile double z = y;

  if (z > 0.0)
    {
      /* Avoid rounding error for x = 0.5 - 2^(-DBL_MANT_DIG-1).  */
      if (z < 0.5)
        z = 0.0;
      /* Avoid rounding errors for values near 2^k, where k >= DBL_MANT_DIG-1.  */
      else if (z < TWO_MANT_DIG)
        {
          /* Add 0.5 to the absolute value.  */
          y = z += 0.5;
          /* Round to the next integer (nearest or up or down, doesn't
             matter).  */
          z += TWO_MANT_DIG;
          z -= TWO_MANT_DIG;
          /* Enforce rounding down.  */
          if (z > y)
            z -= 1.0;
        }
    }
  else if (z < 0.0)
    {
      /* Avoid rounding error for x = -(0.5 - 2^(-DBL_MANT_DIG-1)).  */
      if (z > - 0.5)
        z = MINUS_ZERO;
      /* Avoid rounding errors for values near -2^k, where k >= DBL_MANT_DIG-1.  */
      else if (z > -TWO_MANT_DIG)
        {
          /* Add 0.5 to the absolute value.  */
          y = z -= 0.5;
          /* Round to the next integer (nearest or up or down, doesn't
             matter).  */
          z -= TWO_MANT_DIG;
          z += TWO_MANT_DIG;
          /* Enforce rounding up.  */
          if (z < y)
            z += 1.0;
        }
    }
  return z;
}
