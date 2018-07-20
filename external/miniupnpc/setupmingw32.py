#! /usr/bin/python
# vim: tabstop=8 shiftwidth=8 expandtab
# $Id: setupmingw32.py,v 1.8 2012/05/23 08:50:10 nanard Exp $
# the MiniUPnP Project (c) 2007-2014 Thomas Bernard
# http://miniupnp.tuxfamily.org/ or http://miniupnp.free.fr/
#
# python script to build the miniupnpc module under windows (using mingw32)
#
try:
        from setuptools import setup, Extension
except ImportError:
        from distutils.core import setup, Extension
from distutils import sysconfig
sysconfig.get_config_vars()["OPT"] = ''
sysconfig.get_config_vars()["CFLAGS"] = ''
setup(name="miniupnpc",
      version=open('VERSION').read().strip(),
      author='Thomas BERNARD',
      author_email='miniupnp@free.fr',
      license=open('LICENSE').read(),
      url='http://miniupnp.free.fr/',
      description='miniUPnP client',
      ext_modules=[
         Extension(name="miniupnpc", sources=["miniupnpcmodule.c"],
                   libraries=["ws2_32", "iphlpapi"],
                   extra_objects=["libminiupnpc.a"])
      ])

