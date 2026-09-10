/* Copyright (c) 2026, The Monero Project
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are
 * permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "monero_license.h"

#include <assert.h>
#include <stdbool.h>
#include <string.h>
#include <threads.h>

#define MAX_N_LICENSES 20
#define HL_SEP '%' /* horizontal line separator */
#define COPYRIGHT_PREFIX "Copyright (c) "

extern const unsigned char monero_sublicenses[]; // should match generated include

static monero_license_entry_t licenses[MAX_N_LICENSES];
static size_t n_licenses;
static once_flag inited = ONCE_FLAG_INIT;

/**
 * @brief Get pointer to next target character `t` after p, or `end` if not applicable
 * @param t target chatcter
 * @param p -
 * @param end -
 * @param[out] is_hl true iff no character in range (p, q) not equal to HL_SEP and at least one non-target character
 * @return pointer q in [p, end] s.t. (q == end) || (*q == '\n' && q > p)
*/
static const char * find_next_c(const char t, const char *s, const char * const end, bool *is_hl)
{
    assert(s);
    assert(end);
    assert(is_hl);

    bool non_t = false;

    *is_hl = true;
    if (s < end && *s == t)
        ++s;
    while (s < end)
    {
        const char c = *s;
        assert(c != '\r'); /* UNIX newlines FTW */
        if (c == t)
            break;
        else if (c != HL_SEP)
            *is_hl = false;
        ++s;
        non_t = true;
    }
    if (!non_t)
        *is_hl = false;
    return s;
}

static const char * find_next_line_end(const char *s, const char * const end, bool *is_hl)
{
    return find_next_c('\n', s, end, is_hl);
}

static void init_licenses(void)
{
    const char * p = (const char*)monero_sublicenses;
    const size_t monero_sublicenses_len = strlen(p);
    const char * const end = p + monero_sublicenses_len;
    bool is_hl;

    while (p < end && n_licenses < MAX_N_LICENSES)
    {
        monero_license_entry_t *p_license = &licenses[n_licenses];

        /* 1. consume line of HL_SEP characters */
        const char *next_line_end = find_next_line_end(p, end, &is_hl);
        assert(is_hl);

        /* 2. consume source location */
        p = next_line_end;
        next_line_end = find_next_line_end(p, end, &is_hl);
        if (p >= next_line_end)
            break;
        ++p;
        p_license->source_location = p;
        p_license->source_location_len = next_line_end - p;
        assert(p_license->source_location);
        assert(p_license->source_location_len);
        assert(!is_hl);

        /* 3. consume empty line */
        p = next_line_end;
        next_line_end = find_next_line_end(p, end, &is_hl);
        assert(p + 1 == next_line_end);

        /* 4. consume "$COPYRIGHT_PREFIX <year> <holder>" */
        p = next_line_end;
        next_line_end = find_next_line_end(p, end, &is_hl);
        if (p >= next_line_end)
            break;
        ++p;
        if (next_line_end - p <= (ptrdiff_t)strlen(COPYRIGHT_PREFIX) || 
                0 != memcmp(p, COPYRIGHT_PREFIX, strlen(COPYRIGHT_PREFIX)))
            break;
        assert(!is_hl);
        p += strlen(COPYRIGHT_PREFIX);
        p_license->year = p;
        p_license->holder = find_next_c(' ', p, next_line_end, &is_hl);
        if (p_license->holder >= next_line_end)
            break;
        p_license->year_len = p_license->holder - p_license->year;
        ++p_license->holder;
        p_license->holder_len = next_line_end - p_license->holder;
        assert(p_license->year);
        assert(p_license->year_len);
        assert(p_license->holder);
        assert(p_license->holder_len);
        
        /* 5. consume lines of license text until we hit a line of HL_SEP or EOF */
        p = next_line_end;
        p_license->text = p;
        p_license->text_len = 0;
        while (p < end)
        {
            next_line_end = find_next_line_end(p, end, &is_hl);
            if (is_hl)
                break;
            p = next_line_end;
            p_license->text_len = p - p_license->text;
        }
        assert(p_license->text_len);

        ++n_licenses;
    }
    assert(p == end); /* otherwise need to bump MAX_N_LICENSES, or badly formatted file, or bug */
    assert(n_licenses); /* we have at least our own license */
}

size_t monero_license_num(void)
{
    call_once(&inited, init_licenses);
    return n_licenses;
}

monero_license_entry_t monero_license_get(const size_t idx)
{
    call_once(&inited, init_licenses);
    assert(idx < n_licenses);
    return licenses[idx];
}
