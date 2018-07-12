/* PuLP, C interface, generic functions */

/*
 * Copyright (c) 2004-2005, Jean-Sebastien Roy (js@jeannot.org)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/* @(#) $Jeannot: pulp_generic.h,v 1.4 2005/05/05 09:23:51 js Exp $ */

#ifndef _PULP_GENERIC_
#define _PULP_GENERIC_

#include "Python.h"

/* variable categories */
#define LpContinuous 0
#define LpInteger 1

/* objective sense */
#define LpMinimize 1
#define LpMaximize -1

/* problem status */
#define LpStatusNotSolved 0
#define LpStatusOptimal 1
#define LpStatusInfeasible -1
#define LpStatusUnbounded -2
#define LpStatusUndefined -3

/* constraint sense */
#define LpConstraintLE -1
#define LpConstraintEQ 0
#define LpConstraintGE 1

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct
{
	int constraint;
	int variable;
	double coef;
} pulp_cst_coef;

typedef struct
{
	PyObject *py_variable;
	double low, up;
	double obj;
	int cat;
	
	double value;
} pulp_var;

typedef struct
{
	PyObject *py_constraint;
	double bound;
	int sense; /* <= -1, == 0, >= 1 */
} pulp_cst;

typedef struct
{
	int variables_size, constraints_size, constraints_coefs_size;
	pulp_var *variables;
	pulp_cst *constraints;
	pulp_cst_coef *constraints_coefs;
	int mip;
	int sense; /* -1: maximize, 1: minimize */
} pulp_lp;

/*
 * Convert an LpAffineExpression py_objective, a dictionnary of LpConstraints
 * py_constraints and a sense (LpMinimize or LpMaximize) into a pulp_lp
 * structure.
 * Return 0 when no error occurs.
 */
extern int pulp_extract(PyObject *py_objective, PyObject *py_constraints,
	int sense, pulp_lp *lp);

/*
 * Use the values stored in the pulp_lp structure to update the python variables
 * varValue attributes.
 * Return 0 when no error occurs.
 */
extern int pulp_lp_inject(pulp_lp *lp);

/*
 * Free a pulp_lp structure.
 */
extern void pulp_lp_free(pulp_lp *lp);

/*
 * comparison function for column major sorting of a pulp_cst_coef array by
 * qsort
 */
extern int pulp_lp_column_major(const void *e1, const void *e2);

#if defined(__cplusplus)
}
#endif

#endif /* _PULP_GENERIC_ */
