#!/usr/bin/python
# -*- coding: utf-8 -*-

# Overlay phase-field simulation compositions on ternary phase diagram

# Usage: python TKR4p149-summary.py

import numpy as np
import matplotlib.pylab as plt
from sympy import Matrix, solve_linear_system, symbols
from sympy.abc import x, y

labels = [r'$\gamma$', r'$\delta$', 'Laves']
colors = ['red', 'green', 'blue']

# Constants
epsilon = 1e-10 # tolerance for comparing floating-point numbers to zero
temp = 1143.15 # 870°C
fr1by2 = 1.0 / 2
rt3by2 = np.sqrt(3.0) / 2
RT = 8.3144598*temp # J/mol/K

# Coexistence vertices
gamCr = 0.490
gamNb = 0.025
delCr = 0.015
delNb = 0.245
lavCr = 0.300
lavNb = 0.328

def simX(xnb, xcr):
    return xnb + fr1by2 * xcr
def simY(xcr):
    return rt3by2 * xcr

# Empirically determined constants for coord transformation
def coord149(n):
    tta = 0.8376
    phi = 0.0997
    psi = 0.4636
    dlx = 0.0205
    dly = 0.4018
    cr = 0.0275 + 0.05 * np.random.rand(1, n)
    nb = 0.0100 + 0.05 * np.random.rand(1, n)
    x = nb * (np.cos(tta) + np.tan(psi)) + cr * (np.sin(tta) + np.tan(phi)) + dlx
    y =-nb * (np.sin(tta) + np.tan(psi)) + cr * (np.cos(tta) + np.tan(phi)) + dly
    return (x, y)

def coord159(n):
    tta =  1.1000
    phi = -0.4000
    psi =  0.7000
    dlx =  0.0075
    dly =  0.4750
    cr = 0.0275 + 0.05 * np.random.rand(1, n)
    nb = 0.0100 + 0.05 * np.random.rand(1, n)
    x = nb * (np.cos(tta) + np.tan(psi)) + cr * (np.sin(tta) + np.tan(phi)) + dlx
    y =-nb * (np.sin(tta) + np.tan(psi)) + cr * (np.cos(tta) + np.tan(phi)) + dly
    return (x, y)

def coord160(n):
    tta =  1.1000
    phi = -0.4500
    psi =  0.8000
    dlx =  0.01
    dly =  0.50
    cr = 0.0275 + 0.05 * np.random.rand(1, n)
    nb = 0.0100 + 0.05 * np.random.rand(1, n)
    x = nb * (np.cos(tta) + np.tan(psi)) + cr * (np.sin(tta) + np.tan(phi)) + dlx
    y =-nb * (np.sin(tta) + np.tan(psi)) + cr * (np.cos(tta) + np.tan(phi)) + dly
    return (x, y)

def coordlist(n):
    dlx = 0.0275
    dly = 0.47
    scx = 0.2
    scy = 0.025
    tta = 0.925
    cr = np.random.rand(1, n)
    nb = np.random.rand(1, n)
    x = nb * scx * np.cos(tta) + cr * scy * np.sin(tta) + dlx
    y =-nb * scx * np.sin(tta) + cr * scy * np.cos(tta) + dly
    return (x, y)

def draw_bisector(A, B):
    bNb = (A * delNb + B * lavNb) / (A + B)
    bCr = (A * delCr + B * lavCr) / (A + B)
    x = [simX(gamNb, gamCr), simX(bNb, bCr)]
    y = [simY(gamCr), simY(bCr)]
    return x, y

# triangle bounding the Gibbs simplex
XS = (0.0, simX(1, 0), simX(0, 1), 0.0)
YS = (0.0, simY(0), simY(1), 0.0)

# triangle bounding three-phase coexistence
X0 = (simX(gamNb, gamCr), simX(delNb, delCr), simX(lavNb, lavCr), simX(gamNb, gamCr))
Y0 = (simY(gamCr), simY(delCr), simY(lavCr), simY(gamCr))

# Tick marks along simplex edges
Xtick = []
Ytick = []
tickdens = 10
for i in range(tickdens):
    # Cr-Ni edge
    xcr = (1.0 * i) / tickdens
    xni = 1.0 - xcr
    Xtick.append(simX(-0.002, xcr))
    Ytick.append(simY(xcr))
    # Cr-Nb edge
    xcr = (1.0 * i) / tickdens
    xnb = 1.0 - xcr
    Xtick.append(simX(xnb+0.002, xcr))
    Ytick.append(simY(xcr))
    # Nb-Ni edge
    xnb = (1.0 * i) / tickdens
    Xtick.append(xnb)
    Ytick.append(-0.002)

# Triangular grid
XG = [[]]
YG = [[]]
for a in np.arange(0, 1, 0.1):
    # x1--x2: lines of constant x2=a
    XG.append([simX(a, 0), simX(a, 1-a)])
    YG.append([simY(0),    simY(1-a)])
    # x2--x3: lines of constant x3=a
    XG.append([simX(0, a), simX(1-a, a)])
    YG.append([simY(a),    simY(a)])
    # x1--x3: lines of constant x1=1-a
    XG.append([simX(0, a), simX(a, 0)])
    YG.append([simY(a),    simY(0)])


