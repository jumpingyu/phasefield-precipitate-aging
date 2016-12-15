/*************************************************************************************
 * File: alloy625.hpp                                                                *
 * Declarations for 2D and 3D isotropic Cr-Nb-Ni alloy phase transformations         *
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
 * versions bear some notice that they have been modified.                           *
 *************************************************************************************/

// Number of phases and components (for array allocation)
#define NP 4
#define NC 2

std::string PROGRAM = "alloy625";
std::string MESSAGE = "Isotropic Cr-Nb-Ni alloy phase transformation code";

typedef MMSP::grid<1,MMSP::vector<double> > GRID1D;
typedef MMSP::grid<2,MMSP::vector<double> > GRID2D;
typedef MMSP::grid<3,MMSP::vector<double> > GRID3D;


/* =========================== Composition Class =========================== */

// Trivial class to hold system composition and phase fractions
class Composition {
public:
	Composition()
	{
		for (int i=0; i<NP; i++) {
			for (int j=0; j<NC; j++)
				x[i][j] = 0.0;
		}
		for (int i=0; i<NP; i++)
			N[i] = 0;
	}
	~Composition(){}
	Composition& operator+=(const Composition& c);
	double x[NP][NC]; // composition of each phase
	int    N[NP];     // amount of each phase
};

Composition& Composition::operator+=(const Composition& c)
{
	for (int i=0; i<NP; i++) {
		for (int j=0; j<NC; j++)
		this->x[i][j] += c.x[i][j];
	}
	for (int i=0; i<NP; i++)
		this->N[i] += c.N[i];

	return *this;
}
/* ========================================================================= */



/* ============================ rootsolver Class =========================== */

/* Given const phase fractions (in gamma, mu, and delta) and concentrations of
 * Cr and Nb, iteratively determine the fictitious concentrations in each phase
 * that satisfy the constraints of equal chemical potential at equilibrium and
 * conservation of mass at all times. Pass constants (p and x) by const value,
 * C by non-const reference to update in place.
 */

// Struct to hold parallel tangent solver constants
struct rparams {
	// Composition fields
	double x_Cr;
	double x_Nb;

	// Structure fields
	double n_del;
	double n_mu;
	double n_lav;
};

class rootsolver
{
public:
	rootsolver();
	~rootsolver();
	template<int dim,typename T>
	double solve(MMSP::grid<dim,MMSP::vector<T> >& GRID, int n);

private:
	const size_t n;
	const size_t maxiter;
	const double tolerance;
	gsl_vector* x;
	struct rparams par;
	const gsl_multiroot_fdfsolver_type* algorithm;
	gsl_multiroot_fdfsolver* solver;
	gsl_multiroot_function_fdf mrf;
};
/* ========================================================================= */



// Interface interpolation function and derivatives
double h(const double p);

double hprime(const double p);

double sign(const double& x) {return (x<0) ? -1.0 : 1.0;}


// Gibbs free energy density
double gibbs(const MMSP::vector<double>& v);


// Geometric helpers for initial conditions
double radius(const MMSP::vector<int>& a, const MMSP::vector<int>& b, const double& dx);

double bellCurve(double x, double m, double s);


// Guess values for parallel tangent solver: gamma, mu, and delta equilibrium compositions

template<int dim,typename T>
void guessGamma(MMSP::grid<dim,MMSP::vector<T> >& GRID, int n, std::mt19937_64& mt_rand, std::uniform_real_distribution<T>& real_gen, const T& amp);

template<int dim,typename T>
void guessDelta(MMSP::grid<dim,MMSP::vector<T> >& GRID, int n, std::mt19937_64& mt_rand, std::uniform_real_distribution<T>& real_gen, const T& amp);

template<int dim,typename T>
void guessMu(MMSP::grid<dim,MMSP::vector<T> >& GRID, int n, std::mt19937_64& mt_rand, std::uniform_real_distribution<T>& real_gen, const T& amp);

template<int dim,typename T>
void guessLaves(MMSP::grid<dim,MMSP::vector<T> >& GRID, int n, std::mt19937_64& mt_rand, std::uniform_real_distribution<T>& real_gen, const T& amp);


// Cookie cutter functions to insert features into initial condition
template<int dim,typename T>
Composition enrichMatrix(MMSP::grid<dim,MMSP::vector<T> >& GRID, const double bellCr, const double bellNb);

template<typename T>
Composition embedParticle(MMSP::grid<2,MMSP::vector<T> >& GRID,
                          const MMSP::vector<int>& origin,
                          const int pid,
                          const double rprcp,
                          const T& xCr, const T& xNb,
                          const T phi);

template<typename T>
Composition embedStripe(MMSP::grid<2,MMSP::vector<T> >& GRID,
                        const MMSP::vector<int>& origin,
                        const int pid,
                        const double rprcp,
                        const T& xCr, const T& xNb,
                        const T phi);


template<int dim,class T>
MMSP::vector<double> summarize(const MMSP::grid<dim, MMSP::vector<T> >& GRID);

