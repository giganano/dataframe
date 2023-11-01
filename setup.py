
from setuptools import setup, Extension
import os

def compile_extensions():
	cwd = os.getcwd()
	# os.chdir("%s/src" % (os.path.dirname(os.path.abspath(__file__))))
	os.chdir(os.path.dirname(os.path.abspath(__file__)))
	os.system("mkdir tmp")
	os.system("mv __init__.py ./tmp")
	try:
		setup(ext_modules = [Extension("src.dataframe",
			["src/dataframe.pyx", "src/dataframe.src.c"])])
	finally:
		os.chdir(cwd)
		os.system("mv ./tmp/__init__.py .")
		os.system("rm -rf ./tmp")

if __name__ == "__main__": compile_extensions()
	# setup(ext_modules = [Extension("src.dataframe",
	# 	["./src/dataframe.pyx", "./src/dataframe.src.c"])])

