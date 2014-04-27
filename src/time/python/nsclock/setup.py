from distutils.core import setup, Extension

module1 = Extension('nsclock', sources = ['nsclock.c'], libraries = ['rt'])
setup(name = 'NSClock',
        version = '1.0',
        description = 'nano-second clock granularity',
        ext_modules = [module1])
