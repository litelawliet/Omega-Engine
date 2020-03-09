from distutils.core import setup, Extension

core_module = Extension('_OgCore',
   sources=['OgCore_wrap.cxx'],
)
setup (name = 'OgCore',
   version = '0.1',
   author = "SWIG Docs",
   description = """Module containing all the wrapped functionalities of the core engine""",
   ext_modules = [core_module],
   py_modules = ["OgCore"],
)
