
from setuptools import setup, Extension
from subprocess import Popen, PIPE
import sys
import os

def compile_extensions():
	cwd = os.getcwd()
	os.chdir(os.path.dirname(os.path.abspath(__file__)))
	os.system("mkdir tmp")
	os.system("mv __init__.py ./tmp")
	kwargs = {}
	if "openmp" in sys.argv:
		sys.argv.remove("openmp")
		if sys.platform == "darwin":
			with Popen("brew list libomp", stdout = PIPE, stderr = PIPE,
				shell = True, text = True) as proc:
				out, err = proc.communicate()
				out = out.split('\n')
				if (any([_.endswith("omp.h") for _ in out]) and 
					any([_.endswith("libomp.dylib") for _ in out])):
					# found header and library files to link
					idx = 0
					while not out[idx].endswith("omp.h"): idx += 1
					kwargs["include_dirs"] = [os.sep.join(
						out[idx].split(os.sep)[:-1])]
					idx = 0
					while not out[idx].endswith("libomp.dylib"): idx += 1
					kwargs["library_dirs"] = [os.sep.join(
						out[idx].split(os.sep)[:-1])]
			kwargs["extra_compile_args"] = ["-Xpreprocessor", "-fopenmp"]
			kwargs["extra_link_args"] = ["-Xpreprocessor", "-fopenmp", "-lomp"]
		elif sys.platform == "linux":
			kwargs["extra_compile_args"] = ["-fopenmp"]
			kwargs["extra_link_args"] = ["-fopenmp"]
		else:
			raise OSError("Windows is not supported.")
	else:
		kwargs["extra_compile_args"] = []
		kwargs["extra_link_args"] = []
	try:
		setup(ext_modules = [Extension("src.dataframe",
			["src/dataframe.pyx", "src/dataframe.src.c"], **kwargs)])
	finally:
		os.chdir(cwd)
		os.system("mv ./tmp/__init__.py .")
		os.system("rm -rf ./tmp")

if __name__ == "__main__": compile_extensions()

