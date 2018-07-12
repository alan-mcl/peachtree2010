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

/* @(#) $Jeannot: pulp_generic.c,v 1.4 2005/05/05 09:23:51 js Exp $ */

static char const rcsid[] =
  "@(#) $Jeannot: pulp_generic.c,v 1.4 2005/05/05 09:23:51 js Exp $";

#include <math.h>
#include "pulp_generic.h"
#include "mapkit_vpi.h"

static int PyObject_AsDouble(PyObject *py_obj, double *x);
static int PyObject_AsInt(PyObject *py_obj, int *x);

int PyObject_AsDouble(PyObject *py_obj, double *x)
{
	PyObject *py_float;
	
	py_float = PyNumber_Float(py_obj);
	
	if (py_float == NULL)
		return -1;
	
	*x = PyFloat_AsDouble(py_float);

	Py_DECREF(py_float);
	return 0;
}

int PyObject_AsInt(PyObject *py_obj, int *x)
{
	PyObject *py_int;
	
	py_int = PyNumber_Int(py_obj);
	
	if (py_int == NULL)
		return -1;
	
	if (!PyInt_Check(py_obj))
	{
		Py_DECREF(py_int);
		return -1;
	}
	
	*x = PyInt_AsLong(py_int);

	Py_DECREF(py_int);
	return 0;
}

int pulp_extract(PyObject *py_objective, PyObject *py_constraints, int sense,
	pulp_lp *lp)
{
	PyObject *py_variable, *py_coef, *py_cst_name, *py_lpconstraint;
	int variable;
	int posi, posj, i, j, size;
	map_vpi varindex;
	map_vpi_storage *varptr;
	PyObject *py_obj;

	if (!PyDict_Check(py_objective))
	{
		PyErr_SetString(PyExc_TypeError, "Objective must be a dictionnary.");
		return -2;
	}

	if (!PyDict_Check(py_constraints))
	{
		PyErr_SetString(PyExc_TypeError,
			"Constraints dictionnary must be a dictionnary.");
		return -2;
	}

	if (sense != -1 && sense != 1)
	{
		PyErr_SetString(PyExc_ValueError, "Sense must be -1 or 1.");
		return -2;
	}

	lp->sense = sense;

	if(map_vpi_init(&varindex))
	{
		PyErr_SetString(PyExc_MemoryError,
			"Not enough memory for the variables dictionnary.");
		return -2;
	}
	varindex.defaultvalue = -1;

	lp->constraints = NULL;
	lp->variables = NULL;
	
	/* Lire les contraintes */
	lp->constraints_size = PyDict_Size(py_constraints);
	lp->constraints = malloc(sizeof(*(lp->constraints)) * lp->constraints_size);
	if (lp->constraints == NULL)
	{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_MemoryError, "Not enough memory for constraints.");
			return -1;
	}
	
	posj = 0;
	lp->constraints_coefs_size = 0;
	while (PyDict_Next(py_constraints, &posj, &py_cst_name, &py_lpconstraint))
	{
		if (!PyDict_Check(py_lpconstraint))
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_TypeError,
				"Constraints must be a dictionnaries.");
			return -2;
		}
		lp->constraints_coefs_size += PyDict_Size(py_lpconstraint);
	}
	
#ifdef DEBUG
	printf("size= %d\n", lp->constraints_coefs_size);
#endif
	lp->constraints_coefs = malloc(sizeof(*(lp->constraints_coefs)) *
		lp->constraints_coefs_size);
	if (lp->constraints_coefs == NULL)
	{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_MemoryError,
				"Not enough memory for constraints coefficients.");
			return -1;
	}

	posj = 0;
	j = 0;
	i = 0;
	while (PyDict_Next(py_constraints, &posj, &py_cst_name, &py_lpconstraint))
	{
		double x;
		
		/* Bound */
		py_obj = PyObject_GetAttrString(py_lpconstraint, "constant");
		if (py_obj == NULL)
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_AttributeError,
				"Constraints must have a 'constant' attribute.");
			return -2;
		}
		
		if (PyObject_AsDouble(py_obj, &x))
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_TypeError,
				"Constraints 'constant' attribute must be convertible to floats.");
			return -2;
		}
		
		lp->constraints[j].bound = - x;
			
		/* Sense */
		py_obj = PyObject_GetAttrString(py_lpconstraint, "sense");
		if (py_obj == NULL)
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_AttributeError,
				"Constraints must have a sense attribute.");
			return -2;
		}
		
		if (PyObject_AsInt(py_obj, &(lp->constraints[j].sense)))
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_TypeError,
				"Constraints 'sense' attribute must be convertible to int.");
			return -2;
		}
		
		if (lp->constraints[j].sense != 0 && lp->constraints[j].sense != 1
			&& lp->constraints[j].sense != -1)
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_ValueError,
				"Constraints 'sense' attribute must be -1, 0 or 1.");
			return -2;
		}
	
#ifdef DEBUG
		printf("[%d] sense=%d bound=%g\n", j, lp->constraints[j].sense,
			lp->constraints[j].bound);
