/* PuLP, GLPK module */

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

/* @(#) $Jeannot: pulpGLPK.c,v 1.15 2005/05/05 09:23:51 js Exp $ */

static char const rcsid[] =
  "@(#) $Jeannot: pulpGLPK.c,v 1.15 2005/05/05 09:23:51 js Exp $";

#include <math.h>
#include "pulp_generic.h"
#include "Python.h"

#include "glpk.h"

static PyObject *pulpGLPK_solve(PyObject *self, PyObject *args);

PyObject *pulpGLPK_solve(PyObject *self, PyObject *args)
{
  PyObject *py_objective, *py_constraints;
  int sense, mip, msg, presolve;

  pulp_lp plp;
  int pulp_status;
  int i, j;
  
  /* GLPK */
  int status;
  LPX *lp;

  if (!PyArg_ParseTuple(args, "O!O!iiii",
    &PyDict_Type, &py_objective,
    &PyDict_Type, &py_constraints,
    &sense,
    &msg,
    &mip,
    &presolve)) return NULL;

  if (pulp_extract(py_objective, py_constraints, sense, &plp))
    return NULL;
  
  /* GLPK */
  
  lp = lpx_create_prob();
  if (lp == NULL)
  {
    pulp_lp_free(&plp);
    PyErr_SetString(PyExc_MemoryError,
      "Not enough memory for LP.");
    return NULL;
  }
  
  if (msg == 0)
    lpx_set_int_parm(lp, LPX_K_MSGLEV, 0);

  if (presolve)
    lpx_set_int_parm(lp, LPX_K_PRESOL, 1);
  else
    lpx_set_int_parm(lp, LPX_K_PRESOL, 0);
  
  if (mip && plp.mip)
    lpx_set_class(lp, LPX_MIP);
  
  lpx_add_rows(lp, plp.constraints_size);
  lpx_add_cols(lp, plp.variables_size);

  /* Constraints */
  for (i=0; i<plp.constraints_size; i++)
  {
    if (plp.constraints[i].sense == LpConstraintLE)
      lpx_set_row_bnds(lp, i+1, LPX_UP, 0.0, plp.constraints[i].bound);
    else if (plp.constraints[i].sense == LpConstraintEQ)
      lpx_set_row_bnds(lp, i+1, LPX_FX, plp.constraints[i].bound,
        plp.constraints[i].bound);
    else /* (plp.constraints[i].sense == LpConstraintGE) */
      lpx_set_row_bnds(lp, i+1, LPX_LO, plp.constraints[i].bound, 0.0);
  }

  /* Variables */
  for (i=0; i<plp.variables_size; i++)
  {
    if (plp.variables[i].low == -HUGE_VAL)
    {
      if (plp.variables[i].up == HUGE_VAL)
        lpx_set_col_bnds(lp, i+1, LPX_FR, 0.0, 0.0);
      else
        lpx_set_col_bnds(lp, i+1, LPX_UP, 0.0, plp.variables[i].up);
    }
    else
    {
      if (plp.variables[i].up == HUGE_VAL)
        lpx_set_col_bnds(lp, i+1, LPX_LO, plp.variables[i].low, 0.0);
      else
      {
        if (plp.variables[i].low == plp.variables[i].up)
          lpx_set_col_bnds(lp, i+1, LPX_FX, plp.variables[i].low,
            plp.variables[i].up);
        else
          lpx_set_col_bnds(lp, i+1, LPX_DB, plp.variables[i].low,
            plp.variables[i].up);
      }
    }
    if (mip && plp.variables[i].cat)
      lpx_set_col_kind(lp, i+1, LPX_IV);
      
    lpx_set_col_coef(lp, i+1, plp.variables[i].obj);
  }
  
  if (plp.sense == LpMaximize)
    lpx_set_obj_dir(lp, LPX_MAX);
  else
    lpx_set_obj_dir(lp, LPX_MIN);
  
  /* Constraints matrix */
  {
    int *rn = NULL, *cn = NULL;
    double *a = NULL;
    
    rn = malloc(sizeof(*rn) * (plp.constraints_coefs_size+1));
    cn = malloc(sizeof(*cn) * (plp.constraints_coefs_size+1));
    a = malloc(sizeof(*a) * (plp.constraints_coefs_size+1));

    if (rn == NULL || cn == NULL || a == NULL)
    {
      if (rn != NULL) free(rn);
      if (cn != NULL) free(cn);
      if (a != NULL) free(a);
      pulp_lp_free(&plp);
      lpx_delete_prob(lp);
      PyErr_SetString(PyExc_MemoryError,
        "Not enough memory for coefficient matrix.");
      return NULL;
    }
        
    for (i=0;i<plp.constraints_coefs_size;i++)
    {
      rn[i+1] = plp.constraints_coefs[i].constraint + 1;
      cn[i+1] = plp.constraints_coefs[i].variable + 1;
      a[i+1] = plp.constraints_coefs[i].coef;
    }
    
    lpx_load_mat3(lp, plp.constraints_coefs_size, rn, cn, a);

    free(rn);
    free(cn);
    free(a);
  }
  
#if 0
  lpx_write_lpt(lp, "debug.lp");
#endif

  status = lpx_simplex(lp);

  if (presolve)
  {
    if (status == LPX_E_OK)
      pulp_status = LpStatusOptimal;
    else if (status == LPX_E_NOPFS)
      pulp_status = LpStatusInfeasible;
    else if (status == LPX_E_NODFS)
      pulp_status = LpStatusUnbounded; /* Not exact */
    else
      pulp_status = LpStatusUndefined;
  }
  else
  {
    if (status == LPX_E_OK)
    {
      status = lpx_get_status(lp);
      if (status == LPX_OPT)
        pulp_status = LpStatusOptimal;
      else if (status == LPX_NOFEAS)
        pulp_status = LpStatusInfeasible;
      else if (status == LPX_UNBND)
        pulp_status = LpStatusUnbounded;
      else
        pulp_status = LpStatusUndefined;
    }
    else
      pulp_status = LpStatusUndefined;
  }

  if (pulp_status == LpStatusOptimal)
  {
    if (!mip || !plp.mip)
    {
      for (i=0;i<plp.variables_size;i++)
        lpx_get_col_info(lp, i+1, NULL, &(plp.variables[i].value), NULL);

      if (pulp_lp_inject(&plp))
      {
        pulp_lp_free(&plp);
        lpx_delete_prob(lp);
        return NULL;
      }
    }
    else
    {
      status = lpx_integer(lp);

      if (status == LPX_E_OK)
      {
        status = lpx_get_mip_stat(lp);
        if (status == LPX_I_OPT)
          pulp_status = LpStatusOptimal;
        else if (status == LPX_I_NOFEAS)
          pulp_status = LpStatusInfeasible;
        else
          pulp_status = LpStatusUndefined;

        if (pulp_status == LpStatusOptimal)
        {
          for (i=0;i<plp.variables_size;i++)
            plp.variables[i].value = lpx_get_mip_col(lp, i+1);

          if (pulp_lp_inject(&plp))
          {
            pulp_lp_free(&plp);
            lpx_delete_prob(lp);
            return NULL;
          }
        }
      }
    }
  }
  
  pulp_lp_free(&plp);
  lpx_delete_prob(lp);
  
  return PyInt_FromLong(pulp_status);
}

static PyMethodDef pulpGLPK_methods[] = {
  {"solve", pulpGLPK_solve, METH_VARARGS},
  {NULL, NULL}
};

void initpulpGLPK(void)
{
  (void) Py_InitModule("pulpGLPK", pulpGLPK_methods);
}
