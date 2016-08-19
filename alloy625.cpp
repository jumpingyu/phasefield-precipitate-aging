// alloy625.cpp
// Algorithms for 2D and 3D isotropic Mo-Nb-Ni alloy phase transformations
// Questions/comments to trevor.keller@nist.gov (Trevor Keller)

// This implementation depends on the GNU Scientific Library
// for multivariate root finding algorithms, and
// The Mesoscale Microstructure Simulation Project for
// high-performance grid operations in parallel. Use of these
// software packages does not constitute endorsement by the
// National Institute of Standards and Technology.


#ifndef ALLOY625_UPDATE
#define ALLOY625_UPDATE
#include<cmath>
#include<gsl/gsl_blas.h>
#include<gsl/gsl_vector.h>
#include<gsl/gsl_multiroots.h>
#include<gsl/gsl_interp2d.h>
#include<gsl/gsl_spline2d.h>

#include"MMSP.hpp"
#include"alloy625.hpp"
#include"energy625.c"

// Note: alloy625.hpp contains important declarations and comments. Have a look.
//       energy625.c is generated from CALPHAD using pycalphad and SymPy, in CALPHAD_extraction.ipynb.

// Equilibrium compositions from CALPHAD: gamma, mu, delta
const double cMoEq[4] = {0.5413, 0.0, 0.3940};
const double cNbEq[4] = {0.5413, 0.0, 0.3940};

// Numerical stability (Courant-Friedrich-Lewy) parameters
const double epsilon = 1.0e-10;  // what to consider zero to avoid log(c) explosions
const double CFL = 0.2; // controls timestep

const bool useNeumann = true; // apply zero-flux boundaries (Neumann type)

// Kinetic and model parameters
const double meshres = 0.075; // dx=dy
const double eps_sq = 1.25;
const double a_int = 2.5; // alpha, prefactor of interface width
const double halfwidth = 5.0*meshres; // half the interface width
const double omega = 2.0*eps_sq*pow(a_int/halfwidth,2.0);
const double dt_plimit = CFL*meshres/eps_sq;          // speed limit based on numerical viscosity
const double dt_climit = CFL*pow(meshres,2.0)/eps_sq; // speed limit based on diffusion timescale
const double dt = std::min(dt_plimit, dt_climit);


/* =============================================== *
 * Implement MMSP kernels: generate() and update() *
 * =============================================== */

