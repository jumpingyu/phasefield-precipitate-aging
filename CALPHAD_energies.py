#!/usr/bin/python
# -*- coding: utf-8 -*-

#####################################################################################
# This software was developed at the National Institute of Standards and Technology #
# by employees of the Federal Government in the course of their official duties.    #
# Pursuant to title 17 section 105 of the United States Code this software is not   #
# subject to copyright protection and is in the public domain. NIST assumes no      #
# responsibility whatsoever for the use of this code by other parties, and makes no #
# guarantees, expressed or implied, about its quality, reliability, or any other    #
# characteristic. We would appreciate acknowledgement if the software is used.      #
#                                                                                   #
# This software can be redistributed and/or modified freely provided that any       #
# derivative works bear some notice that they are derived from it, and any modified #
# versions bear some notice that they have been modified.                           #
#####################################################################################

# Gibbs free energy expressions for IN625 from ternary CALPHAD Database
#
# This script extracts relevant thermodynamic functions necessary for the
# phase-field model of solid-state transformations in additively manufactured
# superalloy 625, represented as a ternary (Cr-Nb-Ni) with γ, δ, μ, and Laves
# phases competing. The thermodynamic database was prepared by U. Kattner after
# Du, Liu, Chang, and Yang (2005):
# 
# @Article{Du2005,
#     Title   = {A thermodynamic modeling of the Cr–Nb–Ni system },
#     Author  = {Yong Du and Shuhong Liu and Y.A. Chang and Ying Yang},
#     Journal = {Calphad},
#     Year    = {2005},
#     Volume  = {29},
#     Number  = {2},
#     Pages   = {140 - 148},
#     Doi     = {10.1016/j.calphad.2005.06.001}
# }
# 
# This database models the phases of interest as follows:
# - γ as $\mathrm{(Cr, Nb, Ni)}$
# - δ as $\mathrm{(\mathbf{Nb}, Ni)_1(Cr, Nb, \mathbf{Ni})_3}$
# - Laves as $\mathrm{(\mathbf{Cr}, Nb, Ni)_2(Cr, \mathbf{Nb})_1}$
# 
# The phase field model requires Gibbs free energies as functions of system
# compositions $x_\mathrm{Cr}$, $x_\mathrm{Nb}$, $x_\mathrm{Ni}$. The CALPHAD
# database represents these energies as functions of sublattice compositions
# $y$ in each phase. To avoid solving for internal phase equilibrium at each
# point in the simulation, approximations have been made to allow the following
# one-to-one mappings between $x$ and $y$:
#
# - γ: no changes necessary
#      * $y_\mathrm{Cr}' = x_\mathrm{Cr}$
#      * $y_\mathrm{Nb}' = x_\mathrm{Nb}$
#      * $y_\mathrm{Ni}' = x_\mathrm{Ni}$
#
# - δ: eliminate Nb from the second (Ni) sublattice, $\mathrm{(\mathbf{Nb}, Ni)_1(Cr, \mathbf{Ni})_3}$
#      * $y_\mathrm{Nb}'  = 4x_\mathrm{Nb}$
#      * $y_\mathrm{Ni}'  = 1 - 4x_\mathrm{Nb}$
#      * $y_\mathrm{Cr}'' = \frac{4}{3}x_\mathrm{Cr}$
#      * $y_\mathrm{Ni}'' = 1 - \frac{4}{3}x_\mathrm{Cr}$
#      * Constraints: $x_\mathrm{Nb}\leq\frac{1}{4}$
#                     $x_\mathrm{Cr}\leq\frac{3}{4}$
#
# - Laves: eliminate Nb from the first (Cr) sublattice, $\mathrm{(\mathbf{Cr}, Ni)_2(Cr, \mathbf{Nb})_1}$
#      * $y_\mathrm{Cr}'  = 1 - \frac{3}{2}x_\mathrm{Ni}$
#      * $y_\mathrm{Ni}'  = \frac{3}{2}x_\mathrm{Ni}$
#      * $y_\mathrm{Cr}'' = 1 - 3x_\mathrm{Nb}$
#      * $y_\mathrm{Nb}'' = 3x_\mathrm{Nb}$
#      * Constraints: $0\leq x_\mathrm{Ni}\leq\frac{2}{3}$
#                     $0\leq x_\mathrm{Nb}\leq\frac{1}{3}$

# Numerical libraries
import numpy as np
from scipy.optimize import fsolve
from sympy.utilities.lambdify import lambdify
from scipy.spatial import ConvexHull

# Runtime / parallel libraries
import time
import warnings

# Thermodynamics and computer-algebra libraries
from pycalphad import Database, calculate, Model
from sympy.utilities.codegen import codegen
from sympy.parsing.sympy_parser import parse_expr
from sympy import And, Ge, Gt, Le, Lt, Or, Piecewise, true
from sympy import diff, symbols, simplify, sympify
from sympy import Abs, exp, pi, tanh

# Visualization libraries
import matplotlib.pylab as plt
from matplotlib.colors import LogNorm

# Constants
epsilon = 1e-10 # tolerance for comparing floating-point numbers to zero
temp = 870 + 273.15 # 1143 Kelvin

alpha_gam = 0.00001 # exclusion zone at phase boundaries in which the spline applies
alpha_del = 0.00001
alpha_lav = 0.00001

RT = 8.3144598*temp # J/mol/K
Vm = 1.0e-5         # m^3/mol
inVm = 1.0 / Vm     # mol/m^3

# Let's avoid integer arithmetic in fractions.
fr13by7 = 13.0/7
fr13by6 = 13.0/6
fr13by3 = 13.0/3
fr13by4 = 13.0/4
fr6by7 = 6.0/7
fr6by13 = 6.0/13
fr7by13 = 7.0/13
fr3by4 = 0.75
fr3by2 = 1.5
fr4by3 = 4.0/3
fr2by3 = 2.0/3
fr3by8 = 0.375
fr1by8 = 0.125
fr1by5 = 1.0/5
fr1by4 = 0.25
fr1by3 = 1.0/3
fr1by2 = 0.5
rt3by2 = np.sqrt(3.0)/2
twopi = 2.0 * pi

