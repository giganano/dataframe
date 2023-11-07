# cython: language_level = 3, boundscheck = False

import numbers
from . cimport dataframe
from libc.stdlib cimport malloc, free
from libc.string cimport memset, strlen


cdef class _dataframe:

	def __cinit__(self, pyobj, n_threads = 1):
		cdef double **copy = dict_to_table(pyobj)
		cdef char **labels
		if copy is NULL:
			return # should've already raised an error in dict_to_table below
		else:
			keys = list(pyobj.keys())
			labels = <char **> malloc (len(pyobj.keys()) * sizeof(char *))
			for i in range(len(keys)):
				labels[i] = <char *> malloc (MAX_LABEL_SIZE * sizeof(char))
				memset(labels[i], <char> 0, MAX_LABEL_SIZE)
				for j in range(len(keys[i])):
					labels[i][j] = <char> ord(keys[i][j])
			self._df = dataframe_initialize(copy, labels,
				<unsigned short> len(keys),
				<unsigned long> len(pyobj[keys[0]]),
				<unsigned short> n_threads)
			free(copy)
			free(labels)


	def __init__(self, pyobj, n_threads = 1):
		pass


	def __dealloc__(self):
		dataframe_free(self._df)


	def __len__(self):
		return int(self._df[0].n_entries)


	def __repr__(self):
		rep = "dataframe{\n"
		for key in self.keys():
			rep += "    %s " % (key)
			for _ in range(15 - len(key)): rep += "-"
			rep += "> "
			stored_values = self.__getitem__(key)
			if self._df[0].n_entries >= 10:
				rep += "[%.2e, %.2e, %.2e, ..., %.2e, %.2e, %.2e]\n" % (
					stored_values[0], stored_values[1], stored_values[2],
					stored_values[-3], stored_values[-2], stored_values[-1])
			else:
				rep += "[%.2e" % (stored_values[0])
				for i in range(1, self._df[0].n_entries):
					rep += ", %.2e" % (stored_values[i])
				rep += "]\n"
		rep += "}"
		return rep


	def __getitem__(self, key):
		cdef double *arr
		cdef char *key_copy
		if isinstance(key, str):
			key_copy = <char *> malloc (MAX_LABEL_SIZE * sizeof(char))
			memset(key_copy, <char> 0, MAX_LABEL_SIZE)
			for i in range(len(key)): key_copy[i] = <char> ord(key[i])
			try:
				arr = dataframe_getitem_column(self._df[0], key_copy)
			finally:
				free(key_copy)
			if arr is NULL: raise KeyError(
				"Unrecognized dataframe key: \"%s\"" % (key))
			try:
				result = [float(arr[i]) for i in range(self._df[0].n_entries)]
			finally:
				free(arr)
			return result
		elif isinstance(key, numbers.Number) and key % 1 == 0:
			key = int(key)
			if -int(self._df[0].n_entries) <= key < 0:
				key += self._df[0].n_entries
			elif key < 0 or key >= self._df[0].n_entries:
				raise IndexError("""\
Integer index out of bounds for dataframe of size %d: %d""" % (
					self._df[0].n_entries, key))
			else: pass
			arr = dataframe_get_row(self._df[0], key)
			try:
				result = [float(arr[i]) for i in range(self._df[0].n_labels)]
			finally:
				free(arr)
			return dict(zip(self.keys(), result))
		elif isinstance(key, slice):
			start = key.start if key.start is not None else 0
			stop = key.stop if key.stop is not None else self._df[0].n_entries
			step = key.step if key.step is not None else 1
			rows = _dataframe({"dummy": [1]})
			rows._df = dataframe_getitem_slice(self._df[0], rows._df,
				start, stop, step)
			if rows._df is not NULL:
				return rows
			else:
				raise ValueError("Cannot slice with step-size of 0.")
		else:
			raise TypeError("Index must be of type str or int. Got: %s" % (
				type(key)))


	def __setitem__(self, key, value):
		cdef char *key_copy
		cdef double *value_copy
		cdef char **key_copies
		if isinstance(key, str):
			key_copy = <char *> malloc (MAX_LABEL_SIZE * sizeof(char))
			memset(key_copy, <char> 0, MAX_LABEL_SIZE)
			for i in range(len(key)): key_copy[i] = <char> ord(key[i])
			value_copy = <double *> malloc (len(value) * sizeof(double))
			for i in range(len(value)): value_copy[i] = value[i]
			try:
				if dataframe_assign_column(self._df, key_copy, value_copy,
					len(value)): raise ValueError("""\
Array length mismatch. Dataframe length: %d. \
Got: %d""" % (self._df[0].n_entries, len(value)))
			finally:
				free(key_copy)
				free(value_copy)
		elif isinstance(key, numbers.Number) and key % 1 == 0:
			key = int(key)
			if -int(self._df[0].n_entries) <= key < 0:
				key += self._df[0].n_entries
			elif key < 0 or key > self._df[0].n_entries:
				raise IndexError("""\
Index out of bounds for dataframe of size %d.\
Got: %d""" % (self._df[0].n_entries, key))
			else: pass
			if not isinstance(value, dict): raise TypeError("""\
Must be of type dict. Got: %s""" % (type(value)))
			value_keys = list(value.keys())
			key_copies = <char **> malloc (len(value_keys) * sizeof(char *))
			for i in range(len(value_keys)):
				key_copies[i] = <char *> malloc (MAX_LABEL_SIZE * sizeof(char))
				memset(key_copies[i], <char> 0, MAX_LABEL_SIZE)
				for j in range(len(value_keys[i])):
					key_copies[i][j] = <char> ord(value_keys[i][j])
			value_copy = <double *> malloc (len(value_keys) * sizeof(double))
			for i in range(len(value_keys)): value_copy[i] = value[value_keys[i]]
			try:
				flag = dataframe_assign_row(self._df, key, key_copies,
					value_copy, len(value_keys))
			finally:
				free(key_copies)
				free(value_copy)
			if flag == 1:
				raise ValueError("Unrecognized column label.")
			elif flag == 2:
				raise IndexError("""\
Index out of bounds for dataframe of size %d.\
Got: %d""" % (self._df[0].n_entries, key))
			else: pass
		elif isinstance(key, tuple):
			if len(key) != 2: raise ValueError("""\
Single item assignment must proceed with a key-integer pair.\
Got %d indeces.""" % (len(key)))
			integers = [isinstance(_,
				numbers.Number) and _ % 1 == 0 for _ in key]
			strings = [isinstance(_, str) for _ in key]
			if not sum(integers) == 1 and sum(strings) == 1:
				raise ValueError("""\
Single item assignment must proceed with a key-integer pair.""")
			else: pass
			if isinstance(key[0], numbers.Number):
				index = key[0]
				label = key[1]
			else:
				index = key[1]
				label = key[0]
			self.__setitem__(index, {label: value})


	@property
	def size(self):
		r"""
		Type : ``tuple``

		The number of elements stored in the dataframe. Zero'th element is the
		number of data vectors stored, and the first is the number of keys.
		"""
		return (self._df[0].n_entries, self._df[0].n_labels)


	@property
	def n_threads(self):
		r"""
		Type : ``int``

		The number of openMP threads to use. Impacts the actual number of
		threads used only if the openMP library was linked at compile time.
		"""
		return self._df[0].n_threads

	@n_threads.setter
	def n_threads(self, value):
		if isinstance(value, numbers.Number) and value % 1 == 0:
			value = int(value)
			if value <= 0: raise ValueError("""\
Number of openMP threads must be positive definite. Got: %s""" % (value))
			self._df[0].n_threads = int(value)
		else:
			raise TypeError("""\
Number of openMP threads must be an integer. Got: %s""" % (type(value)))


	def keys(self):
		r"""
		Type : ``list`` (elements of type ``str``)

		The strings which index the dataframe and would return a "column"
		containing one of the scalar components of each data vector if indexed.
		"""
		_keys = []
		for i in range(self._df[0].n_labels):
			_keys.append("".join([chr(self._df[0].labels[i][j]) for j in range(
				strlen(self._df[0].labels[i]))]))
		return _keys


	def filter(self, key, condition, value):
		r"""
		Filter the dataframe based on key-condition-value.
		"""
		cdef _dataframe result
		cdef char *key_copy = <char *> malloc (MAX_LABEL_SIZE * sizeof(char))
		cdef char condition_copy[2]
		memset(key_copy, <char> 0, MAX_LABEL_SIZE)
		for i in range(len(key)): key_copy[i] = <char> ord(key[i])
		if key == "=": key = "=="
		if key == ">": key = ">>"
		if key == "<": key = "<<"
		condition_copy[0] = <char> ord(condition[0])
		condition_copy[1] = <char> ord(condition[1])
		result = _dataframe({"dummy": [1]})
		result._df = dataframe_filter(self._df[0], result._df, key_copy,
			condition_copy, value)
		free(key_copy)
		return result


	def todict(self):
		r"""
		Pipe to a dictionary.
		"""
		copy = {}
		for key in self.keys(): copy[key] = self.__getitem__(key)
		return copy


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
			keys = list(pyobj.keys())
			n_entries = len(pyobj[keys[0]])
			n_labels = len(keys)
			copy = <double **> malloc (n_entries * sizeof(double *))
			for i in range(n_entries):
				copy[i] = <double *> malloc (n_labels * sizeof(double))
				for j in range(n_labels): copy[i][j] = pyobj[keys[j]][i]
			return copy
		else:
			raise ValueError("""\
Input data must be a dictionary containing array-like objects, each of the \
same length, containing numerical values only. """)
	else:
		return NULL