namespace MMSP
{

/* Representation includes thirteen field variables:
 *
 * X0.  molar fraction of Al, Cr, Mo
 * X1.  molar fraction of Nb, Fe
 * bal. molar fraction of Ni
 *
 * P2.  phase fraction of mu
 * P3.  phase fraction of delta
 * bal. phase fraction of gamma
 *
 * C4. fictitious Mo concentration in gamma
 * C5. fictitious Nb concentration in gamma
 * C6. fictitious Ni concentration in gamma
 *
 * C7. fictitious Mo concentration in mu
 * C8. fictitious Nb concentration in mu
 * C9. fictitious Ni concentration in mu
 *
 * C10. fictitious Mo concentration in delta
 * C11. fictitious Nb concentration in delta
 * C12. fictitious Ni concentration in delta
 */

const double noise_amp = 0.00625;


// Define equilibrium phase compositions at global scope
//                     gamma   mu      delta
const double xMo[3] = {0.0161, 0.0007, 0.0007};
const double xNb[3] = {0.0072, 0.0196, 0.0196};
const double xNbdep = 0.5*xNb[0]; // leftover Nb in depleted gamma phase near precipitate particle

// Define st.dev. of Gaussians for alloying element segregation
//                     Mo      Nb
const double bell[2] = {0.0625, 0.025};

double radius(const vector<int>& a, const vector<int>& b, const double& dx)
{
	double r = 0.0;
	for (int i=0; i<length(a) && i<length(b); i++)
		r += std::pow(a[i]-b[i],2.0);
	return dx*std::sqrt(r);
}

double bellCurve(double x, double m, double s)
{
	return std::exp( -std::pow(x-m,2.0)/(2.0*s*s) );
}

// Initial guesses for gamma, mu, and delta equilibrium compositions
void guessGamma(const double& xmo, const double& xnb, double& cmo, double& cnb, double& cni)
{
	cmo = xmo/(xmo+xnb+0.9);
	cnb = xnb/(xmo+xnb+0.9);
	cni = 0.9;
}
void guessDelta(const double& xmo, const double& xnb, double& cmo, double& cnb, double& cni)
{
	cmo = xmo/(xmo+xnb+0.75);
	cnb = xnb/(xmo+xnb+0.75);
	cni = 0.75;
}
void guessMu(const double& xmo, const double& xnb, double& cmo, double& cnb, double& cni)
{
	cmo = 0.025;
	cnb = 0.4875;
	cni = 0.4875;
}


void generate(int dim, const char* filename)
{
	int rank=0;
	#ifdef MPI_VERSION
	rank = MPI::COMM_WORLD.Get_rank();
	#endif
	// Utilize Mersenne Twister from C++11 standard
	std::mt19937_64 mt_rand(time(NULL)+rank);
	std::uniform_real_distribution<double> real_gen(-1,1);

	if (dim==2) {
		const int edge = 128;
		GRID2D initGrid(13,0,edge,0,edge);
		for (int d=0; d<dim; d++)
			dx(initGrid,d)=meshres;

		vector<int> originMu(2,edge/2);
		originMu[1] = 0.25*edge;
		vector<int> originDelta(2,edge/2);
		originMu[1] = 0.75*edge;

		const double rPrecipMu = 3.5*dx(initGrid,0);
		const double rDepltMu = rPrecipMu*std::sqrt(1.0+2.0*xNb[1]/xNbdep); // radius of annular depletion region
		const double rPrecipDelta = 6.5*dx(initGrid,0);
		const double rDepltDelta = rPrecipDelta*std::sqrt(1.0+2.0*xNb[1]/xNbdep); // radius of annular depletion region
		if (rDeplt > edge/2)
			std::cerr<<"Warning: domain too small to accommodate particle. Push beyond "<<rDeplt<<".\n"<<std::endl;

		double delta_mass=0.0;
		double mu_mass=0.0;
		for (int n=0; n<nodes(initGrid); n++) {
			vector<int> x = position(initGrid, n);
			const double rmu = radius(originMu, x, dx(initGrid,0));
			const double rdel = radius(originDelta, x, dx(initGrid,0));
			if (rdel > rDelta && rmu > rMu) {
				// Gamma matrix:
				// x_Mo
				initGrid(n)[0] = xMo[0]*(1.0 + noise_amp*real_gen(mt_rand))
				                 + 0.25*xMo[0]*bellCurve(dx(initGrid,0)*x[0], 0,                     bell[0]*dx(initGrid,0)*edge)
				                 + 0.25*xMo[0]*bellCurve(dx(initGrid,0)*x[0], dx(initGrid,0)*edge/2, bell[0]*dx(initGrid,0)*edge)
				                 + 0.25*xMo[0]*bellCurve(dx(initGrid,0)*x[0], dx(initGrid,0)*edge,   bell[0]*dx(initGrid,0)*edge);
				// x_Nb
				initGrid(n)[1] = xNb[0]*(1.0 + noise_amp*real_gen(mt_rand))
				                 + xNb[0]*bellCurve(dx(initGrid,0)*x[0], 0,                     bell[1]*dx(initGrid,0)*edge)
				                 + xNb[0]*bellCurve(dx(initGrid,0)*x[0], dx(initGrid,0)*edge/2, bell[1]*dx(initGrid,0)*edge)
				                 + xNb[0]*bellCurve(dx(initGrid,0)*x[0], dx(initGrid,0)*edge,   bell[1]*dx(initGrid,0)*edge);
				// phi, C
				for (int i=2; i<fields(initGrid); i++)
					initGrid(n)[i] = 0.01*real_gen(mt_rand);

				if (r<rDeplt) { // point falls within the depletion region
					//double deltaxNb = xNb[0]-xNbdep - xNbdep*(r-rDelta)/(rDeplt-rDelta);
					double deltaxNb = xNbdep - xNbdep*(r-rDelta)/(rDeplt-rDelta);
					initGrid(n)[1] -= deltaxNb;
				}
			} else if (rmu <= rMu) {
				// Mu particle
				mu_mass += 1.0;
				initGrid(n)[0] = xMo[3]*(1.0 + noise_amp*real_gen(mt_rand));
				initGrid(n)[1] = xNb[3]*(1.0 + noise_amp*real_gen(mt_rand));
				initGrid(n)[2] = 1.0;
				for (int i=3; i<fields(initGrid); i++)
					initGrid(n)[i] = 0.0;
			} else {
				// Delta particle
				delta_mass += 1.0;
				initGrid(n)[0] = xMo[3]*(1.0 + noise_amp*real_gen(mt_rand));
				initGrid(n)[1] = xNb[3]*(1.0 + noise_amp*real_gen(mt_rand));
				initGrid(n)[0] = 0.0;
				initGrid(n)[3] = 1.0;
				for (int i=4; i<fields(initGrid); i++)
					initGrid(n)[i] = 0.0;
			}
		}

		// Initialize fictitious compositions
		guessGamma(initGrid(n)[0], initGrid(n)[1], initGrid(n)[4], initGrid(n)[5], initGrid(n)[6]);
		guessMu(   initGrid(n)[0], initGrid(n)[1], initGrid(n)[7], initGrid(n)[8], initGrid(n)[9]);
		guessDelta(initGrid(n)[0], initGrid(n)[1], initGrid(n)[10], initGrid(n)[11], initGrid(n)[12]);

		vector<double> totals(4, 0.0);
		for (int n=0; n<nodes(initGrid); n++) {
			for (int i=0; i<2; i++)
				totals[i] += initGrid(n)[i];
			for (int i=2; i<4; i++)
				totals[i] += std::fabs(initGrid(n)[i]);
		}

		for (int i=0; i<4; i++)
			totals[i] /= double(edge*edge);
		#ifdef MPI_VERSION
		vector<double> myTot(totals);
		for (int i=0; i<4; i++) {
			MPI::COMM_WORLD.Reduce(&myTot[i], &totals[i], 1, MPI_DOUBLE, MPI_SUM, 0);
			MPI::COMM_WORLD.Barrier();
		}
		#endif
		if (rank==0) {
			std::cout<<"x_Mo      x_Nb      x_Ni\n";
			printf("%.6f  %1.2e  %1.2e\n\n", totals[0], totals[1], 1.0-totals[0]-totals[1]);
			std::cout<<"p_g       p_m      p_d\n";
			printf("%.6f  %1.2e  %1.2e\n", 1.0-totals[2]-totals[3], totals[2], totals[3]);
		}

		output(initGrid,filename);
	} else {
		std::cerr<<"Error: "<<dim<<"-dimensional grids unsupported."<<std::endl;
	}

}

template <int dim, typename T> void update(grid<dim,vector<T> >& oldGrid, int steps)
{
	int rank=0;
	#ifdef MPI_VERSION
	rank = MPI::COMM_WORLD.Get_rank();
	#endif

	// Construct the common tangent solver
	rootsolver CommonTangentSolver;

	ghostswap(oldGrid);
	grid<dim,vector<T> > newGrid(oldGrid);
	grid<dim,vector<T> > chemGrid(oldGrid, 2); // storage for chemical potentials
	double dV=1.0;
	for (int d=0; d<dim; d++) {
		dx(oldGrid,d) = meshres;
		dx(newGrid,d) = meshres;
		dx(chemGrid,d) = meshres;
		dV *= dx(oldGrid,d);
		if (useNeumann && x0(oldGrid,d) == g0(oldGrid,d)) {
			b0(oldGrid,d) = Neumann;
			b0(newGrid,d) = Neumann;
			b0(chemGrid,d) = Neumann;
		} else if (useNeumann && x1(oldGrid,d) == g1(oldGrid,d)) {
			b1(oldGrid,d) = Neumann;
			b1(newGrid,d) = Neumann;
			b1(chemGrid,d) = Neumann;
		}
	}

	std::ofstream cfile;
	if (rank==0)
		cfile.open("c.log",std::ofstream::out | std::ofstream::app);

	for (int step=0; step<steps; step++) {
		if (rank==0)
			print_progress(step, steps);

		#ifndef MPI_VERSION
		#pragma omp parallel for
		#endif
		for (int n=0; n<nodes(oldGrid); n++) {
			vector<int> x = position(oldGrid,n);

			const T& C_gam_Mo  = oldGrid(n)[4];
			const T& C_gam_Nb  = oldGrid(n)[5];
			const T& C_gam_Ni  = oldGrid(n)[6];

			chemGrid(x)[0] = dg_gam_dxMo(C_gam_Mo, C_gam_Nb, C_gam_Ni);
			chemGrid(x)[1] = dg_gam_dxNb(C_gam_Mo, C_gam_Nb, C_gam_Ni);
		}

		ghostswap(chemGrid);

		double ctot=0.0, ftot=0.0, utot=0.0, vmax=0.0;
		#ifndef MPI_VERSION
		#pragma omp parallel for
		#endif
		for (int n=0; n<nodes(oldGrid); n++) {
			/* ============================================== *
			 * Point-wise kernel for parallel PDE integration *
			 * ============================================== */

			vector<int> x = position(oldGrid,n);

			/* Representation includes thirteen field variables:
		 	 *
	 		 * X0.  molar fraction of Al, Cr, Mo
		 	 * X1.  molar fraction of Nb, Fe
		 	 * bal. molar fraction of Ni
		 	 *
 			 * P2.  phase fraction of mu
 			 * P3.  phase fraction of delta
 			 * bal. phase fraction of gamma
			 *
			 * C4. fictitious Mo concentration in gamma
			 * C5. fictitious Nb concentration in gamma
			 * C6. fictitious Ni concentration in gamma
			 *
			 * C7. fictitious Mo concentration in mu
			 * C8. fictitious Nb concentration in mu
			 * C9. fictitious Ni concentration in mu
			 *
			 * C10. fictitious Mo concentration in delta
			 * C11. fictitious Nb concentration in delta
			 * C12. fictitious Ni concentration in delta
			 */

			// Cache some frequently-used reference values
			const T& phi_mu = oldGrid(n)[2];
			const T& phi_del = oldGrid(n)[3];

			const T& x_Mo   = oldGrid(n)[0];
			const T& x_Nb   = oldGrid(n)[1];

			const T& C_gam_Mo  = oldGrid(n)[4];
			const T& C_gam_Nb  = oldGrid(n)[5];
			const T& C_gam_Ni  = oldGrid(n)[6];

			const T& C_mu_Mo  = oldGrid(n)[7];
			const T& C_mu_Nb  = oldGrid(n)[8];
			const T& C_mu_Ni  = oldGrid(n)[9];

			const T& C_del_Mo  = oldGrid(n)[10];
			const T& C_del_Nb  = oldGrid(n)[11];
			const T& C_del_Ni  = oldGrid(n)[12];


			/* ============================================= *
			 * Solve the Equation of Motion for Compositions *
			 * ============================================= */

			double gradSqPot_Mo = laplacian(chemGrid, x, 0);
			newGrid(n)[0] = x_Mo + dt * VmSq * M_Mo * gradSqPot_Mo;

			double gradSqPot_Nb = laplacian(chemGrid, x, 1);
			newGrid(n)[1] = x_Nb + dt * VmSq * M_Nb * gradSqPot_Nb;


			/* ======================================== *
			 * Solve the Equation of Motion for Phases  *
			 * ======================================== */

			double df_dphi_mu = sign(phi_mu) * (-hprime(fabs(phi_mu))*g_gam(C_gam_Mo, C_gam_Nb, C_gam_Ni)
			                                    + hprime(fabs(phi_mu))*g_mu(C_mu_Mo, C_mu_Nb, C_mu_Ni)
			                                    - 2.0*omega_mu*phi_mu*phi_mu*(1.0-fabs(phi_mu))
			                                   )
			                    + 2.0*omega_mu*phi_mu*pow(1.0-fabs(phi_mu),2)
			                    + alpha*fabs(phi_mu)*phi_del*phi*del;

			double gradSqPhi_mu = laplacian(oldGrid, x, 2);

			newGrid(n)[2] = phi_mu + dt * L_mu * (eps_sq*gradSqPhi_mu - df_dphi_mu);

			double df_dphi_del = sign(phi_del) * (-hprime(fabs(phi_del))*g_gam(C_gam_Mo, C_gam_Nb, C_gam_Ni)
			                                    + hprime(fabs(phi_del))*g_del(C_del_Mo, C_del_Nb, C_del_Ni)
			                                    - 2.0*omega_del*phi_del*phi_del*(1.0-fabs(phi_del))
			                                   )
			                    + 2.0*omega_del*phi_del*pow(1.0-fabs(phi_del),2)
			                    + alpha*fabs(phi_del)*phi_mu*phi*mu;

			double gradSqPhi_del = laplacian(oldGrid, x, 3);

			newGrid(n)[3] = phi_del + dt * L_del * (eps_sq*gradSqPhi_del - df_dphi_del);


			/* ============================== *
			 * Solve for common tangent plane *
			 * ============================== */

			guessGamma(newGrid(n)[0], newGrid(n)[1], newGrid(n)[4], newGrid(n)[5], newGrid(n)[6]);
			guessMu(   newGrid(n)[0], newGrid(n)[1], newGrid(n)[7], newGrid(n)[8], newGrid(n)[9]);
			guessDelta(newGrid(n)[0], newGrid(n)[1], newGrid(n)[10], newGrid(n)[11], newGrid(n)[12]);

			CommonTangentSolver.solve(newGrid(n)[0], newGrid(n)[1],
			                          newGrid(n)[2], newGrid(n)[3],
			                          newGrid(n)[4], newGrid(n)[5], newGrid(n)[6],
			                          newGrid(n)[7], newGrid(n)[8], newGrid(n)[9],
			                          newGrid(n)[10], newGrid(n)[11], newGrid(n)[12]);


			/* ====================================================================== *
			 * Collate summary & diagnostic data in OpenMP- and MPI-compatible manner *
			 * ====================================================================== */

			double myc = dV*newGrid(n)[1];
			double myf = dV*(0.5*eps_sq*gradPsq + f(newGrid(n)[0], newGrid(n)[1], newGrid(n)[2], newGrid(n)[3]));
			double myv = 0.0;
			if (newGrid(n)[0]>0.3 && newGrid(n)[0]<0.7) {
				gradPsq = 0.0;
				for (int d=0; d<dim; d++) {
					double weight = 1.0/pow(dx(newGrid,d), 2.0);
					s[d] -= 1;
					const T& pl = newGrid(s)[0];
					s[d] += 2;
					const T& ph = newGrid(s)[0];
					s[d] -= 1;
					gradPsq  += weight * pow(0.5*(ph-pl), 2.0);
				}
				myv = (newGrid(n)[0] - phi_old) / (dt * std::sqrt(gradPsq));
			}
			double myu = (fl(newGrid(n)[3])-fs(newGrid(n)[2]))/(Cle - Cse);

			#ifndef MPI_VERSION
			#pragma omp critical
			{
			#endif
				vmax = std::max(vmax,myv); // maximum velocity
				ctot += myc;               // total mass
				ftot += myf;               // total free energy
				utot += myu*myu;           // deviation from equilibrium
				#ifndef MPI_VERSION
			}
				#endif

			/* ======= *
			 * ~ fin ~ *
			 * ======= */
		}
		swap(oldGrid,newGrid);
		ghostswap(oldGrid);

		double ntot(nodes(oldGrid));
		#ifdef MPI_VERSION
		double myvm(vmax);
		double myct(ctot);
		double myft(ftot);
		double myut(utot);
		double myn(ntot);
		MPI::COMM_WORLD.Allreduce(&myct, &ctot, 1, MPI_DOUBLE, MPI_SUM);
		MPI::COMM_WORLD.Allreduce(&myft, &ftot, 1, MPI_DOUBLE, MPI_SUM);
		MPI::COMM_WORLD.Allreduce(&myvm, &vmax, 1, MPI_DOUBLE, MPI_MAX);
		MPI::COMM_WORLD.Allreduce(&myut, &utot, 1, MPI_DOUBLE, MPI_SUM);
		MPI::COMM_WORLD.Allreduce(&myn,  &ntot, 1, MPI_DOUBLE, MPI_SUM);
		#endif
		double CFLmax = (vmax * dt) / meshres;
		utot = std::sqrt(utot/ntot);
		if (rank==0)
			cfile<<ctot<<'\t'<<ftot<<'\t'<<CFLmax<<'\t'<<utot<<std::endl;
	}
	if (rank==0)
		cfile.close();

	print_values(oldGrid, rank);
}


} // namespace MMSP

