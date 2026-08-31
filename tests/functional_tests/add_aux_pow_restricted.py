#!/usr/bin/env python3
#encoding=utf-8

"""Test the restricted RPC add_aux_pow entry limit."""

from framework.daemon import Daemon


ADDRESS = '42ey1afDFnn4886T7196doS9GPMzexD9gXpsZJDwVjeRVdFCSoHnv7KPbBeGpzJBzHRCAs9UxqeoyFQMYbqSWYTfJJQAWDm'


class AddAuxPowRestrictedTest:
    def run_test(self):
        daemon = Daemon()
        restricted_daemon = Daemon(restricted_rpc=True, port=18580)
        template = daemon.getblocktemplate(ADDRESS)
        aux_pow = [{'id': '00' * 32, 'hash': '00' * 32} for _ in range(11)]

        try:
            restricted_daemon.add_aux_pow(template.blocktemplate_blob, aux_pow)
        except AssertionError as error:
            message = str(error)
            assert "'code': -19" in message, message
            assert 'Too many aux pow hashes' in message, message
        else:
            raise AssertionError('restricted add_aux_pow accepted 11 aux_pow entries')


if __name__ == '__main__':
    AddAuxPowRestrictedTest().run_test()
