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

#include <mutex>

#define MAX_N_LICENSES 20
#define HL_SEP '%' /* horizontal line separator */
#define COPYRIGHT_PREFIX "Copyright (c) "

static monero_license_entry_t licenses[MAX_N_LICENSES];
static size_t n_licenses;

extern const unsigned char monero_sublicenses[]; // should match generated include, minus size

static const char * find_next_line_end(const char *s, const char * const end, bool *is_hl)
{
    if (*s == '\n')
        ++s;
    const char *p = strchr(s, '\n');
    if (NULL == p) return end;
    *is_hl = p != s;
    for (; s < p; ++s)
    {
        if (*s != HL_SEP)
        {
            *is_hl = false;
            break;
        }
    }
    return p;
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
        p = next_line_end + 1;
        next_line_end = find_next_line_end(p, end, &is_hl);
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
        p = next_line_end + 1;
        next_line_end = find_next_line_end(p, end, &is_hl);
        if (next_line_end - p <= (ptrdiff_t)strlen(COPYRIGHT_PREFIX) || 
                0 != memcmp(p, COPYRIGHT_PREFIX, strlen(COPYRIGHT_PREFIX)))
            break;
        assert(!is_hl);
        p += strlen(COPYRIGHT_PREFIX);
        p_license->year = p;
        p_license->holder = strchr(p, ' ');
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

static void init_licenses_once(void)
{
    static std::once_flag once;
    std::call_once(once, init_licenses);
}

#ifdef __cplusplus
extern "C" {
#endif

size_t monero_license_num(void)
{
    init_licenses_once();
    return n_licenses;
}

monero_license_entry_t monero_license_get(const size_t idx)
{
    init_licenses_once();
    assert(idx < n_licenses);
    return licenses[idx];
}

#ifdef __cplusplus
} /* defined(MONERO_LICENSE_H) */
#endif
