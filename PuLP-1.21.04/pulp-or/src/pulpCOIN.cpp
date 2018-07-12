/* PuLP, COIN module */

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

/* @(#) $Jeannot: pulpCOIN.cpp,v 1.15 2005/05/05 09:23:51 js Exp $ */

static char const rcsid[] =
  "@(#) $Jeannot: pulpCOIN.cpp,v 1.15 2005/05/05 09:23:51 js Exp $";

#include <Python.h>
#include <math.h>
#include <float.h>
#include "pulp_generic.h"

#include "OsiClpSolverInterface.hpp"
#include "CbcModel.hpp"

#include "CglCutGenerator.hpp"
#include "CglGomory.hpp"
#include "CglProbing.hpp"
#include "CglKnapsackCover.hpp"
#include "CglOddHole.hpp"

#include "CbcHeuristic.hpp"

#include "OsiSolverParameters.hpp"

static PyObject *pulpCOIN_solve(PyObject *self, PyObject *args);
extern "C" void initpulpCOIN(void);

PyObject *pulpCOIN_solve(PyObject *self, PyObject *args)
{
  PyObject *py_objective, *py_constraints;
  int sense, mip, msg;
  int presolve, dual, crash, scale;
  int rounding, integerPresolve, strong, cuts;

  pulp_lp plp;
  int pulp_status;
  int i, j, last_var;
  
  /* COIN */
  double *collb = NULL, *colub = NULL, *rowlb = NULL, *rowub = NULL,
    *obj = NULL, *value = NULL;
  int *start = NULL, *index = NULL;
  OsiClpSolverInterface solver1;
  CbcModel model(solver1);
  double infinity = solver1.getInfinity();

  if (!PyArg_ParseTuple(args, "O!O!iiiiiiiiiii",
    &PyDict_Type, &py_objective,
    &PyDict_Type, &py_constraints,
    &sense,
    &msg,
    &mip,
    &presolve,
    &dual,
    &crash,
    &scale,
    &rounding,
    &integerPresolve,
    &strong,
    &cuts)) return NULL;

  if (pulp_extract(py_objective, py_constraints, sense, &plp))
  {
      return NULL;
  }
  
  /* Allocation */
  collb = (double *)malloc(sizeof(*collb)*plp.variables_size);
  colub = (double *)malloc(sizeof(*colub)*plp.variables_size);
  obj = (double *)malloc(sizeof(*obj)*plp.variables_size);
  rowlb = (double *)malloc(sizeof(*rowlb)*plp.constraints_size);
  rowub = (double *)malloc(sizeof(*rowub)*plp.constraints_size);
  value = (double *)malloc(sizeof(*value)*plp.constraints_coefs_size);
  index = (int *)malloc(sizeof(*index)*plp.constraints_coefs_size);
  start = (int *)malloc(sizeof(*start)*(plp.variables_size+1));
  
  if (collb == NULL || colub == NULL || obj == NULL || 
    rowlb == NULL || rowub == NULL ||
    value == NULL || index == NULL || start == NULL)
  {
    if (collb != NULL) free(collb);
    if (colub != NULL) free(colub);
    if (rowlb != NULL) free(rowlb);
    if (rowub != NULL) free(rowub);
    if (obj != NULL) free(obj);
    if (value != NULL) free(value);
    if (index != NULL) free(index);
    if (start != NULL) free(start);
    pulp_lp_free(&plp);
    PyErr_SetString(PyExc_MemoryError,
      "Not enough memory for problem.");
    return NULL;
  }
  
  /* Variables */
  for (i=0; i<plp.variables_size; i++)
  {
    collb[i] = plp.variables[i].low;
    if (collb[i] == -HUGE_VAL)
      collb[i] = -infinity;
    colub[i] = plp.variables[i].up;
    if (colub[i] == HUGE_VAL)
      colub[i] = infinity;
    obj[i] = plp.variables[i].obj;
  }

  /* Constraints */
  for (i=0; i<plp.constraints_size; i++)
  {
    if (plp.constraints[i].sense == LpConstraintLE)
    {
      rowlb[i] = -infinity;
      rowub[i] = plp.constraints[i].bound;
    }
    else if (plp.constraints[i].sense == LpConstraintEQ)
    {
      rowlb[i] = plp.constraints[i].bound;
      rowub[i] = plp.constraints[i].bound;
    }
    else /* (plp.constraints[i].sense == LpConstraintGE) */
    {
      rowlb[i] = plp.constraints[i].bound;
      rowub[i] = infinity;
    }
  }
  
  /* Constraints matrix */
  qsort(plp.constraints_coefs, plp.constraints_coefs_size,
    sizeof(*plp.constraints_coefs), pulp_lp_column_major);

  for (i=0, last_var=-1, j=0; i<plp.constraints_coefs_size; i++)
  {
    if (plp.constraints_coefs[i].variable != last_var)
    {
      last_var = plp.constraints_coefs[i].variable;
      for (; j<=last_var; j++)
        start[j] = i;
    }
    index[i] = plp.constraints_coefs[i].constraint;
    value[i] = plp.constraints_coefs[i].coef;
  }
  for (; j<=plp.variables_size; j++)
    start[j] = plp.constraints_coefs_size;
  
  if (msg == 0)
  {
    model.solver()->messageHandler()->setLogLevel(-1);
    model.messageHandler()->setLogLevel(-1);
  }
  else
  {
    model.solver()->messageHandler()->setLogLevel(1);
    model.messageHandler()->setLogLevel(1);
  }
  
  model.solver()->setObjSense(plp.sense); /* MAX = -1, MIN = 1 */

  model.solver()->loadProblem(plp.variables_size, plp.constraints_size, 
    start, index, value,
    collb, colub,
    obj, rowlb, rowub);

  free(collb);
  free(colub);
  free(rowlb);
  free(rowub);
  free(obj);
  free(value);
  free(index);
  free(start);

  if (mip && plp.mip)
    for (i=0; i<plp.variables_size; i++)
      if (plp.variables[i].cat)
        model.solver()->setInteger(i);
  
  if (presolve)
    model.solver()->setHintParam(OsiDoPresolveInInitial,true,OsiHintTry);
  if (dual)
    model.solver()->setHintParam(OsiDoDualInInitial,true,OsiHintTry);
  if (!scale)
    model.solver()->setHintParam(OsiDoScale,false,OsiHintTry);
  if (crash)
    model.solver()->setHintParam(OsiDoCrash,true,OsiHintTry);
  
  model.initialSolve();

  if (model.solver()->isProvenOptimal())
    pulp_status = LpStatusOptimal;
  else if (model.solver()->isProvenPrimalInfeasible())
    pulp_status = LpStatusInfeasible;
  else if (model.solver()->isProvenDualInfeasible())
    pulp_status = LpStatusUnbounded;
  else
    pulp_status = LpStatusUndefined;

  if (pulp_status == LpStatusOptimal && mip && plp.mip)
  {
    model.solver()->messageHandler()->setLogLevel(-1);
    
    model.setNumberStrong(strong);
    
    /* Cuts */
    CglGomory gomory;
    CglProbing probing;
    probing.setUsingObjective(true);
    probing.setMaxPass(3);
    probing.setMaxProbe(100);
    probing.setMaxLook(50);
    probing.setRowCuts(3);
    CglKnapsackCover knapsackCover;
    CglOddHole oddHole;
    oddHole.setMinimumViolation(0.005);
    oddHole.setMinimumViolationPer(0.0002);
    oddHole.setMaximumEntries(100);

    if (cuts)
    {
      model.addCutGenerator(&gomory,-1,"Gomory");
      model.addCutGenerator(&probing,-1,"Probing");
      model.addCutGenerator(&knapsackCover,-1,"Knapsack");
      model.addCutGenerator(&oddHole,-1,"OddHole");
    }

    if (integerPresolve)
    {
      CbcModel *model2 = model.integerPresolve();

      if (model2)
      {
        /* Rounding */
          CbcRounding heuristic(*model2);
        if (rounding)
          model2->addHeuristic(&heuristic);
        model2->branchAndBound();
        model.originalModel(model2,false);
      }
      else
        pulp_status = LpStatusInfeasible;
    }
    else
    {
      /* Rounding */
      CbcRounding heuristic(model);
      if (rounding)
        model.addHeuristic(&heuristic);

      model.branchAndBound();
    }

    if (pulp_status == LpStatusOptimal)
    {
      if (model.isProvenOptimal())
        pulp_status = LpStatusOptimal;
      else if (model.isProvenInfeasible())
        pulp_status = LpStatusInfeasible;
      else
        pulp_status = LpStatusUndefined;
    }
  }

  if (pulp_status == LpStatusOptimal)
  {
    for (i=0; i<plp.variables_size; i++)
      plp.variables[i].value = model.solver()->getColSolution()[i];

    if (pulp_lp_inject(&plp))
    {
      pulp_lp_free(&plp);
      return NULL;
    }
  }
  
  pulp_lp_free(&plp);
  
  return PyInt_FromLong(pulp_status);
}

static PyMethodDef pulpCOIN_methods[] = {
  {"solve", pulpCOIN_solve, METH_VARARGS},
  {NULL, NULL}
};

void initpulpCOIN(void)
{
  (void) Py_InitModule("pulpCOIN", pulpCOIN_methods);
}
