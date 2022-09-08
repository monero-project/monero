// Copyright (c) 2022, The Monero Project
//
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "./blobbable.h"
#include "../internal/container.h"
#include "../internal/endianness.h"
#include "../model/visitor.h"

namespace serde::internal
{
    template <typename BlobValue, ENABLE_TPARAM_IF(IS_BLOBBABLE(BlobValue))>
    class BlobVisitor: public model::BasicVisitor
    {
    public:

        BlobVisitor(BlobValue& val): model::BasicVisitor(), m_val_ptr(std::addressof(val)) {}

        std::string expecting() const override final
        {
            return "blob string";
        }

        void visit_bytes(const const_byte_span& blob) override final
        {
            CHECK_AND_ASSERT_THROW_MES
            (
                blob.size() == sizeof(BlobValue),
                "trying to visit blob of incorrect length"
            );

            CHECK_AND_ASSERT_THROW_MES
            (
                !m_visited,
                "blob already visited"
            );

            constexpr bool should_convert_blob = should_convert_pod<BlobValue>();
            if (should_convert_blob)
            {
                BlobValue bv;
                memcpy(&bv, blob.begin(), blob.size());
                *m_val_ptr = le_conversion<BlobValue>::convert(bv);
            }
            else
            {
                memcpy(m_val_ptr, blob.begin(), blob.size());
            }

            m_visited = true;
        }

        bool was_visited() const { return m_visited; }

    private:

        BlobValue* m_val_ptr;
        bool m_visited;
    };

    struct BlobStringVisitor: public model::RefVisitor<std::string>
    {
        BlobStringVisitor(std::string& str_ref): model::RefVisitor<std::string>(str_ref) {}

        std::string expecting() const override final
        {
            return "blob string";
        }

        void visit_bytes(const const_byte_span& blob) override final
        {
            this->visit(internal::byte_span_to_string(blob));
        }
    };

    template <typename Container, ENABLE_TPARAM_IF(IS_BLOBBABLE(typename Container::value_type))>
    struct BlobContainerVisitor: public model::RefVisitor<Container>
    {
        typedef typename Container::value_type value_type;

        BlobContainerVisitor(Container& cont_ref): model::RefVisitor<Container>(cont_ref) {}

        std::string expecting() const override final
        {
            return "container blob string";
        }

        void visit_bytes(const const_byte_span& blob) override final
        {
            constexpr size_t elem_size = sizeof(value_type);

            CHECK_AND_ASSERT_THROW_MES
            (
                blob.size() % elem_size == 0,
                "blob length " << blob.size() << " not a multiple of element size " << elem_size
            );

            Container container; // @TODO: we can speed up by using underlying reference
            const size_t num_elements = blob.size() / elem_size;
            container_reserve(container, num_elements);
            const value_type* const value_ptr = reinterpret_cast<const value_type*>(blob.begin());
            for (size_t i = 0; i < num_elements; i++)
            {
                container_put(container, CONVERT_POD(value_ptr[i]));
            }

            this->visit(std::move(container));
        }
    }; // struct BlobContainerVisitor
} // namespace serde::internal