template<int dim, typename T>
void print_values(const MMSP::grid<dim,MMSP::vector<T> >& oldGrid, const int rank)
{
	double pTot=0.0;
	double cTot=0.0;
	unsigned int nTot = nodes(oldGrid);
	for (int n=0; n<nodes(oldGrid); n++) {
		pTot += oldGrid(n)[0];
		cTot += oldGrid(n)[1];
	}

	#ifdef MPI_VERSION
	double myP(pTot), myC(cTot);
	unsigned int myN(nTot);
	MPI::COMM_WORLD.Allreduce(&myP, &pTot, 1, MPI_DOUBLE, MPI_SUM);
	MPI::COMM_WORLD.Allreduce(&myC, &cTot, 1, MPI_DOUBLE, MPI_SUM);
	MPI::COMM_WORLD.Allreduce(&myN, &nTot, 1, MPI_UNSIGNED, MPI_SUM);
	#endif
	cTot /= nTot;
	double wps = (100.0*pTot)/nTot;
	double wpl = (100.0*(nTot-pTot))/nTot;
	double fs = 100.0*(cTot - Cle)/(Cse-Cle);
	double fl = 100.0*(Cse - cTot)/(Cse-Cle);
	if (rank==0)
		printf("System has %.2f%% solid, %.2f%% liquid, and composition %.2f%% B. Equilibrium is %.2f%% solid, %.2f%% liquid.\n",
		       wps, wpl, 100.0*cTot, fs, fl);
}