# Helper functions to convert compositions into (x,y) coordinates
def simX(x2, x3):
    return x2 + fr1by2 * x3

def simY(x3):
    return rt3by2 * x3

# triangle bounding the Gibbs simplex
XS = [0, simX(1,0), simX(0,1), 0]
YS = [0, simY(0),   simY(1),   0]

# Tick marks along simplex edges
Xtick = []
Ytick = []
for i in range(20):
    # Cr-Ni edge
    xcr = 0.05*i
    xnb = -0.002
    Xtick.append(simX(xnb, xcr))
    Ytick.append(simY(xcr))
    # Cr-Nb edge
    xcr = 0.05*i
    xnb = 1.002 - xcr
    Xtick.append(simX(xnb, xcr))
    Ytick.append(simY(xcr))
    # Nb-Ni edge
    xcr = -0.002
    xnb = 0.05*i
    Xtick.append(simX(xnb, xcr))
    Ytick.append(simY(xcr))

# Read CALPHAD database from disk, specify phases and elements of interest
tdb = Database('Du_Cr-Nb-Ni_simple.tdb')
elements = ['CR', 'NB', 'NI']

species = list(set([i for c in tdb.phases['FCC_A1'].constituents for i in c]))
model = Model(tdb, species, 'FCC_A1')
g_gamma = parse_expr(str(model.ast))

species = list(set([i for c in tdb.phases['D0A_NBNI3'].constituents for i in c]))
model = Model(tdb, species, 'D0A_NBNI3')
g_delta = parse_expr(str(model.ast))

species = list(set([i for c in tdb.phases['C14_LAVES'].constituents for i in c]))
model = Model(tdb, species, 'C14_LAVES')
g_laves = parse_expr(str(model.ast))


# Convert sublattice to phase composition (y to x)
# Declare sublattice variables used in Pycalphad expressions
# Gamma
FCC_A10CR, FCC_A10NB, FCC_A10NI, FCC_A11VA = symbols('FCC_A10CR FCC_A10NB FCC_A10NI FCC_A11VA')
# Delta
D0A_NBNI30NI, D0A_NBNI30NB, D0A_NBNI31CR, D0A_NBNI31NI = symbols('D0A_NBNI30NI D0A_NBNI30NB D0A_NBNI31CR D0A_NBNI31NI')
# Laves
C14_LAVES0CR, C14_LAVES0NI, C14_LAVES1CR, C14_LAVES1NB = symbols('C14_LAVES0CR C14_LAVES0NI C14_LAVES1CR C14_LAVES1NB') 
# Temperature
T = symbols('T')

# Declare system variables for target expressions
XCR, XNB, XNI = symbols('XCR XNB XNI')

# Specify gamma-delta-Laves corners (from phase diagram)
xe_gam_Cr = 0.490
xe_gam_Nb = 0.025
xe_gam_Ni = 1 - xe_gam_Cr - xe_gam_Nb

xe_del_Cr = 0.015
xe_del_Nb = fr1by4 - 0.005

xe_lav_Cr = 0.300
xe_lav_Nb = fr1by3 - 0.005
xe_lav_Ni = 1 - xe_lav_Cr - xe_lav_Nb

# Specify Taylor series expansion points
xt_gam_Cr = 0.40
xt_gam_Nb = 0.20
xt_gam_Ni = 1 - xt_gam_Cr - xt_gam_Nb

xt_del_Cr = 0.10
xt_del_Nb = 0.30

xt_lav_Cr = 0.35
xt_lav_Nb = 0.20
xt_lav_Ni = 1 - xt_lav_Cr - xt_lav_Nb

# Specify upper limit compositions
xcr_del_hi = fr3by4
xnb_del_hi = fr1by4

xnb_lav_hi = fr1by3
xni_lav_hi = fr2by3
xni_lav_hi = fr2by3

# Define slopes and intercepts for linear "funnel" functions
# from range of CALPHAD landscapes at 1143 K
# CALPHAD ranges
#gam_slope =  1.0e9 * (-3.79 + 7.97) # gam range: [-7.97, -3.79]e9 J/m3
#gam_inter = -3.79e9 + 0.25*gam_slope
#
#del_slope =  1.0e9 * (-3.99 + 8.54) # del range: [-8.54, -3.99]e9 J/m3
#del_inter = -3.99e9 + 0.25*del_slope
#
#lav_slope =  1.0e9 * (4.61 + 8.05) # lav range: [-8.05, +4.61]e9 J/m3
#lav_inter =  4.61e9 + 0.25 * lav_slope

# Taylor ranges
gam_slope = 16 * 18e9 # gam range: [-8, 10]e9 J/m3
gam_inter = 10e9 # + 0.25 * gam_slope

del_slope = 8 * 28e9 # del range: [-8, 20]e9 J/m3
del_inter = 20e9 # + 0.25 * del_slope

lav_slope = 16 * 25e9 # lav range: [-10, 15]e9 J/m3
lav_inter = 15e9 # + 0.25 * lav_slope

# Define linear "funnel" functions

f_gamma_Cr_lo = gam_inter - gam_slope * XCR
f_gamma_Nb_lo = gam_inter - gam_slope * XNB
f_gamma_Ni_lo = gam_inter - gam_slope * XNI

f_delta_Cr_lo = del_inter - del_slope * XCR
f_delta_Nb_lo = del_inter - del_slope * XNB
f_delta_Cr_hi = del_inter + del_slope * (XCR - xcr_del_hi)
f_delta_Nb_hi = del_inter + del_slope * (XNB - xnb_del_hi)

f_laves_Nb_lo = lav_inter - lav_slope * XNB
f_laves_Ni_lo = lav_inter - lav_slope * XNI
f_laves_Nb_hi = lav_inter + lav_slope * (XNB - xnb_lav_hi)
f_laves_Ni_hi = lav_inter + lav_slope * (XNI - xni_lav_hi)


# Anchor points for Taylor series
XT = [simX(xt_gam_Nb, xt_gam_Cr), simX(xt_del_Nb, xt_del_Cr), simX(xt_lav_Nb, xt_lav_Cr)]
YT = [simY(xt_gam_Cr),            simY(xt_del_Cr),            simY(xt_lav_Cr)]

