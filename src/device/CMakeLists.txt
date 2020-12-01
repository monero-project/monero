# Copyright (c) 2014-2020, The Monero Project
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are
# permitted provided that the following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of
#    conditions and the following disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list
#    of conditions and the following disclaimer in the documentation and/or other
#    materials provided with the distribution.
#
# 3. Neither the name of the copyright holder nor the names of its contributors may be
#    used to endorse or promote products derived from this software without specific
#    prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
# STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
# THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


set(device_sources
  device.cpp
  device_registry.cpp
  )
  
set(device_impl_sources
  device_default.cpp
  device_registry_impl.cpp
  log.cpp
  )

if(HIDAPI_FOUND)
  set(device_impl_sources 
    ${device_impl_sources} 
    device_ledger.cpp
    device_io_hid.cpp
    )
endif()

set(device_headers
  device.hpp
  device_registry.hpp
  device_io.hpp
 )

set(device_impl_headers
  device_registry_impl.hpp
  device_default.hpp
  device_cold.hpp
  log.hpp
 )

if(HIDAPI_FOUND)
  set(device_impl_headers 
    ${device_impl_headers} 
    device_ledger.hpp
    device_io_hid.hpp
    )
endif()

set(device_private_headers)
set(device_impl_private_headers)


monero_private_headers(device_abstr
  ${device_private_headers})
monero_private_headers(device
  ${device_impl_private_headers})

monero_add_library(device_abstr
  ${device_sources}
  ${device_headers}
  ${device_private_headers})
  
monero_add_library(device
  ${device_impl_sources}
  ${device_impl_headers}
  ${device_impl_private_headers})
  
target_link_libraries(device_abstr
  PUBLIC
    ${OPENSSL_CRYPTO_LIBRARIES}
    ${Boost_SERIALIZATION_LIBRARY}
  PRIVATE
    version
    ${EXTRA_LIBRARIES})

target_link_libraries(device
  PUBLIC
    ${HIDAPI_LIBRARIES}
    device_abstr
    cncrypto
    ringct_basic
    wallet-crypto
    ${OPENSSL_CRYPTO_LIBRARIES}
    ${Boost_SERIALIZATION_LIBRARY}
  PRIVATE
    version
    ${Blocks}
    ${EXTRA_LIBRARIES})
