#!/usr/bin/env/python
""" Setup script for PuLP 1.21 added by Stuart Mitchell 2007
Copyright 2007 Stuart Mitchell
"""
#TODO: use this file to make the extensions 
from distutils.core import setup, Extension

pulpCOIN = Extension('pulpCOIN', 
	include_dirs = ['Cbc/src','Cgl/src','Osi/src','Clp/src','Clp/inc',
	'Osi/src/OsiClp','CoinUtils/src','CoinUtils/inc',
	'Cgl/src/CglClique',
	'Cgl/src/CglDuplicateRow',
	'Cgl/src/CglFlowCover',
	'Cgl/src/CglGomory',
	'Cgl/src/CglKnapsackCover',
	'Cgl/src/CglMixedIntegerRounding',
	'Cgl/src/CglMixedIntegerRounding2',
	'Cgl/src/CglOddHole',
	'Cgl/src/CglPreProcess',
	'Cgl/src/CglRedSplit',
	'Cgl/src/CglTwomir',
	'Cgl/src/CglProbing'],
	define_macros = [('HAVE_CONFIG_H',None)],
	libraries = ['OsiClp','Clp','CoinUtils','Cbc','Osi','Cgl'],
	library_dirs =['Cbc/src/.libs','Cgl/src/.libs','Osi/src/.libs',
	'Clp/src/.libs',
	'Osi/src/OsiClp/.libs','CoinUtils/src/.libs'],
	sources = ['pulp-or/src/pulpCOIN.cpp','pulp-or/src/pulp_generic.c',
	'pulp-or/src/mapkit_vpi.c','pulp-or/src/mapkit_generic.c'])

setup(name="PuLP",
      version="1.21.04",
      description="""PuLP is an LP modeler written in python. PuLP can generate MPS or LP files
and call GLPK[1], COIN CLP/CBC[2], CPLEX[3] and XPRESS[4] to solve linear
problems.""",
      author="J.S. Roy and S.A. Mitchell",
      author_email="s.mitchell@auckland.ac.nz",
      url="http://www.esc.auckland.ac.nz/pulp/",
      #ext_modules = [pulpCOIN],
      py_modules=["pulp.pulp","pulp.sparse"],
      package_dir={'pulp':'pulp-or/src'},
      packages = ['pulp'],
      package_data = {'pulp' : ["AUTHORS","LICENSE","pulp.cfg",
                                "CoinMP.dll","LICENSE.CoinMP.txt",
                                "AUTHORS.CoinMP.txt","README.CoinMP.txt",
                                "libCbc.so","libCbcSolver.so",
                                "libCgl.so", "libClp.so", "libCoinMP.so",
                                "libCoinUtils.so","libOsi.so",
                                "libOsiCbc.so", "libOsiClp.so"]}
      )