#endif
	
		posi = 0;
		while (PyDict_Next(py_lpconstraint, &posi, &py_variable, &py_coef))
		{
			if (map_vpi_get(&varindex, py_variable, &variable))
			{
				variable = varindex.used;
				if (map_vpi_set(&varindex, py_variable, variable))
				{
					map_vpi_free(&varindex);
					pulp_lp_free(lp);
					PyErr_SetString(PyExc_MemoryError,
						"Not enough memory for variables dictionnary.");
					return -1;
				}
			}
			lp->constraints_coefs[i].variable = variable;
			lp->constraints_coefs[i].constraint = j;

			if (PyObject_AsDouble(py_coef, &(lp->constraints_coefs[i].coef)))
			{
				map_vpi_free(&varindex);
				pulp_lp_free(lp);
				PyErr_SetString(PyExc_TypeError,
					"Constraints coefficients must be convertible to floats.");
				return -2;
			}
			
#ifdef DEBUG
			printf("[%d] x%d -> %g\n", lp->constraints_coefs[i].constraint, lp->constraints_coefs[i].variable, lp->constraints_coefs[i].coef);
#endif
			i ++;
		}
		j ++;
	}

	/* Lire les variables de l'objectif */
	posi = 0;
	while (PyDict_Next(py_objective, &posi, &py_variable, &py_coef))
	{
		if (map_vpi_get(&varindex, py_variable, &variable))
		{
			if (map_vpi_set(&varindex, py_variable, varindex.used))
			{
				map_vpi_free(&varindex);
				pulp_lp_free(lp);
				PyErr_SetString(PyExc_MemoryError,
					"Not enough memory for variables dictionnary.");
				return -1;
			}
		}
	}
	
	/* Extraire les informations relatives aux variables */
	lp->variables_size = varindex.used;
	lp->variables = malloc(sizeof(*(lp->variables)) * lp->variables_size);
	if (lp->variables == NULL)
	{
		map_vpi_free(&varindex);
		pulp_lp_free(lp);
		return -1;
	}
	
	lp->mip = 0;
	for (varptr = NULL; (varptr = map_vpi_nextptr(&varindex, varptr)) != NULL; )
	{
		pulp_var *pvariablep = &(lp->variables[varptr->value]);
		
		pvariablep->py_variable = varptr->key;

		/* Low bound */
		py_obj = PyObject_GetAttrString(varptr->key, "lowBound");
		if (py_obj == NULL)
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_AttributeError,
				"Variables must have a 'lowBound' attribute.");
			return -2;
		}
		else if (py_obj == Py_None)
		{
			pvariablep->low = -HUGE_VAL;
		}
		else if (PyObject_AsDouble(py_obj, &(pvariablep->low)))
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_TypeError,
				"Variables lowBound attribute must be convertible to float.");
			return -2;
		}

		/* Upper bound */
		py_obj = PyObject_GetAttrString(varptr->key, "upBound");
		if (py_obj == NULL)
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_AttributeError,
				"Variables must have an 'upBound' attribute.");
			return -2;
		}
		else if (py_obj == Py_None)
		{
			pvariablep->up = HUGE_VAL;
		}
		else if (PyObject_AsDouble(py_obj, &(pvariablep->up)))
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_TypeError,
				"Variables upBound attribute must be convertible to float.");
			return -2;
		}

		/* Category (Continuous=0, Integer=1) */
		py_obj = PyObject_GetAttrString(varptr->key, "cat");
		if (py_obj == NULL)
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_AttributeError,
				"Variables must have a 'cat' attribute.");
			return -2;
		}

		if (PyObject_AsInt(py_obj, &(pvariablep->cat)))
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_TypeError,
				"Variables cat attribute must be convertible to int.");
			return -2;
		}
					
		if (pvariablep->cat != 0 && pvariablep->cat != 1)
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_ValueError,
				"Variables cat attribute must be 0 or 1.");
			return -2;
		}
		
		lp->mip |= pvariablep->cat;
		pvariablep->obj = 0.0;

#ifdef DEBUG
		printf("x%d : %g -> %g (%d)\n", varptr->value, pvariablep->low,
			pvariablep->up, pvariablep->cat);
#endif
	}

	/* Lire l'objectif */
	posi = 0;
	while (PyDict_Next(py_objective, &posi, &py_variable, &py_coef))
	{
		i = map_vpi_value(&varindex, py_variable);

		if (PyObject_AsDouble(py_coef, &(lp->variables[i].obj)))
		{
			map_vpi_free(&varindex);
			pulp_lp_free(lp);
			PyErr_SetString(PyExc_TypeError,
				"Objective coefficients must be convertible to floats.");
			return -2;
		}

#ifdef DEBUG
		printf("obj: x%d -> %g\n", i, lp->variables[i].obj);
#endif
		i ++;
	}
	
	map_vpi_free(&varindex);
	
	return 0;
}

int pulp_lp_inject(pulp_lp *lp)
{
	int i;
	
	for (i=0;i<lp->variables_size;i++)
	{
		PyObject *py_obj = PyFloat_FromDouble(lp->variables[i].value);
		if (py_obj == NULL)
		{
			PyErr_SetString(PyExc_MemoryError, "Not enough memory for a double.");
			return -1;
		}

		if(PyObject_SetAttrString(lp->variables[i].py_variable, "varValue",
			py_obj) == -1)
		{
			PyErr_SetString(PyExc_AttributeError,
				"Cannot set attribute varValue of  a variable.");
			return -2;
		}
	}
	
	return 0;
}

void pulp_lp_free(pulp_lp *lp)
{
	free(lp->constraints);
	free(lp->constraints_coefs);
	free(lp->variables);
}

int pulp_lp_column_major(const void *e1, const void *e2)
{
	pulp_cst_coef *c1 = (pulp_cst_coef *)e1, *c2 = (pulp_cst_coef *)e2;
	int d = c1->variable - c2->variable;
	if (d) return d;
	else return c1->constraint - c2->constraint;
}
