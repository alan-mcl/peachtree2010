/* PuLP, CPLEX module */

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

/* @(#) $Jeannot: pulpCPLEX.c,v 1.8 2005/05/05 09:23:51 js Exp $ */

static char const rcsid[] =
  "@(#) $Jeannot: pulpCPLEX.c,v 1.8 2005/05/05 09:23:51 js Exp $";

#include <math.h>
#include "pulp_generic.h"
#include "Python.h"

#include "ilcplex/cplex.h"

static CPXENVptr genv = NULL;

static PyObject *pulpCPLEX_solve(PyObject *self, PyObject *args);
static PyObject *pulpCPLEX_grabLicence(PyObject *self, PyObject *args);
static PyObject *pulpCPLEX_releaseLicence(PyObject *self, PyObject *args);

static void setCPLEXerrorstring(CPXENVptr env, int status);

void setCPLEXerrorstring(CPXENVptr env, int status)
{
  char errorstring[1024];
  char pythonerrorstring[2048];

  if (CPXgeterrorstring (env, status, errorstring) != NULL)
  {
    sprintf(pythonerrorstring, "CPLEX error (%d): %s", status, errorstring);
    PyErr_SetString(PyExc_RuntimeError,
      pythonerrorstring);
  }
  else
  {
    PyErr_SetString(PyExc_RuntimeError,
      "Generic CPLEX error.");
  }
}

PyObject *pulpCPLEX_grabLicence(PyObject *self, PyObject *args)
{
  int status;
  
  if (!PyArg_ParseTuple(args, "")) return NULL;
  
  genv = CPXopenCPLEX (&status);
  
  return PyInt_FromLong(genv != NULL);
}

PyObject *pulpCPLEX_releaseLicence(PyObject *self, PyObject *args)
{
  if (!PyArg_ParseTuple(args, "")) return NULL;

  if (genv != NULL)
  {
    CPXcloseCPLEX (&genv);
    genv = NULL;
  }
    
  Py_INCREF(Py_None);
  return Py_None;
}

#define CHECKCPXSTATUS if (status) goto cpxerror;

