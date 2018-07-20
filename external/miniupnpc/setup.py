#! /usr/bin/python
# vim: tabstop=8 shiftwidth=8 expandtab
# $Id: setup.py,v 1.9 2012/05/23 08:50:10 nanard Exp $
# the MiniUPnP Project (c) 2007-2017 Thomas Bernard
# http://miniupnp.tuxfamily.org/ or http://miniupnp.free.fr/
#
# python script to build the miniupnpc module under unix
#
# Uses MAKE environment variable (defaulting to 'make')

from setuptools import setup, Extension
from setuptools.command import build_ext
import subprocess
import os

EXT = ['libminiupnpc.a']

class make_then_build_ext(build_ext.build_ext):
      def run(self):
            subprocess.check_call([os.environ.get('MAKE', 'make')] + EXT)
            build_ext.build_ext.run(self)

setup(name="miniupnpc",
      version=open('VERSION').read().strip(),
      author='Thomas BERNARD',
      author_email='miniupnp@free.fr',
      license=open('LICENSE').read(),
      url='http://miniupnp.free.fr/',
      description='miniUPnP client',
      cmdclass={'build_ext': make_then_build_ext},
      ext_modules=[
         Extension(name="miniupnpc", sources=["miniupnpcmodule.c"],
                   extra_objects=EXT)
      ])

