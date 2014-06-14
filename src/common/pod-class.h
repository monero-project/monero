// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

// FIXME: Why is this ifdef needed?  Hopefully making it struct won't break things.

/*
#if defined(_MSC_VER)
#define POD_CLASS struct
#else
#define POD_CLASS class
#endif
*/

#define POD_CLASS struct
