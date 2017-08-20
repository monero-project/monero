#ifndef PORTABLE_BINARY_OARCHIVE_HPP
#define PORTABLE_BINARY_OARCHIVE_HPP

// MS compatible compilers support #pragma once
#if defined(_MSC_VER)
# pragma once
#endif

#if defined(_MSC_VER)
#pragma warning( push )
#pragma warning( disable : 4244 )
#endif

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// portable_binary_oarchive.hpp

// (C) Copyright 2002 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <ostream>
#include <boost/version.hpp>
#include <boost/serialization/string.hpp>
#include <boost/archive/archive_exception.hpp>
#include <boost/archive/basic_binary_oprimitive.hpp>
#include <boost/archive/detail/common_oarchive.hpp>
#include <boost/archive/detail/register_archive.hpp>

#include <boost/archive/portable_binary_archive.hpp>
#include <boost/archive/impl/basic_binary_oprimitive.ipp>

namespace boost { namespace archive {

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// exception to be thrown if integer read from archive doesn't fit
// variable being loaded
class portable_binary_oarchive_exception : 
    public boost::archive::archive_exception
{
public:
    enum exception_code {
        invalid_flags 
    } m_exception_code ;
    portable_binary_oarchive_exception(exception_code c = invalid_flags ) :
        boost::archive::archive_exception(boost::archive::archive_exception::other_exception),
        m_exception_code(c)
    {}
    virtual const char *what( ) const throw( )
    {
        const char *msg = "programmer error";
        switch(m_exception_code){
        case invalid_flags:
            msg = "cannot be both big and little endian";
            break;
        default:
            msg = boost::archive::archive_exception::what();
            assert(false);
            break;
        }
        return msg;
    }
};

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// "Portable" output binary archive.  This is a variation of the native binary 
// archive. it addresses integer size and endienness so that binary archives can
// be passed across systems. Note:floating point types not addressed here

class portable_binary_oarchive :
    public boost::archive::basic_binary_oprimitive<
        portable_binary_oarchive,
        std::ostream::char_type, 
        std::ostream::traits_type
    >,
    public boost::archive::detail::common_oarchive<
        portable_binary_oarchive
    >
{
    typedef boost::archive::basic_binary_oprimitive<
        portable_binary_oarchive,
        std::ostream::char_type, 
        std::ostream::traits_type
    > primitive_base_t;
    typedef boost::archive::detail::common_oarchive<
        portable_binary_oarchive
    > archive_base_t;
#ifndef BOOST_NO_MEMBER_TEMPLATE_FRIENDS
public:
#else
    friend archive_base_t;
    friend primitive_base_t; // since with override save below
    friend class boost::archive::detail::interface_oarchive<
        portable_binary_oarchive
    >;
    friend class boost::archive::save_access;
protected:
#endif
    unsigned int m_flags;
    void save_impl(const boost::intmax_t l, const char maxsize);
    // add base class to the places considered when matching
    // save function to a specific set of arguments.  Note, this didn't
    // work on my MSVC 7.0 system so we use the sure-fire method below
    // using archive_base_t::save;

    // default fall through for any types not specified here
    template<class T>
    void save(const T & t){
        save_impl(t, sizeof(T));
    }
    void save(const std::string & t){
        this->primitive_base_t::save(t);
    }
    #ifndef BOOST_NO_STD_WSTRING
    void save(const std::wstring & t){
        this->primitive_base_t::save(t);
    }
    #endif
    void save(const float & t){
        this->primitive_base_t::save(t);
        // floats not supported
        //BOOST_STATIC_ASSERT(false);
    }
    void save(const double & t){
        this->primitive_base_t::save(t);
        // doubles not supported
        //BOOST_STATIC_ASSERT(false);
    }
    void save(const char & t){
        this->primitive_base_t::save(t);
    }
    void save(const unsigned char & t){
        this->primitive_base_t::save(t);
    }

    // default processing - kick back to base class.  Note the
    // extra stuff to get it passed borland compilers
    typedef boost::archive::detail::common_oarchive<portable_binary_oarchive> 
        detail_common_oarchive;
#if BOOST_VERSION > 105800
    template<class T>
    void save_override(T & t){
        this->detail_common_oarchive::save_override(t);
    }
    // explicitly convert to char * to avoid compile ambiguities
    void save_override(const boost::archive::class_name_type & t){
        const std::string s(t);
        * this << s;
    }
    // binary files don't include the optional information 
    void save_override(
        const boost::archive::class_id_optional_type & /* t */
    ){}
#else
    template<class T>
    void save_override(T & t, int){
        this->detail_common_oarchive::save_override(t, 0);
    }
    // explicitly convert to char * to avoid compile ambiguities
    void save_override(const boost::archive::class_name_type & t, int){
        const std::string s(t);
        * this << s;
    }
    // binary files don't include the optional information 
    void save_override(
        const boost::archive::class_id_optional_type & /* t */, int
    ){}
#endif

    void init(unsigned int flags);
public:
    portable_binary_oarchive(std::ostream & os, unsigned flags = 0) :
        primitive_base_t(
            * os.rdbuf(),
            0 != (flags & boost::archive::no_codecvt)
        ),
        archive_base_t(flags),
        m_flags(flags & (endian_big | endian_little))
    {
        init(flags);
    }

    portable_binary_oarchive(
        std::basic_streambuf<
            std::ostream::char_type, 
            std::ostream::traits_type
        > & bsb, 
        unsigned int flags
    ) :
        primitive_base_t(
            bsb, 
            0 != (flags & boost::archive::no_codecvt)
        ),
        archive_base_t(flags),
        m_flags(0)
    {
        init(flags);
    }
};

} }

