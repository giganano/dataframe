# cython: language_level = 3, boundscheck = False

import numbers
from . cimport dataframe
from libc.stdlib cimport malloc, free
from libc.string cimport memset, strcpy


cdef class _dataframe:

	def __cinit__(self, pyobj, n_threads = 1):
		cdef double **copy = dict_to_table(pyobj)
		cdef char **labels
		if copy is NULL:
			return
		else:
			keys = pyobj.keys()
			labels = <char **> malloc (len(pyobj.keys()) * sizeof(char *))
			for i in range(len(keys)):
				labels[i] = <char *> malloc (MAX_LABEL_SIZE * sizeof(char))
				memset(labels[i], <char> 0, MAX_LABEL_SIZE)
				for j in range(len(keys[i])):
					labels[i][j] = <char> ord(keys[i][j])
			self._df = dataframe.dataframe_initialize(copy, labels,
				<unsigned short> len(keys),
				<unsigned long> len(pyobj[keys[0]]),
				<unsigned short> n_threads)
			free(copy)
			free(labels)

	def __init__(self, pyobj, n_threads = 1):
		pass

	def __dealloc__(self):
		dataframe.dataframe_free(self._df)



cdef double **dict_to_table(pyobj) except *:
	cdef double **copy
	if pyobj is not None:
		tabular_data = isinstance(pyobj, dict)
		tabular_data &= all([isinstance(_, str) for _ in pyobj.keys()])
		for key in pyobj.keys():
			tabular_data &= hasattr(pyobj[key], "__getitem__")
			tabular_data &= hasattr(pyobj[key], "__len__")
			tabular_data &= all([isinstance(_,
				numbers.Number) for _ in pyobj[key]])
		tabular_data &= len(set([len(pyobj[_]) for _ in pyobj.keys()])) == 1
		if tabular_data:
			keys = pyobj.keys()
			n_entries = len(pyobj[keys[0]])
			n_labels = len(keys)
			copy = <double **> malloc (n_entries * sizeof(double *))
			for i in range(n_entries):
				copy[i] = <double *> malloc (n_labels * sizeof(double))
				for j in range(n_labels): copy[i][j] = pyobj[keys[j]][i]
		else:
			raise ValueError("""\
Input data must be a dictionary containing array-like objects, each of the \
same length, containing numerical values only. """)
	else:
		return NULL



