/*
 * winrc/w_inst.h - install and remove functions
 *
 * Copyright (c) 2009, NLnet Labs. All rights reserved.
 *
 * This software is open source.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 * 
 * Neither the name of the NLNET LABS nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *
 * Contains install and remove functions that manipulate the
 * windows services API and windows registry.
 */

#ifndef WINRC_W_INST_H
#define WINRC_W_INST_H

/** 
 * Install service in servicecontrolmanager, setup registry 
 * @param out: debug output printed here (errors). or NULL.
 * @param rename: if nonNULL this executable is not unbound.exe but this name.
 */
void wsvc_install(FILE* out, const char* rename);

/** 
 * Remove installed service from servicecontrolmanager, registry entries 
 * @param out: debug output printed here (errors). or NULL.
 */
void wsvc_remove(FILE* out);

/**
 * Start the service from servicecontrolmanager, tells OS to start daemon.
 * @param out: debug output printed here (errors). or NULL.
 */
void wsvc_rc_start(FILE* out);

/**
 * Stop the service from servicecontrolmanager, tells OS to stop daemon.
 * @param out: debug output printed here (errors). or NULL.
 */
void wsvc_rc_stop(FILE* out);

/** 
 * Convert windows GetLastError() value to a neat string.
 * @param str: destination buffer
 * @param len: length of dest buffer
 * @param fixed: fixed text to prepend to string.
 * @param err: the GetLastError() value.
 */
void wsvc_err2str(char* str, size_t len, const char* fixed, DWORD err);

#endif /* WINRC_W_INST_H */