// required by export in boost version > 1.34
#ifdef BOOST_SERIALIZATION_REGISTER_ARCHIVE
    BOOST_SERIALIZATION_REGISTER_ARCHIVE(portable_binary_oarchive)
#endif

// required by export in boost <= 1.34
#define BOOST_ARCHIVE_CUSTOM_OARCHIVE_TYPES portable_binary_oarchive

/////////1/////////2/////////3/////////4/////////5/////////6/////////7/////////8
// portable_binary_oarchive.cpp

// (C) Copyright 2002-7 Robert Ramey - http://www.rrsd.com . 
// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org for updates, documentation, and revision history.

#include <ostream>
#include <boost/detail/endian.hpp>

namespace boost { namespace archive {

inline void 
portable_binary_oarchive::save_impl(
    const boost::intmax_t l,
    const char maxsize
){
    signed char size = 0;

    if(l == 0){
        this->primitive_base_t::save(size);
        return;
    }

    boost::intmax_t ll;
    bool negative = (l < 0);
    if(negative)
        ll = -l;
    else
        ll = l;

    do{
        ll >>= CHAR_BIT;
        ++size;
    }while(ll != 0);

    this->primitive_base_t::save(
        static_cast<signed char>(negative ? -size : size)
    );

    if(negative)
        ll = -l;
    else
        ll = l;
    char * cptr = reinterpret_cast<char *>(& ll);
    #ifdef BOOST_BIG_ENDIAN
        cptr += (sizeof(boost::intmax_t) - size);
        if(m_flags & endian_little)
            reverse_bytes(size, cptr);
    #else
        if(m_flags & endian_big)
            reverse_bytes(size, cptr);
    #endif
    this->primitive_base_t::save_binary(cptr, size);
}

inline void 
portable_binary_oarchive::init(unsigned int flags) {
    if(m_flags == (endian_big | endian_little)){
        boost::serialization::throw_exception(
            portable_binary_oarchive_exception()
        );
    }
    if(0 == (flags & boost::archive::no_header)){
        // write signature in an archive version independent manner
        const std::string file_signature(
            boost::archive::BOOST_ARCHIVE_SIGNATURE()
        );
        * this << file_signature;
        // ignore archive version checking
        const boost::archive::library_version_type v{};
        /*
        // write library version
        const boost::archive::library_version_type v(
            boost::archive::BOOST_ARCHIVE_VERSION()
        );
        */
        * this << v;
    }
    save(static_cast<unsigned char>(m_flags >> CHAR_BIT));
}

} }

namespace boost {
namespace archive {

namespace detail {
    template class archive_serializer_map<portable_binary_oarchive>;
}

// template class basic_binary_oprimitive<
//     portable_binary_oarchive,
//     std::ostream::char_type, 
//     std::ostream::traits_type
// > ;

} // namespace archive
} // namespace boost

#if defined(_MSC_VER)
#pragma warning( pop )
#endif

#endif // PORTABLE_BINARY_OARCHIVE_HPP
