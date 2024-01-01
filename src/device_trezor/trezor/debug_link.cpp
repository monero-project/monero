// Copyright (c) 2017-2024, The Monero Project
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
//

#include "debug_link.hpp"

namespace hw{
namespace trezor{

  DebugLink::DebugLink(){

  }

  DebugLink::~DebugLink(){
    if (m_transport){
      close();
    }
  }

  void DebugLink::init(std::shared_ptr<Transport> & transport){
    CHECK_AND_ASSERT_THROW_MES(!m_transport, "Already initialized");
    m_transport = transport;
    m_transport->open();
  }

  void DebugLink::close(){
    CHECK_AND_ASSERT_THROW_MES(m_transport, "Not initialized");
    if (m_transport) m_transport->close();
  }

  std::shared_ptr<messages::debug::DebugLinkState> DebugLink::state(){
    return call<messages::debug::DebugLinkState>(
        messages::debug::DebugLinkGetState(),
        boost::make_optional(messages::MessageType_DebugLinkGetState));
  }

  void DebugLink::input_word(const std::string & word){
    messages::debug::DebugLinkDecision decision;
    decision.set_input(word);
    call(decision, boost::none, true);
  }

  void DebugLink::input_button(bool button){
    messages::debug::DebugLinkDecision decision;
    decision.set_button(button ? messages::debug::DebugLinkDecision_DebugButton_YES : messages::debug::DebugLinkDecision_DebugButton_NO);
    call(decision, boost::none, true);
  }

  void DebugLink::input_swipe(messages::debug::DebugLinkDecision_DebugSwipeDirection direction){
    messages::debug::DebugLinkDecision decision;
    decision.set_swipe(direction);
    call(decision, boost::none, true);
  }

  void DebugLink::stop(){
    messages::debug::DebugLinkStop msg;
    call(msg, boost::none, true);
  }





}
}