/*************************************************************************************
 * File: cuda625.cpp                                                                 *
 * Algorithms for 2D and 3D isotropic Cr-Nb-Ni alloy phase transformations           *
 * This implementation depends on the GNU Scientific Library for multivariate root   *
 * finding algorithms, and Mesoscale Microstructure Simulation Project for high-     *
 * performance grid operations in parallel.                                          *
 *                                                                                   *
 * Questions/comments to trevor.keller@nist.gov (Trevor Keller, Ph.D.)               *
 *                                                                                   *
 * This software was developed at the National Institute of Standards and Technology *
 * by employees of the Federal Government in the course of their official duties.    *
 * Pursuant to title 17 section 105 of the United States Code this software is not   *
 * subject to copyright protection and is in the public domain. NIST assumes no      *
 * responsibility whatsoever for the use of this code by other parties, and makes no *
 * guarantees, expressed or implied, about its quality, reliability, or any other    *
 * characteristic. We would appreciate acknowledgement if the software is used.      *
 *                                                                                   *
 * This software can be redistributed and/or modified freely provided that any       *
 * derivative works bear some notice that they are derived from it, and any modified *
 * versions bear some notice that they have been modified. Derivative works that     *
 * include MMSP or other software licensed under the GPL may be subject to the GPL.  *
 *************************************************************************************/

#ifndef __CUDA625_CPP__
#define __CUDA625_CPP__

#include <set>
#include <cmath>
#include <random>
#include <sstream>
#include <vector>
#ifdef _OPENMP
#include "omp.h"
#endif
#include "MMSP.hpp"
#include "alloy625.hpp"
#include "cuda_data.h"
#include "mesh.h"
#include "numerics.h"
#include "output.h"
#include "parabola625.h"
#include "nucleation.h"

// Kinetic and model parameters
const double meshres = 0.25e-9;      // grid spacing (m)
const fp_t alpha = 1.07e11;          // three-phase coexistence coefficient (J/m^3)
const fp_t LinStab = 1.0 / 19.42501; // threshold of linear (von Neumann) stability

// Diffusion constants in FCC Ni from Xu (m^2/s)
//                     Cr        Nb
const fp_t D_Cr[NC] = {2.16e-15, 0.56e-15}; // first column of diffusivity matrix
const fp_t D_Nb[NC] = {2.97e-15, 4.29e-15}; // second column of diffusivity matrix
const fp_t aFccNi = 0.352e-9;               // lattice spacing of FCC nickel (m)

// Define st.dev. of bell curves for alloying element segregation
//                       Cr      Nb
const double bell[NC] = {150e-9, 50e-9}; // est. between 80-200 nm from SEM

// Precipitate radii: minimum for thermodynamic stability is 7.5 nm,
// minimum for numerical stability is 14*dx (due to interface width).
const fp_t rPrecip[NP] = {7.5 * 5e-9 / meshres,  // delta
                          7.5 * 5e-9 / meshres}; // Laves

// Choose numerical diffusivity to lock chemical and transformational timescales
//                      delta      Laves
const fp_t kappa[NP] = {1.24e-8, 1.24e-8};     // gradient energy coefficient (J/m)
const fp_t Lmob[NP]  = {2.904e-11, 2.904e-11}; // numerical mobility (m^2/Ns)
const fp_t sigma[NP] = {1.010, 1.010};         // interfacial energy (J/m^2)

// Compute interfacial width (nm) and well height (J/m^3)
const fp_t ifce_width = 10. * meshres;
const fp_t width_factor = 2.2; // 2.2 if interface is [0.1,0.9]; 2.94 if [0.05,0.95]
const fp_t omega[NP] = {3.0 * width_factor* sigma[0] / ifce_width,  // delta
                        3.0 * width_factor* sigma[1] / ifce_width}; // Laves