double h(const double p)
{
	return p;
}

double hprime(const double p)
{
	return 1.0;
}

double g(const double p)
{
	return pow(p,2.0) * pow(1.0-p,2.0);
}

double gprime(const double p)
{
	return 2.0*p * (1.0-p)*(1.0-2.0*p);
}

double gibbs(const vector<double>& v)
{
	double g  = g_gam(v[0],v[1]) * (1.0 - (h(abs(v[2])) + h(abs(v[3])) + h(abs(v[4]))));
	       g += g_mu(v[0],v[1]) * h(abs(v[2]));
	       g += g_del(v[0],v[1]) * h(abs(v[3]));
	       g += w_mu * v[2]*v[2] * (1.0 - abs(v[2])*(1.0 - abs(v[2]);
	       g += w_del * v[3]*v[3] * (1.0 - abs(v[3])*(1.0 - abs(v[3]);
	for (int i=2; i<v.length(); i++)
		for (int j=i+1; j<v.length(); j++)
			g += 2.0 * alpha * v[i]*v[i] * v[j]*v[j];

	return g;
}

void simple_progress(int step, int steps)
{
	if (step==0)
		std::cout<<" ["<<std::flush;
	else if (step==steps-1)
		std::cout<<"•] "<<std::endl;
	else if (step % (steps/20) == 0)
		std::cout<<"• "<<std::flush;
}




/* ====================================== *
 * Invoke GSL to solve for common tangent *
 * ====================================== */

/* Given const phase fraction (p) and concentration (c), iteratively determine
 * the solid (Cs) and liquid (Cl) fictitious concentrations that satisfy the
 * equal chemical potential constraint. Pass p and c by const value,
 * Cs and Cl by non-const reference to update in place. This allows use of this
 * single function to both populate the LUT and interpolate values based thereupon.
 */

struct rparams {
	// Composition fields
	double x_Mo;
	double x_Nb;

	// Structure fields
	double p_mu_Mo;
	double p_del_Mo;

	double p_mu_Nb;
	double p_del_Nb;
};


int commonTangent_f(const gsl_vector* x, void* params, gsl_vector* f)
{
	// Prepare constants
	const double x_Mo = ((struct rparams*) params)->x_Mo;
	const double x_Nb = ((struct rparams*) params)->x_Nb;
	const double p_mu = ((struct rparams*) params)->p_mu;
	const double p_del = ((struct rparams*) params)->p_del;

	const double x_Ni = 1.0 - x_Mo - x_Nb;
	const double n_gam = 1.0 - h(fabs(p_mu)) - h(fabs(p_del));
	const double n_mu  = h(fabs(p_mu));
	const double n_del = h(fabs(p_del));

	// Prepare variables
	const double C_gam_Mo = gsl_vector_get(x, 0);
	const double C_mu_Mo  = gsl_vector_get(x, 1);
	const double C_del_Mo = gsl_vector_get(x, 2);

	const double C_gam_Nb = gsl_vector_get(x, 3);
	const double C_mu_Nb  = gsl_vector_get(x, 4);
	const double C_del_Nb = gsl_vector_get(x, 5);

	for (int i=0; i<f.size; i++)
		gsl_vector_set(f, i, 0.0);

	gsl_vector_set(f, 0, x_Mo - n_gam*C_gam_Mo - n_mu*C_mu_Mo - n_del*C_del_Mo);
	gsl_vector_set(f, 1, x_Nb - n_gam*C_gam_Nb - n_mu*C_mu_Nb - n_del*C_del_Nb;
	gsl_vector_set(f, 2, x_Ni - n_gam*C_gam_Ni - n_mu*C_mu_Ni - n_del*C_del_Ni;

	gsl_vector_set(f, 3, dg_gam_dxMo(C_gam_Mo) - dg_mu_dxMo(C_mu_Mo));
	gsl_vector_set(f, 4, dg_gam_dxNb(C_gam_Nb) - dg_mu_dxNb(C_mu_Nb));
	gsl_vector_set(f, 5, dg_gam_dxNi(C_gam_Ni) - dg_mu_dxNi(C_mu_Ni));

	gsl_vector_set(f, 6, dg_mu_dxMo(C_mu_Mo) - dg_del_dxMo(C_del_Mo));
	gsl_vector_set(f, 7, dg_mu_dxNb(C_mu_Nb) - dg_del_dxNb(C_del_Nb));
	gsl_vector_set(f, 8, dg_mu_dxNi(C_mu_Ni) - dg_del_dxNi(C_del_Ni));

	return GSL_SUCCESS;
}


int commonTangent_df(const gsl_vector* x, void* params, gsl_matrix* J)
{
	// Prepare constants
	const double x_Mo = ((struct rparams*) params)->x_Mo;
	const double x_Nb = ((struct rparams*) params)->x_Nb;
	const double p_mu = ((struct rparams*) params)->p_mu;
	const double p_del = ((struct rparams*) params)->p_del;

	const double x_Ni = 1.0 - x_Mo - x_Nb;
	const double n_gam = 1.0 - h(fabs(p_mu)) - h(fabs(p_del);
	const double n_mu  = h(fabs(p_mu));
	const double n_del = h(fabs(p_del);

	// Prepare variables
	const double C_gam_Mo = gsl_vector_get(x, 0);
	const double C_mu_Mo  = gsl_vector_get(x, 1);
	const double C_del_Mo = gsl_vector_get(x, 2);

	const double C_gam_Nb = gsl_vector_get(x, 3);
	const double C_mu_Nb  = gsl_vector_get(x, 4);
	const double C_del_Nb = gsl_vector_get(x, 5);

	// Jacobian matrix

	for (int i=0; i<J.size1; i++)
		for (int j=0; j<J.size2; j++)
			gsl_matrix_set(J, i, j, 0.0);

	// Conservation of mass (Mo, Nb, Ni)
	gsl_matrix_set(J, 0, 0, -n_gam);
	gsl_matrix_set(J, 1, 1, -n_gam);
	gsl_matrix_set(J, 2, 2, -n_gam);

	gsl_matrix_set(J, 0, 3, -n_mu);
	gsl_matrix_set(J, 1, 4, -n_mu);
	gsl_matrix_set(J, 2, 5, -n_mu);

	gsl_matrix_set(J, 0, 6, -n_del);
	gsl_matrix_set(J, 1, 7, -n_del);
	gsl_matrix_set(J, 2, 8, -n_del);

	// Equal chemical potential in gamma phase (Mo, Nb, Ni)
	gsl_matrix_set(J, 3, 0,  d2f_gam_dxMoMo(C_gam_Mo, C_gam_Nb, C_gam_Ni));
	gsl_matrix_set(J, 4, 0,  d2f_gam_dxNbMo(C_gam_Mo, C_gam_Nb, C_gam_Ni));
	gsl_matrix_set(J, 5, 0,  d2f_gam_dxNiMo(C_gam_Mo, C_gam_Nb, C_gam_Ni));

	gsl_matrix_set(J, 3, 1,  d2f_gam_dxMoNb(C_gam_Mo, C_gam_Nb, C_gam_Ni));
	gsl_matrix_set(J, 4, 1,  d2f_gam_dxNbNb(C_gam_Mo, C_gam_Nb, C_gam_Ni));
	gsl_matrix_set(J, 5, 1,  d2f_gam_dxNiNb(C_gam_Mo, C_gam_Nb, C_gam_Ni));

	gsl_matrix_set(J, 3, 2,  d2f_gam_dxMoNi(C_gam_Mo, C_gam_Nb, C_gam_Ni));
	gsl_matrix_set(J, 4, 2,  d2f_gam_dxNbNi(C_gam_Mo, C_gam_Nb, C_gam_Ni));
	gsl_matrix_set(J, 5, 2,  d2f_gam_dxNiNi(C_gam_Mo, C_gam_Nb, C_gam_Ni));


	// Equal chemical potential in mu phase (Mo, Nb, Ni)
	const double J33 = -d2f_mu_dxMoMo(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	const double J43 = -d2f_mu_dxNbMo(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	const double J53 = -d2f_mu_dxNiMo(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	gsl_matrix_set(J, 3, 3,  J33);
	gsl_matrix_set(J, 4, 3,  J43);
	gsl_matrix_set(J, 5, 3,  J53);

	gsl_matrix_set(J, 6, 3, -J33);
	gsl_matrix_set(J, 7, 3, -J43);
	gsl_matrix_set(J, 8, 3, -J53);

	const double J34 = -d2f_mu_dxNbMo(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	const double J44 = -d2f_mu_dxNbNb(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	const double J54 = -d2f_mu_dxNbNi(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	gsl_matrix_set(J, 3, 4,  J34);
	gsl_matrix_set(J, 4, 4,  J44);
	gsl_matrix_set(J, 5, 4,  J54);

	gsl_matrix_set(J, 6, 4, -J34);
	gsl_matrix_set(J, 7, 4, -J44);
	gsl_matrix_set(J, 8, 4, -J54);

	const double J35 = -d2f_mu_dxNiMo(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	const double J45 = -d2f_mu_dxNiNb(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	const double J55 = -d2f_mu_dxNiNi(C_mu_Mo, C_mu_Nb, C_mu_Ni);
	gsl_matrix_set(J, 3, 5,  J35);
	gsl_matrix_set(J, 4, 5,  J45);
	gsl_matrix_set(J, 5, 5,  J55);

	gsl_matrix_set(J, 6, 5, -J35);
	gsl_matrix_set(J, 7, 5, -J45);
	gsl_matrix_set(J, 8, 5, -J55);


	// Equal chemical potential in delta phase (Mo, Nb, Ni)
	gsl_matrix_set(J, 6, 6,  d2f_del_dxMoMo(C_del_Mo, C_del_Nb, C_del_Ni));
	gsl_matrix_set(J, 7, 6,  d2f_del_dxNbMo(C_del_Mo, C_del_Nb, C_del_Ni));
	gsl_matrix_set(J, 8, 6,  d2f_del_dxNiMo(C_del_Mo, C_del_Nb, C_del_Ni));

	gsl_matrix_set(J, 6, 7,  d2f_del_dxMoNb(C_del_Mo, C_del_Nb, C_del_Ni));
	gsl_matrix_set(J, 7, 7,  d2f_del_dxNbNb(C_del_Mo, C_del_Nb, C_del_Ni));
	gsl_matrix_set(J, 8, 7,  d2f_del_dxNiNb(C_del_Mo, C_del_Nb, C_del_Ni));

	gsl_matrix_set(J, 6, 8,  d2f_del_dxMoNi(C_del_Mo, C_del_Nb, C_del_Ni));
	gsl_matrix_set(J, 7, 8,  d2f_del_dxNbNi(C_del_Mo, C_del_Nb, C_del_Ni));
	gsl_matrix_set(J, 8, 8,  d2f_del_dxNiNi(C_del_Mo, C_del_Nb, C_del_Ni));

	return GSL_SUCCESS;
}


int commonTangent_fdf(const gsl_vector* x, void* params, gsl_vector* f, gsl_matrix* J)
{
	commonTangent_f(x, params, f);
	commonTangent_df(x, params, J);

	return GSL_SUCCESS;
}


rootsolver::rootsolver() :
	n(9), // eight equations
	maxiter(5000),
	tolerance(1.0e-10)
{
	x = gsl_vector_alloc(n);

	// configure algorithm
	algorithm = gsl_multiroot_fdfsolver_gnewton; // gnewton, hybridj, hybridsj, newton
	solver = gsl_multiroot_fdfsolver_alloc(algorithm, n);

	mrf = {&commonTangent_f, &commonTangent_df, &commonTangent_fdf, n, &par};
}

template <typename T> double
rootsolver::solve(const T& x_Mo, const T& x_Nb, const T& p_mu, const T& p_del,
                  T& C_gam_Mo, T& C_mu_Mo, T& C_del_Mo,
                  T& C_gam_Nb, T& C_mu_Nb, T& C_del_Nb,
                  T& C_gam_Ni, T& C_mu_Ni, T& C_del_Ni)
{
	int status;
	size_t iter = 0;

	// initial guesses
	par.x_Mo = x_Mo;
	par.x_Nb = x_Nb;
	par.p_mu = p_mu;
	par.p_del = p_del;

	gsl_vector_set(x, 0, C_gam_Mo);
	gsl_vector_set(x, 1, C_mu_Mo);
	gsl_vector_set(x, 2, C_del_Mo);

	gsl_vector_set(x, 3, C_gam_Nb);
	gsl_vector_set(x, 4, C_mu_Nb);
	gsl_vector_set(x, 5, C_del_Nb);

	gsl_vector_set(x, 6, C_gam_Ni);
	gsl_vector_set(x, 7, C_mu_Ni);
	gsl_vector_set(x, 8, C_del_Ni);

	gsl_multiroot_fdfsolver_set(solver, &mrf, x);

	do {
		iter++;
		status = gsl_multiroot_fdfsolver_iterate(solver);
		if (status) // extra points for finishing early!
			break;
		status = gsl_multiroot_test_residual(solver->f, tolerance);
	} while (status==GSL_CONTINUE && iter<maxiter);

	C_gam_Mo = static_cast<T>(gsl_vector_get(solver->x, 0));
	C_mu_Mo  = static_cast<T>(gsl_vector_get(solver->x, 1));
	C_del_Mo = static_cast<T>(gsl_vector_get(solver->x, 2));

	C_gam_Nb = static_cast<T>(gsl_vector_get(solver->x, 3));
	C_mu_Nb  = static_cast<T>(gsl_vector_get(solver->x, 4));
	C_del_Nb = static_cast<T>(gsl_vector_get(solver->x, 5));

	double residual = gsl_blas_dnrm2(solver->f);

	return residual;
}

rootsolver::~rootsolver()
{
	gsl_multiroot_fdfsolver_free(solver);
	gsl_vector_free(x);
}


#endif

#include"MMSP.main.hpp"
