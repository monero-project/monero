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
            if isinstance(v, dict):
                self[k] = Response(v)
            elif isinstance(v, list):
                self[k] = [Response(i) if isinstance(i, dict) else i for i in v]
            else:
                self[k] = v

    def __getattr__(self, key):
        try:
            return self[key]
        except KeyError:
            raise AttributeError(f"'Response' object has no attribute '{key}'")

    def __setattr__(self, key, value):
        self[key] = value

class JSONRPC:
    def __init__(self, url, username=None, password=None, timeout=20):
        self.url = url
        self.username = username
        self.password = password
        self.timeout = timeout

    def send_request(self, path, inputs, result_field=None):
        try:
            response = requests.post(
                f"{self.url}{path}",
                data=json.dumps(inputs),
                headers={'content-type': 'application/json'},
                auth=HTTPDigestAuth(self.username, self.password) if self.username else None,
                timeout=self.timeout
            )
            response.raise_for_status()
        except requests.exceptions.RequestException as e:
            raise SystemExit(f"Request failed: {e}")

        res = response.json()

        if 'error' in res:
            raise ValueError(f"Error in response: {res['error']}")

        if result_field:
            return Response(res.get(result_field, {}))

        return Response(res)

    def send_json_rpc_request(self, inputs):
        return self.send_request("/json_rpc", inputs, 'result')