namespace MMSP
{


void generate(int dim, const char* filename)
{
	int rank = 0;
	#ifdef MPI_VERSION
	rank = MPI::COMM_WORLD.Get_rank();
	#endif

	FILE* cfile = NULL;

	if (rank == 0)
		cfile = fopen("c.log", "w"); // existing log will be overwritten

	const double dtTransformLimited = (meshres*meshres) / (std::pow(2.0, dim) * Lmob[0]*kappa[0]);
	const double dtDiffusionLimited = (meshres*meshres) / (std::pow(2.0, dim) * std::max(D_Cr[0], D_Nb[1]));
	const fp_t dt = LinStab * std::min(dtTransformLimited, dtDiffusionLimited);

	// Initialize pseudo-random number generator
	std::random_device rd;     // seed generator
	std::mt19937 mtrand(rd()); // Mersenne Twister PRNG
	std::uniform_real_distribution<double> unidist(0, 1);

	if (dim==2) {
		const int Nx = 4000;
		const int Ny = 2500;
		double Ntot = 1.0;
		GRID2D initGrid(2*NC+NP, -Nx/2, Nx/2, -Ny/2, Ny/2);
		for (int d = 0; d < dim; d++) {
			dx(initGrid,d) = meshres;
			Ntot *= g1(initGrid, d) - g0(initGrid, d);
			if (x0(initGrid, d) == g0(initGrid, d))
				b0(initGrid, d) = Neumann;
			if (x1(initGrid, d) == g1(initGrid, d))
				b1(initGrid, d) = Neumann;
		}

		if (rank == 0)
			std::cout << "Timestep dt=" << dt
			          << ". Linear stability limits: dtTransformLimited=" << dtTransformLimited
			          << ", dtDiffusionLimited=" << dtDiffusionLimited
			          << '.' << std::endl;

		/* Randomly choose enriched compositions in a circular region of the phase diagram near the
		 * gamma corner of three-phase coexistence triangular phase field. Set system composition
		 * to the nearest point on the triangular phase field to the IN625 composition.
		 */
		const double xCr0 = 0.3250;
		const double xNb0 = 0.1025;
		const double xe = 0.4800;
		const double ye = 0.0430;
		const double re = 0.0100;
		double xCrE = xe + re * (unidist(mtrand) - 1.);
		double xNbE = ye + re * (unidist(mtrand) - 1.);

		#ifdef MPI_VERSION
		MPI::COMM_WORLD.Barrier();
		MPI::COMM_WORLD.Bcast(&xCrE, 1, MPI_DOUBLE, 0);
		MPI::COMM_WORLD.Bcast(&xNbE, 1, MPI_DOUBLE, 0);
		#endif

		// Zero initial condition
		const MMSP::vector<fp_t> blank(MMSP::fields(initGrid), 0.);
		#ifdef _OPENMP
		#pragma omp parallel for
		#endif
		for (int n = 0; n < MMSP::nodes(initGrid); n++) {
			initGrid(n) = blank;
		}

		vector<int> x(2, 0);

		// Initialize matrix (gamma phase): bell curve along x-axis for Cr and Nb composition
		const double avgCr = bell_average(-Nx*meshres/2, Nx*meshres/2, bell[0]);
		const double avgNb = bell_average(-Nx*meshres/2, Nx*meshres/2, bell[1]);
		for (x[1]=y0(initGrid); x[1]<y1(initGrid); x[1]++) {
			for (x[0]=x0(initGrid); x[0]<x1(initGrid); x[0]++) {
				const double pos = dx(initGrid,0)*x[0];
				const double matrixCr = xCr0 + (xCrE - xCr0) * (1. - avgCr)
					                         * (bell_curve(bell[0], pos) - avgCr);
				const double matrixNb = xNb0 + (xNbE - xNb0) * (1. - avgNb)
					                         * (bell_curve(bell[1], pos) - avgNb);
				initGrid(x)[0] = matrixCr;
				initGrid(x)[1] = matrixNb;
				update_compositions(initGrid(x));
			}
		}

		// Embed two particles as a sanity check
		const int rad = 64;
		const fp_t w = ifce_width / meshres;
		x[0] = g0(initGrid, 0) / 2;
		x[1] = 0;
		for (int i = -2*rad; i < 2*rad; i++) {
			for (int j = -2*rad; j < 2*rad; j++) {
				vector<int> y(x);
				y[0] += i;
				y[1] += j;
				const fp_t r = sqrt(i*i + j*j);
				const fp_t f = interface_profile(w, r - rad - w);
				initGrid(y)[2] = f;
				y[0] *= -1;
				initGrid(y)[3] = f;
			}
		}

		ghostswap(initGrid);

		vector<double> summary = summarize_fields(initGrid);
		double energy = summarize_energy(initGrid);

		if (rank == 0) {
			fprintf(cfile, "%9s\t%9s\t%9s\t%9s\t%9s\t%9s\t%9s\t%9s\t%9s\n",
			        "ideal", "timestep", "x_Cr", "x_Nb", "gamma", "delta", "Laves", "free_energy", "ifce_vel");
			fprintf(cfile, "%9g\t%9g\t%9g\t%9g\t%9g\t%9g\t%9g\t%9g\n",
			        dt, dt, summary[0], summary[1], summary[2], summary[3], summary[4], energy);

			printf("%9s %9s %9s %9s %9s %9s\n",
			       "x_Cr", "x_Nb", "x_Ni", " p_g", " p_d", " p_l");
			printf("%9g %9g %9g %9g %9g %9g\n",
			       summary[0], summary[1], 1.0-summary[0]-summary[1], summary[2], summary[3], summary[4]);
		}

		output(initGrid,filename);


	} else {
		std::cerr << "Error: " << dim << "-dimensional grids unsupported." << std::endl;
		MMSP::Abort(-1);
	}

	if (rank == 0)
		fclose(cfile);
}

} // namespace MMSP


