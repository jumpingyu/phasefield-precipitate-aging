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
 \file  type.h
 \brief Definition of scalar data type and Doxygen diffusion group
*/

/** \cond SuppressGuard */
#ifndef _TYPE_H_
#define _TYPE_H_
/** \endcond */

#include <curand_kernel.h>

/**
 Specify the basic data type to achieve the desired accuracy in floating-point
 arithmetic: float for single-precision, double for double-precision. This
 choice propagates throughout the code, and may significantly affect runtime
 on GPU hardware.
*/
typedef double fp_t;

/**
 \brief Container for pointers to arrays on the CPU
*/
struct HostData {
	fp_t** mask_lap;

	fp_t** conc_Cr_old;
	fp_t** conc_Cr_new;

	fp_t** conc_Nb_old;
	fp_t** conc_Nb_new;

	fp_t** phi_del_old;
	fp_t** phi_del_new;

	fp_t** phi_lav_old;
	fp_t** phi_lav_new;

	fp_t** gam_Cr;
	fp_t** gam_Nb;
};

/**
 \brief Container for pointers to arrays on the GPU
*/
struct CudaData {
	fp_t* conc_Cr_old;
	fp_t* conc_Cr_new;

	fp_t* conc_Nb_old;
	fp_t* conc_Nb_new;

	fp_t* phi_del_old;
	fp_t* phi_del_new;

	fp_t* phi_lav_old;
	fp_t* phi_lav_new;

	fp_t* gam_Cr;
	fp_t* gam_Nb;

	fp_t* lap_gam_Cr;
	fp_t* lap_gam_Nb;

	curandState* prng;
};

/** \cond SuppressGuard */
#endif /* _TYPE_H_ */
/** \endcond */
