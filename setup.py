
from setuptools import setup, Extension

if __name__ == "__main__":
	setup(ext_modules = [Extension("src.dataframe",
		["./src/dataframe.pyx", "./src/dataframe.src.c"])])

	# extensions = []
	# for root, dirs, files in os.walk(path):
	# 	for i in files:
	# 		if i.endswith(".pyx"):
	# 			# The name of the extension
	# 			name = "%s.%s" % (root[2:].replace('/', '.'),
	# 				i.split('.')[0])
	# 			# The source files in the C library
	# 			src_files = ["%s/%s" % (root[2:], i)]
	# 			src_files += vice.find_c_extensions(name)
	# 			extensions.append(Extension(name, src_files))
	# 		else: continue
	# return extensions