double radius(const MMSP::vector<int>& a, const MMSP::vector<int>& b, const double& dx)
{
	double r = 0.0;
	for (int i = 0; i < a.length() && i < b.length(); i++)
		r += std::pow(a[i] - b[i], 2.0);
	return dx*std::sqrt(r);
}

template<typename T>
void update_compositions(MMSP::vector<T>& GRIDN)
{
	const T& xcr = GRIDN[0];
	const T& xnb = GRIDN[1];

	const T fdel = h(GRIDN[2]);
	const T flav = h(GRIDN[3]);
	const T fgam = 1. - fdel - flav;

	const T inv_det = inv_fict_det(fdel, fgam, flav);
	GRIDN[NC+NP  ] = fict_gam_Cr(inv_det, xcr, xnb, fdel, fgam, flav);
	GRIDN[NC+NP+1] = fict_gam_Nb(inv_det, xcr, xnb, fdel, fgam, flav);
}

template <typename T>
T gibbs(const MMSP::vector<T>& v)
{
	const T xCr = v[0];
	const T xNb = v[1];
	const T f_del = h(v[NC  ]);
	const T f_lav = h(v[NC+1]);
	const T f_gam = 1.0 - f_del - f_lav;
	const T inv_det = inv_fict_det(f_del, f_gam, f_lav);
	const T gam_Cr = v[NC+NP];
	const T gam_Nb = v[NC+NP];
	const T del_Cr = fict_del_Cr(inv_det, xCr, xNb, f_del, f_gam, f_lav);
	const T del_Nb = fict_del_Nb(inv_det, xCr, xNb, f_del, f_gam, f_lav);
	const T lav_Cr = fict_lav_Cr(inv_det, xCr, xNb, f_del, f_gam, f_lav);
	const T lav_Nb = fict_lav_Nb(inv_det, xCr, xNb, f_del, f_gam, f_lav);

	MMSP::vector<T> vsq(NP);

	for (int i = 0; i < NP; i++)
		vsq[i] = v[NC+i]*v[NC+i];

	T g  = f_gam * g_gam(gam_Cr, gam_Nb);
	g += f_del * g_del(del_Cr, del_Nb);
	g += f_lav * g_lav(lav_Cr, lav_Nb);

	for (int i = 0; i < NP; i++)
		g += omega[i] * vsq[i] * pow(1.0 - v[NC+i], 2);

	// Trijunction penalty
	for (int i = 0; i < NP-1; i++)
		for (int j = i+1; j < NP; j++)
			g += 2.0 * alpha * vsq[i] *vsq[j];

	return g;
}

