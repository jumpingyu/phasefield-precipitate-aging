/**********************************************************************************
 HiPerC: High Performance Computing Strategies for Boundary Value Problems
 written by Trevor Keller and available from https://github.com/usnistgov/hiperc

 This software was developed at the National Institute of Standards and Technology
 by employees of the Federal Government in the course of their official duties.
 Pursuant to title 17 section 105 of the United States Code this software is not
 subject to copyright protection and is in the public domain. NIST assumes no
 responsibility whatsoever for the use of this software by other parties, and makes
 no guarantees, expressed or implied, about its quality, reliability, or any other
 characteristic. We would appreciate acknowledgement if the software is used.

 This software can be redistributed and/or modified freely provided that any
 derivative works bear some notice that they are derived from it, and any modified
 versions bear some notice that they have been modified.

 Questions/comments to Trevor Keller (trevor.keller@nist.gov)
 **********************************************************************************/

/**
 \file  boundaries.h
 \brief Declaration of boundary condition function prototypes
*/

/** \cond SuppressGuard */
#ifndef _BOUNDARIES_H_
#define _BOUNDARIES_H_
/** \endcond */

#include "type.h"

/**
 \brief Initialize zero-flux boundary conditions
*/
void apply_initial_conditions(struct HostData* host, const int nx, const int ny, const int nm);

/**
 \brief Set zero-flux boundary values
*/
void apply_boundary_conditions(struct HostData* host, const int nx, const int ny, const int nm);

/** \cond SuppressGuard */
#endif /* _BOUNDARIES_H_ */
/** \endcond */
