// nucleation.h

#ifndef __NUCLEATION_H_
#define __NUCLEATION_H_

#include "globals.h"

void nucleation_driving_force_delta(const fp_t& xCr, const fp_t& xNb, fp_t* dG);

void nucleation_driving_force_laves(const fp_t& xCr, const fp_t& xNb, fp_t* dG);

void nucleation_probability_sphere(const fp_t& xCr, const fp_t& xNb,
                                   const fp_t& dG_chem,
                                   const fp_t& D_CrCr, const fp_t& D_NbNb,
                                   const fp_t& sigma,
                                   const fp_t& Vatom,
                                   const fp_t& n_gam, const fp_t& dV, const fp_t& dt,
                                   fp_t* Rstar, fp_t* P_nuc);

#endif
