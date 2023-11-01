/*
Implements the dataframe base functionality.
*/

#if defined(_OPENMP)
	#include <omp.h>
#endif /* _OPENMP */
#include <stdlib.h>
#include <string.h>
#include "dataframe.src.h"

static signed short column_index(DATAFRAME df, const char *label);
static unsigned long integer_sum(const unsigned short *input,
	const unsigned long length);

/*
Allocate memory for and return a pointer to a dataframe object.

Parameters
----------
df : ``double **``
	A 2-D array of floating point values to turn into a dataframe.
labels : ``char **``
	The labels to use for each "column" of the input data.
n_labels : ``unsigned short``
	The number of "columns" in the input data.
n_entries : ``unsigned long``
	The number of "rows" in the input data (i.e., the sample size).

Returns
-------
df : ``DATAFRAME *``
	A pointer to the newly instantiated dataframe object. NULL if any of the
	``labels`` have a ``strlen`` longer than ``MAX_LABEL_SIZE``.
*/
extern DATAFRAME *dataframe_initialize(double **data, char **labels,
	const unsigned short n_labels, const unsigned long n_entries,
	const unsigned short n_threads) {

	DATAFRAME *df = (DATAFRAME *) malloc (sizeof(DATAFRAME));
	df -> n_labels = n_labels;
	df -> n_entries = n_entries;
	df -> data = (double **) malloc (n_entries * sizeof(double *));
	df -> labels = (char **) malloc (n_labels * sizeof(char *));
	df -> n_threads = n_threads;

	#if defined(_OPENMP)
		#pragma omp parallel for num_threads(n_threads)
	#endif
	for (unsigned long i = 0ul; i < n_entries; i++) {
		df -> data[i] = (double *) malloc (n_labels * sizeof(double));
		for (unsigned short j = 0ul; j < n_labels; j++) {
			df -> data[i][j] = data[i][j];
		}
	}

	for (unsigned short i = 0u; i < n_labels; i++) {
		if (strlen(labels[i]) <= MAX_LABEL_SIZE) {
			df -> labels[i] = (char *) malloc (MAX_LABEL_SIZE * sizeof(char));
			memset(df -> labels[i], '\0', sizeof((*df).labels[i]));
			strcpy(df -> labels[i], labels[i]);
		} else {
			dataframe_free(df);
			return NULL;
		}
	}

	return df;

}


/*
Initialize an empty dataframe object. Automatically assigns the attributes
``data`` and ``labels`` to NULL, ``n_labels`` and ``n_entries`` to 0, and
``n_threads`` to 1.
*/
extern DATAFRAME *dataframe_empty(void) {

	DATAFRAME *df = (DATAFRAME *) malloc (sizeof(DATAFRAME));
	df -> data = NULL;
	df -> labels = NULL;
	df -> n_labels = 0u;
	df -> n_entries = 0ul;
	df -> n_threads = 1u;
	return df;

}


/*
Free up the memory associated with a ``DATAFRAME`` object.
*/
extern void dataframe_free(DATAFRAME *df) {

	if (df != NULL) {

		if ((*df).data != NULL) {
			for (unsigned long i = 0ul; i < (*df).n_entries; i++) {
				free(df -> data[i]);
			}
			free(df -> data);
		} else {}

		if ((*df).labels != NULL) {
			for (unsigned short i = 0u; i < (*df).n_labels; i++) {
				free(df -> labels[i]);
			}
			free(df -> labels);
		} else {}

		df -> n_labels = 0u;
		df -> n_entries = 0ul;

	} else {}

}


/*
Get a copy of a "row" from the dataframe.

Parameters
----------
df : ``DATAFRAME``
	The source dataframe itself
index : ``const unsigned long``
	The index of the row to take.

Returns
-------
copy : ``double *``
	A pointer with the corresponding data copied over. NULL if ``index`` is
	not between 0 and ``df.n_entries``.
*/
extern double *dataframe_get_row(DATAFRAME df, const unsigned long index) {

	if (index >= 0ul && index < df.n_entries) {
		double *copy = (double *) malloc (df.n_labels * sizeof(double));
		for (unsigned short i = 0u; i < df.n_labels; i++) {
			copy[i] = df.data[index][i];
		}
		return copy;
	} else {
		return NULL;
	}

}