PyObject *pulpCPLEX_solve(PyObject *self, PyObject *args)
{
  PyObject *py_objective, *py_constraints;
  int sense, mip, msg;
  double timeLimit;

  pulp_lp plp;
  int pulp_status;
  int i, j;
  
  /* CPLEX */
  CPXENVptr env = NULL;
  CPXLPptr lp = NULL;
  int status;
  double *obj = NULL, *lb = NULL, *ub = NULL, *rhs = NULL;
  char *ctype = NULL, *csense = NULL;
  int *rn = NULL, *cn = NULL;
  double *a = NULL, *x = NULL;

  if (!PyArg_ParseTuple(args, "O!O!iiid",
    &PyDict_Type, &py_objective,
    &PyDict_Type, &py_constraints,
    &sense,
    &msg,
    &mip,
    &timeLimit)) return NULL;

  if(pulp_extract(py_objective, py_constraints, sense, &plp))
  {
    return NULL;
  }
  
  /* CPLEX */

  if (genv != NULL)
  {
    env = genv;
    status = 0;
  }
  else
  {
    env = CPXopenCPLEX (&status);
    CHECKCPXSTATUS;
    if (env == NULL) goto cpxerror;
  }
  
  lp = CPXcreateprob (env, &status, "pulp");
  CHECKCPXSTATUS;
  if (lp == NULL) goto cpxerror;
  
  if (msg)
  {
    status = CPXsetintparam (env, CPX_PARAM_SCRIND, CPX_ON);
    CHECKCPXSTATUS;
  }

  /* Time limit */
  if (timeLimit<0) timeLimit = 1.0e75;
  status = CPXsetdblparam (env, CPX_PARAM_TILIM, timeLimit);
  CHECKCPXSTATUS;

  obj = malloc(sizeof(*obj)*plp.variables_size);
  lb = malloc(sizeof(*lb)*plp.variables_size);
  ub = malloc(sizeof(*ub)*plp.variables_size);
  ctype = malloc(sizeof(*ctype)*plp.variables_size);
  rhs = malloc(sizeof(*rhs)*plp.constraints_size);
  csense = malloc(sizeof(*csense)*plp.constraints_size);
  rn = malloc(sizeof(*rn)*(plp.constraints_coefs_size+1));
  cn = malloc(sizeof(*cn)*(plp.constraints_coefs_size+1));
  a = malloc(sizeof(*a)*(plp.constraints_coefs_size+1));
  x = malloc(sizeof(*x)*plp.variables_size);

  if (obj == NULL || lb == NULL || ub == NULL || ctype == NULL || rhs == NULL
    || csense == NULL || rn == NULL || cn == NULL || a == NULL || x == NULL)
  {
    PyErr_SetString(PyExc_MemoryError,
      "Not enough memory for problem.");
    goto error;
  }

  /* Variables */
  for (i=0; i<plp.variables_size; i++)
  {
    lb[i] = plp.variables[i].low;
    ub[i] = plp.variables[i].up;
    obj[i] = plp.variables[i].obj;
    if (plp.variables[i].cat)
      ctype[i] = 'I';
    else
      ctype[i] = 'C';
  }
  
  /* Constraints */
  for (i=0; i<plp.constraints_size; i++)
  {
    rhs[i] = plp.constraints[i].bound;
    if (plp.constraints[i].sense == LpConstraintLE)
      csense[i] = 'L';
    else if (plp.constraints[i].sense == LpConstraintEQ)
      csense[i] = 'E';
    else /* (plp.constraints[i].sense == LpConstraintGE) */
      csense[i] = 'G';
  }

  /* Constraints matrix */
  for (i=0;i<plp.constraints_coefs_size;i++)
  {
    rn[i] = plp.constraints_coefs[i].constraint;
    cn[i] = plp.constraints_coefs[i].variable;
    a[i] = plp.constraints_coefs[i].coef;
  }
  
  status = CPXnewcols(env, lp, plp.variables_size, obj, lb, ub, NULL, NULL);
  CHECKCPXSTATUS;

  if (mip && plp.mip)
  {
    status = CPXcopyctype(env, lp, ctype);
    CHECKCPXSTATUS;
  }

  status = CPXnewrows(env, lp, plp.constraints_size, rhs, csense, NULL, NULL);
  CHECKCPXSTATUS;

  status = CPXchgcoeflist(env, lp, plp.constraints_coefs_size, rn, cn, a);
  CHECKCPXSTATUS;

  free(obj);
  free(ub);
  free(lb);
  free(ctype);
  free(csense);
  free(rhs);
  free(rn);
  free(cn);
  free(a);

  if (plp.sense == LpMaximize)
    CPXchgobjsen(env, lp, CPX_MAX);
  else
    CPXchgobjsen(env, lp, CPX_MIN);
  
  pulp_status = LpStatusUndefined;

#if 0
  status = CPXwriteprob (env, lp, "debug.lp", "LP");
  CHECKCPXSTATUS;
#endif
          
  if (plp.mip && mip)
    status = CPXmipopt(env, lp);
  else
    status = CPXlpopt(env, lp);
    CHECKCPXSTATUS;

  status = CPXgetstat(env, lp);

  switch (status)
  {
    case CPX_STAT_OPTIMAL:
    case CPXMIP_OPTIMAL:
    case CPXMIP_OPTIMAL_TOL:
      pulp_status = LpStatusOptimal;
      break;
    case CPXMIP_UNBOUNDED:
    case CPX_STAT_UNBOUNDED:
      pulp_status = LpStatusUnbounded;
      break;
    case CPX_STAT_INFEASIBLE:
    case CPXMIP_INFEASIBLE:
      pulp_status = LpStatusInfeasible;
      break;
    default:
      pulp_status = LpStatusUndefined;
  }

  if (pulp_status == LpStatusOptimal)
  {
    if (plp.mip && mip)
      status = CPXgetmipx(env, lp, x, 0, plp.variables_size-1);
    else
      status = CPXsolution(env, lp, NULL, NULL, x, NULL, NULL, NULL);

    for (i=0;i<plp.variables_size;i++)
      plp.variables[i].value = x[i];

    if(pulp_lp_inject(&plp))
      goto error;
  }
  
  pulp_lp_free(&plp);
  free(x);
  CPXfreeprob (env, &lp);
  if (genv == NULL)
    CPXcloseCPLEX (&env);
  
  return PyInt_FromLong(pulp_status);

cpxerror:
  setCPLEXerrorstring(env, status);
error:
  if (lb != NULL) free(lb);
  if (ub != NULL) free(ub);
  if (obj != NULL) free(obj);
  if (ctype != NULL) free(ctype);
  if (rhs != NULL) free(rhs);
  if (csense != NULL) free(csense);
  if (rn != NULL) free(rn);
  if (cn != NULL) free(cn);
  if (a != NULL) free(a);
  if (x != NULL) free(x);
  pulp_lp_free(&plp);
  if (env != NULL && lp != NULL) CPXfreeprob (env, &lp);
  if (genv == NULL && env != NULL) CPXcloseCPLEX (&env);
  return NULL;
}

static PyMethodDef pulpCPLEX_methods[] = {
  {"solve", pulpCPLEX_solve, METH_VARARGS},
  {"grabLicence", pulpCPLEX_grabLicence, METH_VARARGS},
  {"releaseLicence", pulpCPLEX_releaseLicence, METH_VARARGS},
  {NULL, NULL}
};

void initpulpCPLEX(void)
{
  (void) Py_InitModule("pulpCPLEX", pulpCPLEX_methods);
}
