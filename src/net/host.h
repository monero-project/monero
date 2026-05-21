#pragma once

#include <string>

namespace net
{
    /*!
    * \brief Canonicalize a hostname by converting letters to lowercase.
    *
    * \param host Hostname to canonicalize in-place.
    */
    inline void canonicalize_host(std::string& host) noexcept
    {
        for (char& c : host)
        {
            if ('A' <= c && c <= 'Z')
                c = static_cast<char>(c - 'A' + 'a');
        }
    }
}