/*
The equivalent of ``dataframe_get_row`` above, but to be called from Python.
In this case, a ``DATAFRAME`` object with the labels preserved must be returned.

Parameters
----------
input : ``DATAFRAME``
	The original dataframe, passed down from Python.
output : ``DATAFRAME *``
	The dataframe to store the output in. If ``NULL``, a new one will be
	created automatically.
index : ``const unsigned long``
	The integer index (zero-based) of the desired row number.

Returns
-------
output : ``DATAFRAME *``
	The single-row dataframe containing input.data[index] as the sole entry.
*/
extern DATAFRAME *dataframe_getitem_integer(DATAFRAME input, DATAFRAME *output,
	const unsigned long index) {

	if (output != NULL) dataframe_free(output);
	output = dataframe_empty();

	output -> data = (double **) malloc (sizeof(double *));
	output -> data[0] = dataframe_get_row(input, index);
	output -> labels = (char **) malloc (input.n_labels * sizeof(char *));
	output -> n_labels = input.n_labels;
	output -> n_entries = 1ul;
	output -> n_threads = input.n_threads;

	for (unsigned short i = 0u; i < (*output).n_labels; i++) {
		output -> labels[i] = (char *) malloc (MAX_LABEL_SIZE * sizeof(char));
		memset(output -> labels[i], '\0', MAX_LABEL_SIZE);
		strcpy(output -> labels[i], input.labels[i]);
	}

	return output;

}


/*
Assign new values to a given row of the dataframe.

Parameters
----------
df : ``DATAFRAME *``
	The dataframe itself.
index : ``unsigned long``
	The row number to modify. If equivalent to ``(*df).n_entries``, then a new
	row is added at the end of the table, and the number of entries is
	incremented by one.
labels : ``char **``
	The labels associated with the new values to be copied over.
new_values : ``double *``
	The new values themselves, matched component-wise with ``labels``.
n_values : ``unsigned short``
	The number of elements in both ``labels`` and ``new_values``.

Returns
-------
0u on success. 1u in the event that one of the column ``labels`` is not
already present in the dataframe. 2u if the index is not between 0 and
``(*df).n_entries`` (inclusive).
*/
extern unsigned short dataframe_assign_row(DATAFRAME *df, unsigned long index,
	char **labels, double *new_values, unsigned short n_values) {

	signed short *indeces = (signed short *) malloc (n_values * sizeof(
		signed short));
	for (unsigned short i = 0u; i < n_values; i++) {
		indeces[i] = column_index(*df, labels[i]);
		if (indeces[i] == -1) {
			free(indeces);
			return 1u;
		} else {}
	}

	if (index == (*df).n_entries) {
		df -> n_entries++;
		df -> data = (double **) realloc (df -> data,
			(*df).n_entries * sizeof(double *));
	} else if (index < 0 || index > (*df).n_entries) {
		free(indeces);
		return 2u;
	} else {}

	for (unsigned short i = 0u; i < n_values; i++) {
		df -> data[index][indeces[i]] = new_values[i];
	}

	return 0u;

}


/*
Get a copy of a "column" from the dataframe.

Parameters
----------
df : ``DATAFRAME``
	The source dataframe itself.
label : ``const char *``
	The label of the column to pull.

Returns
-------
copy : ``double *``
	A pointer with the corresponding data copied over. NULL if ``label`` does
	not match any of the strings in ``df.labels``.
*/
extern double *dataframe_getitem_column(DATAFRAME df, const char *label) {

	signed short index = column_index(df, label);
	if (index >= 0 && index < df.n_labels) {
		double *copy = (double *) malloc (df.n_entries * sizeof(double));
		#if defined(_OPENMP)
			#pragma omp parallel for num_threads(df.n_threads)
		#endif
		for (unsigned long i = 0ul; i < df.n_entries; i++) {
			copy[i] = df.data[i][index];
		}
		return copy;
	} else {
		return NULL;
	}

}


/*
Obtain the integer index of a "column" from the dataframe.

Parameters
----------
df : ``DATAFRAME``
	The source dataframe itself.
label : ``const char *``
	The label of the column to get the index of.

Returns
-------
index : ``signed short``
	The integer such that ``df.labels[index]`` matches the input ``label``.
	-1 if there is no match.
*/
static signed short column_index(DATAFRAME df, const char *label) {

	for (signed short i = 0; i < df.n_labels; i++) {
		if (!strcmp(df.labels[i], label)) return i;
	}
	return -1;

}


