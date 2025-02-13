# Copyright (c) 2018-2024, The Monero Project

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

import requests
from requests.auth import HTTPDigestAuth
import json

class Response(dict):
    def __init__(self, d):
        for k, v in d.items():
            self[k] = self._decode(v)

    @staticmethod
    def _decode(o):
        if isinstance(o, dict):
            return Response(o)
        elif isinstance(o, list):
            return [Response._decode(i) for i in o]
        else:
            return o

    def __getattr__(self, key):
        return self[key]
    def __setattr__(self, key, value):
        self[key] = value

class JSONRPC(object):
    def __init__(self, url, username=None, password=None):
        self.url = url
        self.username = username
        self.password = password

    def send_request(self, path, inputs, result_field = None):
        res = requests.post(
            self.url + path,
            data=json.dumps(inputs),
            headers={'content-type': 'application/json'},
            auth=HTTPDigestAuth(self.username, self.password) if self.username is not None else None)
        res = res.json()

        assert 'error' not in res, res

        if result_field:
            res = res[result_field]

        return Response(res)

    def send_json_rpc_request(self, inputs):
        return self.send_request("/json_rpc", inputs, 'result')



