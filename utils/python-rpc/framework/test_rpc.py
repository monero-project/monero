#!/usr/bin/env python3
import functools
import unittest

from rpc import Response


class TestResponse(unittest.TestCase):
    def test_init__empty(self):
        r = Response({})
        assert isinstance(r, Response)
        assert len(r) == 0

    def test_init__scalar_values(self):
        r = Response(dict(k='v', k2='vv'))
        assert len(r) == 2
        assert r['k'] == 'v'
        assert r['k2'] == 'vv'

    def test_init__response_value(self):
        child = Response(dict(ck='cv'))
        root = dict(k=child)
        r = Response(root)
        assert len(r) == 1
        assert isinstance(r['k'], Response)
        assert r['k'] == child
        assert r['k'] is not child

    def test_init__dict_value(self):
        child = dict(ck='cv')
        root = dict(k=child)
        r = Response(root)
        assert len(r) == 1
        assert isinstance(r['k'], Response)
        assert r['k'] == Response(child)
        assert r['k'] is not child

    def test_init__list_value(self):
        value = [1, 2]
        r = Response(dict(k=value))
        assert len(r) == 1
        assert r['k'] == value
        assert r['k'] is not value

    def test_init__list_value_with_nested_response(self):
        nested = dict(ck=[1])
        root = dict(k=[nested])
        r = Response(root)
        assert len(r) == 1
        assert r['k'] == [Response(nested)]
        assert isinstance(r['k'][0], Response)

    def test_init__list_value_with_nested_list(self):
        nested = [1, 2]
        root = dict(k=[nested])
        r = Response(root)
        assert len(r) == 1
        assert r['k'] == [nested]

    def test_getattr__present(self):
        r = Response(dict(k='v'))
        assert r.k == 'v'

    def test_getattr__missing(self):
        # This should raise an AttributeError to match the python data model.
        # However, to maintain  backwards compatibility, it raises a KeyError.
        r = Response({})
        with self.assertRaises(KeyError):
            r.k

    def test_setattr(self):
        r = Response({})
        r.k = 'v'
        assert r.k == 'v'
        assert r['k'] == 'v'

    def test_eq__identity(self):
        r = Response({})
        assert r == r

    def test_eq__empty(self):
        assert Response({}) == Response({})

    def test_eq__nonnested_matching(self):
        assert Response(dict(k='v')) == Response(dict(k='v'))

    def test_eq__nonnested_size_mismatch(self):
        assert Response(dict(k='v')) != Response(dict(k='v', k2='v'))

    def test_eq__nonnested_key_mismatch(self):
        assert Response(dict(k='v')) != Response(dict(k2='v'))

    def test_eq__nonnested_value_mismatch(self):
        assert Response(dict(k='v')) != Response(dict(k='v2'))

    def test_eq__nested(self):
        def data():
            return dict(k='v', c=dict(ck='cv'))

        assert Response(data()) == Response(data())

    def test_eq__list_nonnested(self):
        def data():
            return dict(k=[1, 2])

        assert Response(data()) == Response(data())

    def test_eq__list_nested_response(self):
        def data():
            return dict(k=[Response(dict(ck=[1]))])

        assert Response(data()) == Response(data())

    def test_eq__list_nested_list(self):
        def data():
            return dict(k=[[Response(dict(k=1))]])

        assert Response(data()) == Response(data())

    def test_eq__dict__empty(self):
        assert Response({}) == {}

    def test_eq__dict__nonnested(self):
        assert Response(dict(k='v')) == dict(k='v')

    def test_eq__dict__nested(self):
        def data():
            return dict(k='v', c=dict(ck='cv'))

        assert Response(data()) == data()


if __name__ == '__main__':
    unittest.main()
