
#ifndef DATAFRAME_SRC_H
#define DATAFRAME_SRC_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* the maximum number of characters in a string label */
#ifndef MAX_LABEL_SIZE
#define MAX_LABEL_SIZE 100U
#endif /* MAX_LABEL_SIZE */

typedef struct dataframe {

	/*
	A generic data container, similar to the Pandas DataFrame, indexable on
	row number for all of the components of one data vector or by a quantity
	label for that particular component of each data vector in the sample.

	Attributes
	----------
	data : ``double *``
		The table itself, organized such that the first axis of indexing is the
		"row" number (i.e., different data vectors) and the second axis is the
		"column" number (i.e., different vector components).
	labels : ``char **``
		Descriptive labels of each of the vector components.
	n_labels : ``unsigned short``
		The number of entries in ``labels`` (i.e., the dimensionality of the
		sample).
	n_entries : ``unsigned long``
		The number of entries in ``data`` (i.e., the sample size).
	n_threads : ``unsigned short``
		The number of threads to use in accessing and subsampling the data.
	*/

	double **data;
	char **labels;
	unsigned short n_labels;
	unsigned long n_entries;
	unsigned short n_threads;

} DATAFRAME;

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
n_entires : ``unsigned long``
	The number of "rows" in the input data (i.e., the sample size).

Returns
-------
df : ``DATAFRAME *``
	A pointer to the newly instantiated dataframe object. NULL if any of the
	``labels`` have a ``strlen`` longer than ``MAX_LABEL_SIZE``.
*/
extern DATAFRAME *dataframe_initialize(double **data, char **labels,
	const unsigned short n_labels, const unsigned long n_entries,
	const unsigned short n_threads);

/*
Initialize an empty dataframe object. Automatically assigns the attributes
``data`` and ``labels`` to NULL, ``n_labels`` and ``n_entries`` to 0, and
``n_threads`` to 1.
*/
extern DATAFRAME *dataframe_empty(void);

/*
Free up the memory associated with a ``DATAFRAME`` object.
*/
extern void dataframe_free(DATAFRAME *df);

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
	const unsigned long index);

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
extern double *dataframe_get_row(DATAFRAME df, const unsigned long index);

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
	char **labels, double *new_values, unsigned short n_values);

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
extern double *dataframe_getitem_column(DATAFRAME df, const char *label);

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
	double *new_values, unsigned long length);

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
	const unsigned long n_indeces);

/*
Take a "slice" of a given dataframe, constructed by taking a range of rows.

Parameters
----------
df : ``DATAFRAME``
	The input dataframe to subsample from.
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
	unsigned long start, unsigned long stop, unsigned short step);

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
	char condition[2], double value);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DATAFRAME_SRC_H */