# Plot ternary axes and labels
plt.figure(0, figsize=(10, 7.5)) # inches
plt.title("Phase Coexistence at %.0f K"%temp, fontsize=18)
plt.xlabel(r'$x_\mathrm{Nb}$', fontsize=24)
plt.ylabel(r'$x_\mathrm{Cr}$', fontsize=24)
plt.xticks(np.linspace(0, 1, 11))
plt.plot(XS, YS, '-k')
plt.scatter(Xtick, Ytick, color='black', s=3, zorder=5)
plt.plot(X0, Y0, color='black', zorder=5)
for a in range(len(XG)):
    plt.plot(XG[a], YG[a], ':k', linewidth=0.5, alpha=0.5, zorder=1)

# Plot generated data, colored by phase
base, xCr, xNb, phase = np.genfromtxt('TKR4p149-summary-permanent.csv', delimiter=',', dtype=None, unpack=True, skip_header=0)
dCr = np.array(xCr[phase == 'D'], dtype=float)
dNb = np.array(xNb[phase == 'D'], dtype=float)
lCr = np.array(xCr[phase == 'L'], dtype=float)
lNb = np.array(xNb[phase == 'L'], dtype=float)
plt.scatter(simX(dNb, dCr), simY(dCr), s=12, c="blue", zorder=2)
plt.scatter(simX(lNb, lCr), simY(lCr), s=12, c="orange", zorder=2)

# Plot surveyed regions using random points on [0,1)
xB, yB = draw_bisector(6., 5.)
plt.plot(xB, yB, c="green", lw=2, zorder=1)

gann = plt.text(simX(0.010, 0.495), simY(0.495), r'$\gamma$', fontsize=14)
plt.xlim([0.20, 0.48])
plt.ylim([0.25, 0.50])
plt.savefig("diagrams/TKR4p149/coexistence-149.png", dpi=300, bbox_inches='tight')
plt.close()




# Define lever rule equations
x0, y0 = symbols('x0 y0')
xb, yb = symbols('xb yb')
xc, yc = symbols('xc yc')
xd, yd = symbols('xd yd')

system = Matrix(( (y0 - yb, xb - x0, xb * (y0 - yb) + yb * (xb - x0)),
                  (yc - yd, xd - xc, xd * (yc - yd) + yd * (xd - xc)) ))
levers = solve_linear_system(system, x, y)

# Plot ternary axes and labels
plt.figure(0, figsize=(10, 7.5)) # inches
plt.title("Phase Coexistence at %.0f K"%temp, fontsize=18)
plt.xlabel(r'$x_\mathrm{Nb}$', fontsize=24)
plt.ylabel(r'$x_\mathrm{Cr}$', fontsize=24)
plt.xticks(np.linspace(0, 1, 11))
plt.plot(XS, YS, '-k')
plt.scatter(Xtick, Ytick, color='black', s=3, zorder=5)
plt.plot(X0, Y0, color='black', zorder=5)
for a in range(len(XG)):
    plt.plot(XG[a], YG[a], ':k', linewidth=0.5, alpha=0.5, zorder=1)
gann = plt.text(simX(0.010, 0.495), simY(0.495), r'$\gamma$', fontsize=14)
plt.xlim([0.20, 0.48])
plt.ylim([0.25, 0.50])

N = 500
coords = np.array(coordlist(N)).T.reshape((N, 2))

for rNb, rCr in coords:
    # Compute equilibrium delta fraction
    aNb = float(levers[x].subs({x0: rNb, y0: rCr, xb: delNb, yb: delCr, xc: lavNb, yc: lavCr, xd: gamNb, yd: gamCr}))
    aCr = float(levers[y].subs({x0: rNb, y0: rCr, xb: delNb, yb: delCr, xc: lavNb, yc: lavCr, xd: gamNb, yd: gamCr}))
    lAO = np.sqrt((aNb - rNb)**2 + (aCr - rCr)**2)
    lAB = np.sqrt((aNb - delNb)**2 + (aCr - delCr)**2)
    fd0 = lAO / lAB

    # Compute equilibrium Laves fraction
    aNb = float(levers[x].subs({x0: rNb, y0: rCr, xb: lavNb, yb: lavCr, xc: gamNb, yc: gamCr, xd: delNb, yd: delCr}))
    aCr = float(levers[y].subs({x0: rNb, y0: rCr, xb: lavNb, yb: lavCr, xc: gamNb, yc: gamCr, xd: delNb, yd: delCr}))
    lAO = np.sqrt((aNb - rNb)**2 + (aCr - rCr)**2)
    lAB = np.sqrt((aNb - lavNb)**2 + (aCr - lavCr)**2)
    fl0 = lAO / lAB

    # Collate data, colored by phase
    if fd0 >= fl0:
        plt.scatter(simX(rNb, rCr), simY(rCr), s=12, c="blue", zorder=2)
    else:
        plt.scatter(simX(rNb, rCr), simY(rCr), s=12, c="orange", zorder=2)

xB, yB = draw_bisector(6., 5.)
plt.plot(xB, yB, c="green", lw=2, zorder=1)
xB, yB = draw_bisector(1., 1.)
plt.plot(xB, yB, c="black", lw=2, zorder=1)
xB, yB = draw_bisector(5., 7.)
plt.plot(xB, yB, c="red", lw=2, zorder=1)
plt.savefig("diagrams/TKR4p149/prediction.png", dpi=300, bbox_inches='tight')
plt.close()
