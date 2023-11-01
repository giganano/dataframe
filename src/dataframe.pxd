# cython: language_level = 3, boundscheck = False

cdef extern from "./dataframe.src.h":

	unsigned short MAX_LABEL_SIZE

	ctypedef struct DATAFRAME:
		double **data
		char **labels
		unsigned short n_labels
		unsigned long n_entries
		unsigned short n_threads

	DATAFRAME *dataframe_initialize(double **data, char **labels,
		const unsigned short n_labels, const unsigned long n_entries,
		const unsigned short n_threads)
	DATAFRAME *dataframe_empty()
	void dataframe_free(DATAFRAME *df)
	double *dataframe_getitem_column(DATAFRAME df, const char *label)
	DATAFRAME *dataframe_getitem_integer(DATAFRAME input, DATAFRAME *output,
		const unsigned long index)
	DATAFRAME *dataframe_getitem_slice(DATAFRAME input, DATAFRAME *output,
		unsigned long start, unsigned long stop, unsigned short step)
	DATAFRAME *dataframe_filter(DATAFRAME df, DATAFRAME *output, char *label,
		char condition[2], double value)


	double *dataframe_get_row(DATAFRAME df, const unsigned long index)
	DATAFRAME *dataframe_take(DATAFRAME df, const unsigned long *indeces,
		const unsigned long n_indeces)


cdef class _dataframe:
	cdef DATAFRAME *_df

cdef double **dict_to_table(pyobj) except *


