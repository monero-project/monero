#! /usr/bin/python
# $Id: setup.py,v 1.9 2012/05/23 08:50:10 nanard Exp $
# the MiniUPnP Project (c) 2007-2012 Thomas Bernard
# http://miniupnp.tuxfamily.org/ or http://miniupnp.free.fr/
#
# python script to build the miniupnpc module under unix
#
# replace libminiupnpc.a by libminiupnpc.so for shared library usage
from distutils.core import setup, Extension
from distutils import sysconfig
sysconfig.get_config_vars()["OPT"] = ''
sysconfig.get_config_vars()["CFLAGS"] = ''
setup(name="miniupnpc", version="1.7",
      ext_modules=[
	         Extension(name="miniupnpc", sources=["miniupnpcmodule.c"],
			           extra_objects=["libminiupnpc.a"])
			 ])

