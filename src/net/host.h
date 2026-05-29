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
            if (u8'A' <= c && c <= u8'Z')
                c = static_cast<char>(c - u8'A' + u8'a');
        }
    }
}
