// Copyright (c) 2012-2013 The Cryptonote developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.


#pragma once

#include "targetver.h"


#if !defined(__GNUC__) 
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif 



#include <stdio.h>


#define BOOST_FILESYSTEM_VERSION 3
#define ENABLE_RELEASE_LOGGING
#include "log_opt_defs.h"
#include "misc_log_ex.h"