/*
Assign new values to a given column of the dataframe.

Parameters
----------
df : ``DATAFRAME *``
	The dataframe to modify.
label : ``char *``
	The string label of the column within the dataframe to modify.
new_values : ``char *``
	The new values themselves.
length : ``unsigned long``
	The number of elements in ``new_values``. If ``df`` is not an empty
	dataframe, then this value must match ``(*df).n_entries``.

Returns
-------
0u on success. 1u if the input array does not have the same entries as the
input dataframe.
*/
extern unsigned short dataframe_assign_column(DATAFRAME *df, char *label,
	double *new_values, unsigned long length) {

	if ((*df).n_entries == 0ul && (*df).n_labels == 0u) {
		df -> n_entries = length;
		df -> n_labels = 1u;
		df -> data = (double **) malloc (length * sizeof(double *));
		for (unsigned long i = 0ul; i < length; i++) {
			df -> data[i] = (double *) malloc (sizeof(double));
			df -> data[i][0] = new_values[i];
		}
		return 0u;
	} else if (length == (*df).n_entries) {
		signed short index = column_index(*df, label);

		if (index == -1) {
			index = (signed) (*df).n_labels++;
			df -> labels = (char **) realloc (df -> labels,
				(*df).n_labels * sizeof(char *));
			df -> labels[index] = (char *) malloc (MAX_LABEL_SIZE * sizeof(char));
			memset(df -> labels[index], '\0', MAX_LABEL_SIZE);
			strcpy(df -> labels[index], label);
			#if defined(_OPENMP)
				#pragma omp parallel for num_threads((*df).n_threads)
			#endif
			for (unsigned short i = 0u; i < length; i++) {
				df -> data[i] = (double *) realloc (df -> data[i],
					(*df).n_labels * sizeof(double));
			}
		} else {}

		#if defined(_OPENMP)
			#pragma omp parallel for num_threads((*df).n_threads)
		#endif
		for (unsigned short i = 0u; i < length; i++) {
			df -> data[i][index] = new_values[i];
		}
		return 0u;

		#if 0
		if (index == -1) {
			df -> n_labels++;
			df -> labels = (char **) realloc ((*df).n_labels * sizeof(char *));
			df -> labels[(*df).n_labels - 1u] = (char *) malloc (
				MAX_LABEL_SIZE * sizeof(char));
			memset(df -> labels[(*df).n_labels - 1u], '\0', MAX_LABEL_SIZE);
			strcpy(df -> labels[(*df).n_labels - 1u], label);
			for (unsigned short i = 0u; i < (*df).n_entries; i++) {
				df -> data[i] = (double *) realloc (df -> data[i],
					(*df).n_labels * sizeof(double));
				df -> data[i][(*df).n_labels - 1u] = new_values[i];
			}
		} else {
			for (unsigned short i = 0u; i < (*df).n_entries; i++) {
				df -> data[i][index] = new_values[i];
			}
		}
		#endif

	} else {
		return 1u;
	}

}


/*
Take a subsample of a given dataframe.

Parameters
----------
df : ``DATAFRAME``
	The dataframe to subsample from.
indeces : ``const unsigned long *``
	The row indeces to take from the input datafrme.
n_indeces : ``const unsigned long``
	The number of elements in ``indeces``.

Returns
-------
subsample ``DATAFRAME *``
	A new dataframe, containing only the selected rows from the input dataframe.
*/
extern DATAFRAME *dataframe_take(DATAFRAME df, const unsigned long *indeces,
	const unsigned long n_indeces) {

	static unsigned short flag = 0u;
	double **copy = (double **) malloc (n_indeces * sizeof(double *));
	#if defined(_OPENMP)
		#pragma omp parallel for num_threads(df.n_threads)
	#endif
	for (unsigned long i = 0ul; i < n_indeces; i++) {
		copy[i] = dataframe_get_row(df, indeces[i]);
		if (copy[i] == NULL) flag = 1u; /* can't return from OpenMP region */
	}

	if (flag) {
		free(copy);
		return NULL;
	} else {}

	DATAFRAME *subsample = dataframe_initialize(copy, df.labels, df.n_labels,
		n_indeces, df.n_threads);
	free(copy);
	return subsample;

}