# triangle bounding three-phase coexistence
X0 = [simX(xe_gam_Nb, xe_gam_Cr), simX(xe_del_Nb, xe_del_Cr), simX(xe_lav_Nb, xe_lav_Cr)]
Y0 = [simY(xe_gam_Cr),            simY(xe_del_Cr),            simY(xe_lav_Cr)]

# Make sublattice -> system substitutions
g_gamma = inVm * g_gamma.subs({FCC_A10CR: XCR,
                               FCC_A10NB: XNB,
                               FCC_A10NI: 1 - XCR - XNB,
                               FCC_A11VA: 1,
                               T: temp})

g_delta = inVm * g_delta.subs({D0A_NBNI30NB: 4*XNB,
                               D0A_NBNI30NI: 1 - 4*XNB,
                               D0A_NBNI31CR: fr4by3 * XCR,
                               D0A_NBNI31NI: 1 - fr4by3 * XCR,
                               T: temp})

g_laves = inVm * g_laves.subs({C14_LAVES0CR: 1 - fr3by2 * (1 - XCR - XNB),
                               C14_LAVES0NI: fr3by2 * (1 - XCR - XNB),
                               C14_LAVES1CR: 1 - 3*XNB,
                               C14_LAVES1NB: 3 * XNB,
                               T: temp})

# Create Taylor series expansions

# Free-Energy Minima
TA_gam = g_gamma.subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb})
TA_del = g_delta.subs({XCR: xt_del_Cr, XNB: xt_del_Nb})
TA_lav = g_laves.subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb})

# Linear Slopes
TB_gam_Cr = diff(g_gamma, XCR).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb})
TB_gam_Nb = diff(g_gamma, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb})

TB_del_Cr = diff(g_delta, XCR).subs({XCR: xt_del_Cr, XNB: xt_del_Nb})
TB_del_Nb = diff(g_delta, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb})

TB_lav_Cr = diff(g_laves, XCR).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb})
TB_lav_Nb = diff(g_laves, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb})