template <int dim, typename T>
MMSP::vector<T> maskedgradient(const MMSP::grid<dim,MMSP::vector<T> >& GRID, const MMSP::vector<int>& x, const int N)
{
	MMSP::vector<T> gradient(dim);
	MMSP::vector<int> s = x;

	for (int i = 0; i < dim; i++) {
		s[i] += 1;
		const T& yh = GRID(s)[N];
		s[i] -= 2;
		const T& yl = GRID(s)[N];
		s[i] += 1;

		double weight = 1.0 / (2.0 * dx(GRID, i));
		gradient[i] = weight * (yh - yl);
	}

	return gradient;
}

template<int dim,class T>
MMSP::vector<double> summarize_fields(MMSP::grid<dim,MMSP::vector<T> > const& GRID)
{
	#ifdef MPI_VERSION
	MPI_Request reqs;
	MPI_Status stat;
	#endif

	double Ntot = 1.0;
	for (int d = 0; d<dim; d++)
		Ntot *= double(MMSP::g1(GRID, d) - MMSP::g0(GRID, d));

	MMSP::vector<double> summary(NC+NP+1, 0.0);

	#ifdef _OPENMP
	#pragma omp parallel for
	#endif
	for (int n = 0; n<MMSP::nodes(GRID); n++) {
		MMSP::vector<int> x = MMSP::position(GRID,n);
		MMSP::vector<T>& gridN = GRID(n);
		MMSP::vector<double> mySummary(NC+NP+1, 0.0);

		for (int i = 0; i < NC; i++)
			mySummary[i] = gridN[i]; // compositions

		mySummary[NC] = 1.0; // gamma fraction init

		for (int i = 0; i < NP; i++) {
			const T newPhaseFrac = h(gridN[NC+i]);

			mySummary[NC+i+1] = newPhaseFrac;  // secondary phase fraction
			mySummary[NC    ] -= newPhaseFrac; // contributes to gamma phase;
		}

		#ifdef _OPENMP
		#pragma omp critical (critSum)
		{
		#endif
			summary += mySummary;
		#ifdef _OPENMP
		}
		#endif
	}

	for (int i = 0; i < NC+NP+1; i++)
		summary[i] /= Ntot;

	#ifdef MPI_VERSION
	MMSP::vector<double> temp(summary);
	MPI_Ireduce(&(temp[0]), &(summary[0]), NC+NP+1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD, &reqs);
	MPI_Wait(&reqs, &stat);
	#endif

	return summary;
}

template<int dim,class T>
double summarize_energy(MMSP::grid<dim,MMSP::vector<T> > const& GRID)
{
	#ifdef MPI_VERSION
	MPI_Request reqs;
	MPI_Status stat;
	#endif

	double dV = 1.0;
	for (int d = 0; d<dim; d++)
		dV *= MMSP::dx(GRID, d);

	double energy = 0.0;

	#ifdef _OPENMP
	#pragma omp parallel for
	#endif
	for (int n = 0; n<MMSP::nodes(GRID); n++) {
		MMSP::vector<int> x = MMSP::position(GRID,n);
		MMSP::vector<T>& gridN = GRID(n);

		double myEnergy = dV * gibbs(gridN); // energy density init

		for (int i = 0; i < NP; i++) {
			const MMSP::vector<T> gradPhi = maskedgradient(GRID, x, NC+i);
			const T gradSq = (gradPhi * gradPhi); // vector inner product

			myEnergy += dV * kappa[i] * gradSq; // gradient contributes to energy
		}

		#ifdef _OPENMP
		#pragma omp atomic
		#endif
		energy += myEnergy;
	}

	#ifdef MPI_VERSION
	double temp(energy);
	MPI_Ireduce(&temp, &energy, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD, &reqs);
	MPI_Wait(&reqs, &stat);
	#endif

	return energy;
}

#endif

#include "main.cpp"