/*
Take a "slice" of a given dataframe, constructed by taking a range of rows.

Parameters
----------
df : ``DATAFRAME``
	The input dataframe to subsample from.
output : ``DATAFRAME *``
	The dataframe to store the output in. If ``NULL``, a new one will be
	created automatically.
start : ``unsigned long``
	The starting row number of the subsample.
stop : ``unsigned long``
	The stopping row number of the subsample.
step : ``unsigned short``
	The stepsize to take in slicing from start to stop.

Returns
-------
slice : ``DATAFRAME *``
	A subsample of the input dataframe, containing the rows from ``start`` to
	``stop - 1`` (inclusive).
*/
extern DATAFRAME *dataframe_getitem_slice(DATAFRAME df, DATAFRAME *output,
	unsigned long start, unsigned long stop, unsigned short step) {

	if (!step) return NULL;
	if (-df.n_entries <= start && start < 0) start += df.n_entries;
	if (-df.n_entries <= stop && stop < 0) stop += df.n_entries;
	if (start < 0 || stop < 0 ||
		start >= df.n_entries || stop >= df.n_entries) return NULL;

	unsigned long n = 0ul, *indeces;

	if (start <= stop && step > 0) {
		if (output != NULL) dataframe_free(output);
		unsigned long idx = start;
		while (idx < stop) {
			if (n) {
				indeces = (unsigned long *) realloc (indeces,
					(n + 1ul) * sizeof(unsigned long));
			} else {
				indeces = (unsigned long *) malloc (sizeof(unsigned long));
			}
			indeces[n++] = idx;
			idx += step;
		}
		output = dataframe_take(df, indeces, n);
	} else if (start <= stop && step < 0) {
		if (output != NULL) dataframe_free(output);
		unsigned long idx = start;
		while (idx > start) {
			if (n) {
				indeces = (unsigned long *) realloc (indeces,
					(n + 1ul) * sizeof(unsigned long));
			} else {
				indeces = (unsigned long *) malloc (sizeof(unsigned long));
			}
			indeces[n++] = idx;
			idx -= step;
		}
		output = dataframe_take(df, indeces, n);
	} else if (start > stop) {
		return dataframe_getitem_slice(df, output, stop, start, -step);
	} else {
		return NULL;
	}

	return output;

}


/*
Filter the dataframe based on some condition applied to a particular column.

Parameters
----------
df : ``DATAFRAME``
	The input, unfiltered dataframe.
output : ``DATAFRAME *``
	The dataframe to store the output in. If ``NULL``, a new one will be
	created automatically.
label : ``char *``
	The string label denoting the quantity to filter the sample based on.
condition : ``char[2]``
	A two-character string denothing the condition: "<<" for less than, "<="
	for less than or equal to, "==" for exactly equal to, ">=" for greater
	than or equal to, or ">>" for greater than.
value : ``double``
	The value of compare each datum against.

Returns
-------
output : ``DATAFRAME *``
	A subsample of the input data, where each data vector satisfies the
	requirement ``df.data[row][label_index] condition value``. NULL if the
	column label is not recognized or the condition is invalid.
*/
extern DATAFRAME *dataframe_filter(DATAFRAME df, DATAFRAME *output, char *label,
	char condition[2], double value) {

	static unsigned short flag = 0u;

	signed short index = column_index(df, label);
	if (index == -1) return NULL;

	unsigned short *accept = (unsigned short *) malloc (df.n_entries *
		sizeof(unsigned short));

	unsigned short condition_checksum = (
		(unsigned short) condition[0] + (unsigned short) condition[1]
	);

	#if defined(_OPENMP)
		#pragma omp parallel for num_threads(df.n_threads)
	#endif
	for (unsigned long i = 0ul; i < df.n_entries; i++) {

		switch (condition_checksum) {

			case 120: /* "<<" -> less than but *not* equal to */
				accept[i] = df.data[i][index] < value;
				break;

			case 121: /* "<=" -> less than or equal to */
				accept[i] = df.data[i][index] <= value;
				break;

			case 122: /* "==" -> exactly equal to */
				accept[i] = df.data[i][index] == value;
				break;

			case 123: /* ">=" -> greater than or equal to */
				accept[i] = df.data[i][index] >= value;
				break;

			case 124: /* ">>" -> greater than but *not* equal to */
				accept[i] = df.data[i][index] > value;
				break;

			default:
				flag = 1u;
				break;

		}

	}

	if (flag) { /* can't return from OpenMP region above */
		free(accept);
		return NULL;
	} else {}

	unsigned long n_pass = integer_sum(accept, df.n_entries);
	unsigned long n = 0ul, *indeces = (unsigned long *) malloc (n_pass *
		sizeof(unsigned long));
	for (unsigned long i = 0ul; i < df.n_entries; i++) {
		if (accept[i]) indeces[n++] = i;
	}

	if (output != NULL) dataframe_free(output);
	output = dataframe_take(df, indeces, n_pass);
	free(accept);
	free(indeces);
	return output;

}


/*
Obtain the sum of an array of positive integers.

Parameters
----------
input : ``const unsigned long *``
	The input array itself.
length : ``const unsigned long``
	The number of entries in ``input``.

Returns
-------
sum : ``unsigned long``
	The additive sum of each element of ``input``.
*/
static unsigned long integer_sum(const unsigned short *input,
	const unsigned long length) {

	unsigned long sum = 0ul;
	for (unsigned long i = 0ul; i < length; i++) {
		sum += input[i];
	}
	return sum;

}