# Quadratic Curvatures
TC_gam_CrCr = 1.0 * diff(g_gamma, XCR, XCR).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 2
TC_gam_CrNb = 2.0 * diff(g_gamma, XCR, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 2
TC_gam_NbNb = 1.0 * diff(g_gamma, XNB, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 2

TC_del_CrCr = 1.0 * diff(g_delta, XCR, XCR).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 2
TC_del_CrNb = 2.0 * diff(g_delta, XCR, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 2
TC_del_NbNb = 1.0 * diff(g_delta, XNB, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 2

TC_lav_CrCr = 1.0 * diff(g_laves, XCR, XCR).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 2
TC_lav_CrNb = 2.0 * diff(g_laves, XCR, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 2
TC_lav_NbNb = 1.0 * diff(g_laves, XNB, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 2

# Cubic Curvatures
TD_gam_CrCrCr = 1.0 * diff(g_gamma, XCR, XCR, XCR).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 6
TD_gam_CrCrNb = 3.0 * diff(g_gamma, XCR, XCR, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 6
TD_gam_CrNbNb = 3.0 * diff(g_gamma, XCR, XNB, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 6
TD_gam_NbNbNb = 1.0 * diff(g_gamma, XNB, XNB, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 6

TD_del_CrCrCr = 1.0 * diff(g_delta, XCR, XCR, XCR).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 6
TD_del_CrCrNb = 3.0 * diff(g_delta, XCR, XCR, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 6
TD_del_CrNbNb = 3.0 * diff(g_delta, XCR, XNB, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 6
TD_del_NbNbNb = 1.0 * diff(g_delta, XNB, XNB, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 6

TD_lav_CrCrCr = 1.0 * diff(g_laves, XCR, XCR, XCR).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 6
TD_lav_CrCrNb = 3.0 * diff(g_laves, XCR, XCR, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 6
TD_lav_CrNbNb = 3.0 * diff(g_laves, XCR, XNB, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 6
TD_lav_NbNbNb = 1.0 * diff(g_laves, XNB, XNB, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 6

# Quartic Curvatures
TE_gam_CrCrCrCr = 1.0 * diff(g_gamma, XCR, XCR, XCR, XCR).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 24
TE_gam_CrCrCrNb = 4.0 * diff(g_gamma, XCR, XCR, XCR, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 24
TE_gam_CrCrNbNb = 6.0 * diff(g_gamma, XCR, XCR, XNB, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 24
TE_gam_CrNbNbNb = 4.0 * diff(g_gamma, XCR, XNB, XNB, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 24
TE_gam_NbNbNbNb = 1.0 * diff(g_gamma, XNB, XNB, XNB, XNB).subs({XCR: xt_gam_Cr, XNB: xt_gam_Nb}) / 24

TE_del_CrCrCrCr = 1.0 * diff(g_delta, XCR, XCR, XCR, XCR).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 24
TE_del_CrCrCrNb = 4.0 * diff(g_delta, XCR, XCR, XCR, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 24
TE_del_CrCrNbNb = 6.0 * diff(g_delta, XCR, XCR, XNB, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 24
TE_del_CrNbNbNb = 4.0 * diff(g_delta, XCR, XNB, XNB, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 24
TE_del_NbNbNbNb = 1.0 * diff(g_delta, XNB, XNB, XNB, XNB).subs({XCR: xt_del_Cr, XNB: xt_del_Nb}) / 24

TE_lav_CrCrCrCr = 1.0 * diff(g_laves, XCR, XCR, XCR, XCR).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 24
TE_lav_CrCrCrNb = 4.0 * diff(g_laves, XCR, XCR, XCR, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 24
TE_lav_CrCrNbNb = 6.0 * diff(g_laves, XCR, XCR, XNB, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 24
TE_lav_CrNbNbNb = 4.0 * diff(g_laves, XCR, XNB, XNB, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 24
TE_lav_NbNbNbNb = 1.0 * diff(g_laves, XNB, XNB, XNB, XNB).subs({XCR: xt_lav_Cr, XNB: xt_lav_Nb}) / 24

print("Gamma Taylor coefficients:")
print(int(TA_gam))
print(int(TB_gam_Cr), int(TB_gam_Nb))
print(int(TC_gam_CrCr), int(TC_gam_CrNb), int(TC_gam_NbNb))
print(int(TD_gam_CrCrCr), int(TD_gam_CrCrNb), int(TD_gam_CrNbNb), int(TD_gam_NbNbNb))
print(int(TE_gam_CrCrCrCr), int(TE_gam_CrCrCrNb), int(TE_gam_CrCrNbNb), int(TE_gam_CrNbNbNb), int(TE_gam_NbNbNbNb))

print("\nDelta Taylor coefficients:")
print(int(TA_del))
print(int(TB_del_Cr), int(TB_del_Nb))
print(int(TC_del_CrCr), int(TC_del_CrNb), int(TC_del_NbNb))
print(int(TD_del_CrCrCr), int(TD_del_CrCrNb), int(TD_del_CrNbNb), int(TD_del_NbNbNb))
print(int(TE_del_CrCrCrCr), int(TE_del_CrCrCrNb), int(TE_del_CrCrNbNb), int(TE_del_CrNbNbNb), int(TE_del_NbNbNbNb))

print("\nLaves Taylor coefficients:")
print(int(TA_lav))
print(int(TB_lav_Cr), int(TB_lav_Nb))
print(int(TC_lav_CrCr), int(TC_lav_CrNb), int(TC_lav_NbNb))
print(int(TD_lav_CrCrCr), int(TD_lav_CrCrNb), int(TD_lav_CrNbNb), int(TD_lav_NbNbNb))
print(int(TE_lav_CrCrCrCr), int(TE_lav_CrCrCrNb), int(TE_lav_CrCrNbNb), int(TE_lav_CrNbNbNb), int(TE_lav_NbNbNbNb))

# Expressions
t_gamma = TA_gam \
        + TB_gam_Cr * (XCR - xt_gam_Cr) \
        + TB_gam_Nb * (XNB - xt_gam_Nb) \
        + TC_gam_CrCr * (XCR - xt_gam_Cr)**2                        \
        + TC_gam_CrNb * (XCR - xt_gam_Cr)    * (XNB - xt_gam_Nb)    \
        + TC_gam_NbNb                        * (XNB - xt_gam_Nb)**2 \
        + TD_gam_CrCrCr * (XCR - xt_gam_Cr)**3                        \
        + TD_gam_CrCrNb * (XCR - xt_gam_Cr)**2 * (XNB - xt_gam_Nb)    \
        + TD_gam_CrNbNb * (XCR - xt_gam_Cr)    * (XNB - xt_gam_Nb)**2 \
        + TD_gam_NbNbNb                        * (XNB - xt_gam_Nb)**3 \
        + TE_gam_CrCrCrCr * (XCR - xt_gam_Cr)**4                        \
        + TE_gam_CrCrCrNb * (XCR - xt_gam_Cr)**3 * (XNB - xt_gam_Nb)    \
        + TE_gam_CrCrNbNb * (XCR - xt_gam_Cr)**2 * (XNB - xt_gam_Nb)**2 \
        + TE_gam_CrNbNbNb * (XCR - xt_gam_Cr)    * (XNB - xt_gam_Nb)**3 \
        + TE_gam_NbNbNbNb                        * (XNB - xt_gam_Nb)**4

t_delta = TA_del \
        + TB_del_Cr * (XCR - xt_del_Cr) \
        + TB_del_Nb * (XNB - xt_del_Nb) \
        + TC_del_CrCr * (XCR - xt_del_Cr)**2                        \
        + TC_del_CrNb * (XCR - xt_del_Cr)    * (XNB - xt_del_Nb)    \
        + TC_del_NbNb                        * (XNB - xt_del_Nb)**2 \
        + TD_del_CrCrCr * (XCR - xt_del_Cr)**3                        \
        + TD_del_CrCrNb * (XCR - xt_del_Cr)**2 * (XNB - xt_del_Nb)    \
        + TD_del_CrNbNb * (XCR - xt_del_Cr)    * (XNB - xt_del_Nb)**2 \
        + TD_del_NbNbNb                        * (XNB - xt_del_Nb)**3 \
        + TE_del_CrCrCrCr * (XCR - xt_del_Cr)**4                        \
        + TE_del_CrCrCrNb * (XCR - xt_del_Cr)**3 * (XNB - xt_del_Nb)    \
        + TE_del_CrCrNbNb * (XCR - xt_del_Cr)**2 * (XNB - xt_del_Nb)**2 \
        + TE_del_CrNbNbNb * (XCR - xt_del_Cr)    * (XNB - xt_del_Nb)**3 \
        + TE_del_NbNbNbNb                        * (XNB - xt_del_Nb)**4

t_laves = TA_lav \
        + TB_lav_Cr * (XCR - xt_lav_Cr) \
        + TB_lav_Nb * (XNB - xt_lav_Nb) \
        + TC_lav_CrCr * (XCR - xt_lav_Cr)**2                        \
        + TC_lav_CrNb * (XCR - xt_lav_Cr)    * (XNB - xt_lav_Nb)    \
        + TC_lav_NbNb                        * (XNB - xt_lav_Nb)**2 \
        + TD_lav_CrCrCr * (XCR - xt_lav_Cr)**3                        \
        + TD_lav_CrCrNb * (XCR - xt_lav_Cr)**2 * (XNB - xt_lav_Nb)    \
        + TD_lav_CrNbNb * (XCR - xt_lav_Cr)    * (XNB - xt_lav_Nb)**2 \
        + TD_lav_NbNbNb                        * (XNB - xt_lav_Nb)**3 \
        + TE_lav_CrCrCrCr * (XCR - xt_lav_Cr)**4                        \
        + TE_lav_CrCrCrNb * (XCR - xt_lav_Cr)**3 * (XNB - xt_lav_Nb)    \
        + TE_lav_CrCrNbNb * (XCR - xt_lav_Cr)**2 * (XNB - xt_lav_Nb)**2 \
        + TE_lav_CrNbNbNb * (XCR - xt_lav_Cr)    * (XNB - xt_lav_Nb)**3 \
        + TE_lav_NbNbNbNb                        * (XNB - xt_lav_Nb)**4

# Generate interpolation functions using sublattice domain restrictions

psi_gam_lo_Cr = fr1by2 * (1 + tanh(twopi / alpha_gam * (-XCR + fr1by2 * alpha_gam)))
psi_gam_lo_Nb = fr1by2 * (1 + tanh(twopi / alpha_gam * (-XNB + fr1by2 * alpha_gam)))
psi_gam_lo_Ni = fr1by2 * (1 + tanh(twopi / alpha_gam * (-XNI + fr1by2 * alpha_gam)))

psi_del_lo_Cr = fr1by2 * (1 + tanh(twopi / alpha_del * (-XCR              + fr1by2 * alpha_del)))
psi_del_hi_Cr = fr1by2 * (1 + tanh(twopi / alpha_del * ( XCR - xcr_del_hi + fr1by2 * alpha_del)))
psi_del_lo_Nb = fr1by2 * (1 + tanh(twopi / alpha_del * (-XNB              + fr1by2 * alpha_del)))
psi_del_hi_Nb = fr1by2 * (1 + tanh(twopi / alpha_del * ( XNB - xnb_del_hi + fr1by2 * alpha_del)))

psi_lav_lo_Nb = fr1by2 * (1 + tanh(twopi / alpha_lav * (-XNB              + fr1by2 * alpha_lav)))
psi_lav_hi_Nb = fr1by2 * (1 + tanh(twopi / alpha_lav * ( XNB - xnb_lav_hi + fr1by2 * alpha_lav)))
psi_lav_lo_Ni = fr1by2 * (1 + tanh(twopi / alpha_lav * (-XNI              + fr1by2 * alpha_lav)))
psi_lav_hi_Ni = fr1by2 * (1 + tanh(twopi / alpha_lav * ( XNI - xni_lav_hi + fr1by2 * alpha_lav)))


# Generate safe CALPHAD expressions
cornerwt = fr1by4

c_gamma = ((1 - psi_gam_lo_Cr - psi_gam_lo_Nb - psi_gam_lo_Ni
              + psi_gam_lo_Cr * psi_gam_lo_Nb
              + psi_gam_lo_Cr * psi_gam_lo_Ni
              + psi_gam_lo_Nb * psi_gam_lo_Ni) * g_gamma + \
          psi_gam_lo_Cr * (1 - cornerwt * psi_gam_lo_Nb - cornerwt * psi_gam_lo_Ni) * f_gamma_Cr_lo + \
          psi_gam_lo_Nb * (1 - cornerwt * psi_gam_lo_Cr - cornerwt * psi_gam_lo_Ni) * f_gamma_Nb_lo + \
          psi_gam_lo_Ni * (1 - cornerwt * psi_gam_lo_Cr - cornerwt * psi_gam_lo_Nb) * f_gamma_Ni_lo).subs({XNI: 1 - XCR - XNB})

c_delta = (1 - psi_del_lo_Cr - psi_del_hi_Cr - psi_del_lo_Nb - psi_del_hi_Nb
             + psi_del_lo_Cr * psi_del_lo_Nb
             + psi_del_lo_Cr * psi_del_hi_Nb
             + psi_del_hi_Cr * psi_del_lo_Nb
             + psi_del_hi_Cr * psi_del_hi_Nb) * g_delta + \
            psi_del_lo_Cr * (1 - cornerwt * psi_del_lo_Nb - cornerwt * psi_del_hi_Nb) * f_delta_Cr_lo + \
            psi_del_hi_Cr * (1 - cornerwt * psi_del_lo_Nb - cornerwt * psi_del_hi_Nb) * f_delta_Cr_hi + \
            psi_del_lo_Nb * (1 - cornerwt * psi_del_lo_Cr - cornerwt * psi_del_hi_Cr) * f_delta_Nb_lo + \
            psi_del_hi_Nb * (1 - cornerwt * psi_del_lo_Cr - cornerwt * psi_del_hi_Cr) * f_delta_Nb_hi

c_laves = ((1 - psi_lav_lo_Nb - psi_lav_hi_Nb - psi_lav_lo_Ni - psi_lav_hi_Ni
              + psi_lav_lo_Nb * psi_lav_lo_Ni
              + psi_lav_lo_Nb * psi_lav_hi_Ni
              + psi_lav_hi_Nb * psi_lav_lo_Ni
              + psi_lav_hi_Nb * psi_lav_hi_Ni) * g_laves + \
           psi_lav_lo_Nb * (1 - cornerwt * psi_lav_lo_Ni - cornerwt * psi_lav_hi_Ni) * f_laves_Nb_lo + \
           psi_lav_hi_Nb * (1 - cornerwt * psi_lav_lo_Ni - cornerwt * psi_lav_hi_Ni) * f_laves_Nb_hi + \
           psi_lav_lo_Ni * (1 - cornerwt * psi_lav_lo_Nb - cornerwt * psi_lav_hi_Nb) * f_laves_Ni_lo + \
           psi_lav_hi_Ni * (1 - cornerwt * psi_lav_lo_Nb - cornerwt * psi_lav_hi_Nb) * f_laves_Ni_hi).subs({XNI: 1 - XCR - XNB})

# Generate first derivatives of CALPHAD landscape
dGgam_dxCr = diff(g_gamma, XCR)
dGgam_dxNb = diff(g_gamma, XNB)

dGdel_dxCr = diff(g_delta, XCR)
dGdel_dxNb = diff(g_delta, XNB)

dGlav_dxCr = diff(g_laves, XCR)
dGlav_dxNb = diff(g_laves, XNB)

# Generate second derivatives of CALPHAD landscape
d2Ggam_dxCrCr = diff(g_gamma, XCR, XCR)
d2Ggam_dxCrNb = diff(g_gamma, XCR, XNB)
d2Ggam_dxNbCr = diff(g_gamma, XNB, XCR)
d2Ggam_dxNbNb = diff(g_gamma, XNB, XNB)

d2Gdel_dxCrCr = diff(g_delta, XCR, XCR)
d2Gdel_dxCrNb = diff(g_delta, XCR, XNB)
d2Gdel_dxNbCr = diff(g_delta, XNB, XCR)
d2Gdel_dxNbNb = diff(g_delta, XNB, XNB)

d2Glav_dxCrCr = diff(g_laves, XCR, XCR)
d2Glav_dxCrNb = diff(g_laves, XCR, XNB)
d2Glav_dxNbCr = diff(g_laves, XNB, XCR)
d2Glav_dxNbNb = diff(g_laves, XNB, XNB)


## Generate safe Taylor series expressions
#
#t_gamma = (1 - psi_gam_lo_Cr - psi_gam_lo_Nb - psi_gam_lo_Ni
#             + psi_gam_lo_Cr * psi_gam_lo_Nb
#             + psi_gam_lo_Cr * psi_gam_lo_Ni
#             + psi_gam_lo_Nb * psi_gam_lo_Ni) * t_gamma + \
#          psi_gam_lo_Cr * (1 - cornerwt * psi_gam_lo_Nb - cornerwt * psi_gam_lo_Ni) * f_gamma_Cr_lo + \
#          psi_gam_lo_Nb * (1 - cornerwt * psi_gam_lo_Cr - cornerwt * psi_gam_lo_Ni) * f_gamma_Nb_lo + \
#          psi_gam_lo_Ni * (1 - cornerwt * psi_gam_lo_Cr - cornerwt * psi_gam_lo_Nb) * f_gamma_Ni_lo
#
#t_delta = (1 - psi_del_lo_Cr - psi_del_hi_Cr - psi_del_lo_Nb - psi_del_hi_Nb
#             + psi_del_lo_Cr * psi_del_lo_Nb
#             + psi_del_lo_Cr * psi_del_hi_Nb
#             + psi_del_hi_Cr * psi_del_lo_Nb
#             + psi_del_hi_Cr * psi_del_hi_Nb) * t_delta + \
#            psi_del_lo_Cr * (1 - cornerwt * psi_del_lo_Nb - cornerwt * psi_del_hi_Nb) * f_delta_Cr_lo + \
#            psi_del_hi_Cr * (1 - cornerwt * psi_del_lo_Nb - cornerwt * psi_del_hi_Nb) * f_delta_Cr_hi + \
#            psi_del_lo_Nb * (1 - cornerwt * psi_del_lo_Cr - cornerwt * psi_del_hi_Cr) * f_delta_Nb_lo + \
#            psi_del_hi_Nb * (1 - cornerwt * psi_del_lo_Cr - cornerwt * psi_del_hi_Cr) * f_delta_Nb_hi
#
#t_laves = (1 - psi_lav_lo_Nb - psi_lav_hi_Nb - psi_lav_lo_Ni - psi_lav_hi_Ni
#             + psi_lav_lo_Nb * psi_lav_lo_Ni
#             + psi_lav_lo_Nb * psi_lav_hi_Ni
#             + psi_lav_hi_Nb * psi_lav_lo_Ni
#             + psi_lav_hi_Nb * psi_lav_hi_Ni) * t_laves + \
#           psi_lav_lo_Nb * (1 - cornerwt * psi_lav_lo_Ni - cornerwt * psi_lav_hi_Ni) * f_laves_Nb_lo + \
#           psi_lav_hi_Nb * (1 - cornerwt * psi_lav_lo_Ni - cornerwt * psi_lav_hi_Ni) * f_laves_Nb_hi + \
#           psi_lav_lo_Ni * (1 - cornerwt * psi_lav_lo_Nb - cornerwt * psi_lav_hi_Nb) * f_laves_Ni_lo + \
#           psi_lav_hi_Ni * (1 - cornerwt * psi_lav_lo_Nb - cornerwt * psi_lav_hi_Nb) * f_laves_Ni_hi

# Generate first derivatives of Taylor series landscape
t_dGgam_dxCr = diff(t_gamma, XCR)
t_dGgam_dxNb = diff(t_gamma, XNB)

t_dGdel_dxCr = diff(t_delta, XCR)
t_dGdel_dxNb = diff(t_delta, XNB)

t_dGlav_dxCr = diff(t_laves, XCR)
t_dGlav_dxNb = diff(t_laves, XNB)

# Generate second derivatives of Taylor series landscape
t_d2Ggam_dxCrCr = diff(t_gamma, XCR, XCR)
t_d2Ggam_dxCrNb = diff(t_gamma, XCR, XNB)
t_d2Ggam_dxNbCr = diff(t_gamma, XNB, XCR)
t_d2Ggam_dxNbNb = diff(t_gamma, XNB, XNB)

t_d2Gdel_dxCrCr = diff(t_delta, XCR, XCR)
t_d2Gdel_dxCrNb = diff(t_delta, XCR, XNB)
t_d2Gdel_dxNbCr = diff(t_delta, XNB, XCR)
t_d2Gdel_dxNbNb = diff(t_delta, XNB, XNB)

t_d2Glav_dxCrCr = diff(t_laves, XCR, XCR)
t_d2Glav_dxCrNb = diff(t_laves, XCR, XNB)
t_d2Glav_dxNbCr = diff(t_laves, XNB, XCR)
t_d2Glav_dxNbNb = diff(t_laves, XNB, XNB)


# Generate parabolic expressions (the crudest of approximations)

# Free-Energy Minima
PA_gam = 0 #g_gamma.subs({XCR: xe_gam_Cr, XNB: xe_gam_Nb})
PA_del = 0 #g_delta.subs({XCR: xe_del_Cr, XNB: xe_del_Nb})
PA_lav = 0 #g_laves.subs({XCR: xe_lav_Cr, XNB: xe_lav_Nb})

# Slopes
PB_gam_Cr = 0 #diff(g_gamma, XCR).subs({XCR: xe_gam_Cr, XNB: xe_gam_Nb})
PB_gam_Nb = 0 #diff(g_gamma, XNB).subs({XCR: xe_gam_Cr, XNB: xe_gam_Nb})

PB_del_Cr = 0 #diff(g_delta, XCR).subs({XCR: xe_del_Cr, XNB: xe_del_Nb})
PB_del_Nb = 0 #diff(g_delta, XNB).subs({XCR: xe_del_Cr, XNB: xe_del_Nb})

PB_lav_Cr = 0 #diff(g_laves, XCR).subs({XCR: xe_lav_Cr, XNB: xe_lav_Nb})
PB_lav_Nb = 0 #diff(g_laves, XNB).subs({XCR: xe_lav_Cr, XNB: xe_lav_Nb})

# Curvatures
PC_gam_CrCr = 1.0 * diff(g_gamma, XCR, XCR).subs({XCR: xe_gam_Cr, XNB: xe_gam_Nb}) / 2
PC_gam_CrNb = 2.0 * diff(g_gamma, XCR, XNB).subs({XCR: xe_gam_Cr, XNB: xe_gam_Nb}) / 2
PC_gam_NbNb = 1.0 * diff(g_gamma, XNB, XNB).subs({XCR: xe_gam_Cr, XNB: xe_gam_Nb}) / 2

PC_del_CrCr = 1.0 * diff(g_delta, XCR, XCR).subs({XCR: xe_del_Cr, XNB: xe_del_Nb}) / 2
PC_del_CrNb = 2.0 * diff(g_delta, XCR, XNB).subs({XCR: xe_del_Cr, XNB: xe_del_Nb}) / 2
PC_del_NbNb = 1.0 * diff(g_delta, XNB, XNB).subs({XCR: xe_del_Cr, XNB: xe_del_Nb}) / 2

PC_lav_CrCr = 1.0 * diff(g_laves, XCR, XCR).subs({XCR: xe_lav_Cr, XNB: xe_lav_Nb}) / 2
PC_lav_CrNb = 2.0 * diff(g_laves, XCR, XNB).subs({XCR: xe_lav_Cr, XNB: xe_lav_Nb}) / 2
PC_lav_NbNb = 1.0 * diff(g_laves, XNB, XNB).subs({XCR: xe_lav_Cr, XNB: xe_lav_Nb}) / 2

# Expressions
p_gamma = PA_gam \
        + PB_gam_Cr * (XCR - xe_gam_Cr) \
        + PB_gam_Nb * (XNB - xe_gam_Nb) \
        + PC_gam_CrCr * (XCR - xe_gam_Cr)**2                      \
        + PC_gam_CrNb * (XCR - xe_gam_Cr)    * (XNB - xe_gam_Nb)  \
        + PC_gam_NbNb                        * (XNB - xe_gam_Nb)**2

p_delta = PA_del \
        + PB_del_Cr * (XCR - xe_del_Cr) \
        + PB_del_Nb * (XNB - xe_del_Nb) \
        + PC_del_CrCr * (XCR - xe_del_Cr)**2                      \
        + PC_del_CrNb * (XCR - xe_del_Cr)    * (XNB - xe_del_Nb)  \
        + PC_del_NbNb                        * (XNB - xe_del_Nb)**2

p_laves = PA_lav \
        + PB_lav_Cr * (XCR - xe_lav_Cr) \
        + PB_lav_Nb * (XNB - xe_lav_Nb) \
        + PC_lav_CrCr * (XCR - xe_lav_Cr)**2                      \
        + PC_lav_CrNb * (XCR - xe_lav_Cr)    * (XNB - xe_lav_Nb)  \
        + PC_lav_NbNb                        * (XNB - xe_lav_Nb)**2

# Generate first derivatives of Taylor series landscape
p_dGgam_dxCr = diff(p_gamma, XCR)
p_dGgam_dxNb = diff(p_gamma, XNB)

p_dGdel_dxCr = diff(p_delta, XCR)
p_dGdel_dxNb = diff(p_delta, XNB)

p_dGlav_dxCr = diff(p_laves, XCR)
p_dGlav_dxNb = diff(p_laves, XNB)

# Generate second derivatives of Taylor series landscape
p_d2Ggam_dxCrCr = diff(p_gamma, XCR, XCR)
p_d2Ggam_dxCrNb = diff(p_gamma, XCR, XNB)
p_d2Ggam_dxNbCr = diff(p_gamma, XNB, XCR)
p_d2Ggam_dxNbNb = diff(p_gamma, XNB, XNB)

p_d2Gdel_dxCrCr = diff(p_delta, XCR, XCR)
p_d2Gdel_dxCrNb = diff(p_delta, XCR, XNB)
p_d2Gdel_dxNbCr = diff(p_delta, XNB, XCR)
p_d2Gdel_dxNbNb = diff(p_delta, XNB, XNB)

p_d2Glav_dxCrCr = diff(p_laves, XCR, XCR)
p_d2Glav_dxCrNb = diff(p_laves, XCR, XNB)
p_d2Glav_dxNbCr = diff(p_laves, XNB, XCR)
p_d2Glav_dxNbNb = diff(p_laves, XNB, XNB)


print "\nFinished generating CALPHAD, Taylor series, and parabolic energy functions."


# Write CALPHAD functions as C code
codegen([# Gibbs energies
         ('g_gam', g_gamma),
         ('g_del', g_delta),
         ('g_lav', g_laves),
         # Constants
         ('xe_gam_Cr', xt_gam_Cr),
         ('xe_gam_Nb', xt_gam_Nb),
         ('xe_del_Cr', xt_del_Cr),
         ('xe_del_Nb', xt_del_Nb),
         ('xe_lav_Cr', xt_lav_Cr),
         ('xe_lav_Nb', xt_lav_Nb),
         ('xe_lav_Ni', xt_lav_Ni),
         # First derivatives
         ('dg_gam_dxCr', dGgam_dxCr),
         ('dg_gam_dxNb', dGgam_dxNb),
         ('dg_del_dxCr', dGdel_dxCr),
         ('dg_del_dxNb', dGdel_dxNb),
         ('dg_lav_dxCr', dGlav_dxCr),
         ('dg_lav_dxNb', dGlav_dxNb),
         # Second derivatives
         ('d2g_gam_dxCrCr', d2Ggam_dxCrCr),
         ('d2g_gam_dxCrNb', d2Ggam_dxCrNb),
         ('d2g_gam_dxNbCr', d2Ggam_dxNbCr),
         ('d2g_gam_dxNbNb', d2Ggam_dxNbNb),
         ('d2g_del_dxCrCr', d2Gdel_dxCrCr),
         ('d2g_del_dxCrNb', d2Gdel_dxCrNb),
         ('d2g_del_dxNbCr', d2Gdel_dxNbCr),
         ('d2g_del_dxNbNb', d2Gdel_dxNbNb),
         ('d2g_lav_dxCrCr', d2Glav_dxCrCr),
         ('d2g_lav_dxCrNb', d2Glav_dxCrNb),
         ('d2g_lav_dxNbCr', d2Glav_dxNbCr),
         ('d2g_lav_dxNbNb', d2Glav_dxNbNb)],
        language='C', prefix='energy625', project='ALLOY625', to_files=True)

# Write Taylor series functions as C code
codegen([# Gibbs energies
         ('g_gam', t_gamma),
         ('g_del', t_delta),
         ('g_lav', t_laves),
         # Constants
         ('xe_gam_Cr', xt_gam_Cr),
         ('xe_gam_Nb', xt_gam_Nb),
         ('xe_del_Cr', xt_del_Cr),
         ('xe_del_Nb', xt_del_Nb),
         ('xe_lav_Cr', xt_lav_Cr),
         ('xe_lav_Nb', xt_lav_Nb),
         ('xe_lav_Ni', xt_lav_Ni),
         # First derivatives
         ('dg_gam_dxCr', t_dGgam_dxCr),
         ('dg_gam_dxNb', t_dGgam_dxNb),
         ('dg_del_dxCr', t_dGdel_dxCr),
         ('dg_del_dxNb', t_dGdel_dxNb),
         ('dg_lav_dxCr', t_dGlav_dxCr),
         ('dg_lav_dxNb', t_dGlav_dxNb),
         # Second derivatives
         ('d2g_gam_dxCrCr', t_d2Ggam_dxCrCr),
         ('d2g_gam_dxCrNb', t_d2Ggam_dxCrNb),
         ('d2g_gam_dxNbCr', t_d2Ggam_dxNbCr),
         ('d2g_gam_dxNbNb', t_d2Ggam_dxNbNb),
         ('d2g_del_dxCrCr', t_d2Gdel_dxCrCr),
         ('d2g_del_dxCrNb', t_d2Gdel_dxCrNb),
         ('d2g_del_dxNbCr', t_d2Gdel_dxNbCr),
         ('d2g_del_dxNbNb', t_d2Gdel_dxNbNb),
         ('d2g_lav_dxCrCr', t_d2Glav_dxCrCr),
         ('d2g_lav_dxCrNb', t_d2Glav_dxCrNb),
         ('d2g_lav_dxNbCr', t_d2Glav_dxNbCr),
         ('d2g_lav_dxNbNb', t_d2Glav_dxNbNb)],
        language='C', prefix='taylor625', project='ALLOY625', to_files=True)

# Write parabolic functions as C code
codegen([# Gibbs energies
         ('g_gam', p_gamma),
         ('g_del', p_delta),
         ('g_lav', p_laves),
         # Constants
         ('xe_gam_Cr', xe_gam_Cr),
         ('xe_gam_Nb', xe_gam_Nb),
         ('xe_del_Cr', xe_del_Cr),
         ('xe_del_Nb', xe_del_Nb),
         ('xe_lav_Cr', xe_lav_Cr),
         ('xe_lav_Nb', xe_lav_Nb),
         ('xe_lav_Ni', xe_lav_Ni),
         # First derivatives
         ('dg_gam_dxCr', p_dGgam_dxCr),
         ('dg_gam_dxNb', p_dGgam_dxNb),
         ('dg_del_dxCr', p_dGdel_dxCr),
         ('dg_del_dxNb', p_dGdel_dxNb),
         ('dg_lav_dxCr', p_dGlav_dxCr),
         ('dg_lav_dxNb', p_dGlav_dxNb),
         # Second derivatives
         ('d2g_gam_dxCrCr', p_d2Ggam_dxCrCr),
         ('d2g_gam_dxCrNb', p_d2Ggam_dxCrNb),
         ('d2g_gam_dxNbCr', p_d2Ggam_dxNbCr),
         ('d2g_gam_dxNbNb', p_d2Ggam_dxNbNb),
         ('d2g_del_dxCrCr', p_d2Gdel_dxCrCr),
         ('d2g_del_dxCrNb', p_d2Gdel_dxCrNb),
         ('d2g_del_dxNbCr', p_d2Gdel_dxNbCr),
         ('d2g_del_dxNbNb', p_d2Gdel_dxNbNb),
         ('d2g_lav_dxCrCr', p_d2Glav_dxCrCr),
         ('d2g_lav_dxCrNb', p_d2Glav_dxCrNb),
         ('d2g_lav_dxNbCr', p_d2Glav_dxNbCr),
         ('d2g_lav_dxNbNb', p_d2Glav_dxNbNb)],
        language='C', prefix='parabola625', project='ALLOY625', to_files=True)

print "Finished writing CALPHAD, Taylor series, and parabolic energy functions to disk."


# Generate numerically efficient system-composition expressions

# Lambdify unsafe CALPHAD expressions
GG = lambdify([XCR, XNB], g_gamma, modules='sympy')
GD = lambdify([XCR, XNB], g_delta, modules='sympy')
GL = lambdify([XCR, XNB], g_laves, modules='sympy')

# Lambdify safe CALPHAD expressions
CG = lambdify([XCR, XNB], g_gamma, modules='sympy')
CD = lambdify([XCR, XNB], g_delta, modules='sympy')
CL = lambdify([XCR, XNB], g_laves, modules='sympy')

# Lambdify safe Taylor expressions
TG = lambdify([XCR, XNB], t_gamma, modules='sympy')
TD = lambdify([XCR, XNB], t_delta, modules='sympy')
TL = lambdify([XCR, XNB], t_laves, modules='sympy')

# Lambdify parabolic expressions
PG = lambdify([XCR, XNB], p_gamma, modules='sympy')
PD = lambdify([XCR, XNB], p_delta, modules='sympy')
PL = lambdify([XCR, XNB], p_laves, modules='sympy')

print "Finished lambdifying CALPHAD, Taylor series, and parabolic energy functions."
