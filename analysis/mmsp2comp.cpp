/**
 \file mmsp2comp.cpp
 \brief Extract composition pathway fr4om MMSP grid
 Input: MMSP grid containing vector data with at least two fields
 Output: Pairs of comma-delimited coordinates representing position in (v0,v1)
         phase space. Expected usage is for composition spaces, hence comp.
*/

#include <vector>
#include <algorithm>
#include "MMSP.hpp"
#include "parabola625.c"

#define NC 2
#define NP 2

/* Representation includes ten field variables:
 *
 * X0. molar fraction of Cr + Mo
 * X1. molar fraction of Nb
 *
 * P2. phase fraction of delta
 * P3. phase fraction of Laves
 *
 * C4. Cr molar fraction in pure gamma
 * C5. Nb molar fraction in pure gamma
 *
 * C6. Cr molar fraction in pure delta
 * C7. Nb molar fraction in pure delta
 *
 * C8. Cr molar fraction in pure Laves
 * C9. Nb molar fraction in pure Laves
 */

template<int dim, typename T>
void vectorComp(const MMSP::grid<dim,MMSP::vector<T> >& GRID, std::vector<double>& idx, std::vector<double>& xcr, std::vector<double>& xnb, std::vector<double>& P)
{
	// Perform a line-scan parallel to the x-axis through the center of the grid
	MMSP::vector<int> x(dim, 0);
	double dV = 1.0;
	for (int d=0; d<dim; d++) {
		x[d] = (MMSP::g1(GRID,d)+MMSP::g0(GRID,d))/2;
		dV *= dx(GRID,d);
	}

	const int d = 8 + (MMSP::g1(GRID, 0) - MMSP::g0(GRID, 0))/2;

	// for (x[0] = MMSP::g0(GRID,0); x[0] < MMSP::g1(GRID,0); x[0]++) {
	for (x[0] = -d/2; x[0] < d/2; x[0]++) {
		if (1) {
			// Record position
			idx.push_back(dx(GRID) * x[0]);

			// Record system compositions
			xcr.push_back(GRID(x)[0]);
			xnb.push_back(GRID(x)[1]);

			// Compute diffusion potential in matrix
			const double chempot[NC] = {dg_gam_dxCr(GRID(x)[NC+NP], GRID(x)[NC+NP+1]),
			                            dg_gam_dxNb(GRID(x)[NC+NP], GRID(x)[NC+NP+1])
			                           };

			double PhaseEnergy[NP+1] = {g_del(GRID(x)[2*NC+NP], GRID(x)[2*NC+NP+1]),
			                            g_lav(GRID(x)[3*NC+NP], GRID(x)[3*NC+NP+1]),
			                            g_gam(GRID(x)[  NC+NP], GRID(x)[  NC+NP+1]) // matrix phase last
			                           };

			// Record driving force for phase transformation
			double Pressure[NP] = {0.0};
			for (int j = 0; j < NP; j++) {
				Pressure[j] += PhaseEnergy[NP] - PhaseEnergy[j];
				for (int i = 0; i < NC; i++)
					Pressure[j] -= (GRID(x)[NC+NP+i] - GRID(x)[NC+NP+i+NC*(j+1)]) * chempot[i];
			}
			P.push_back(dV * (*std::max_element(Pressure, Pressure+NP)));
		}
	}
}

int main(int argc, char* argv[])
{
	// command line error check
	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " [--help] infile [outfile]\n\n";
		exit(-1);
	}

	// help diagnostic
	if (std::string(argv[1]) == "--help") {
		std::cout << argv[0] << ": convert MMSP grid data to (p,c) points.\n";
		std::cout << "Usage: " << argv[0] << " [--help] infile [outfile]\n\n";
		std::cout << "Questions/comments to trevor.keller@nist.gov (Trevor Keller).\n\n";
		exit(0);
	}

	// file open error check
	std::ifstream input(argv[1]);
	if (!input) {
		std::cerr << "File input error: could not open " << argv[1] << ".\n\n";
		exit(-1);
	}

	// generate output file name
	std::stringstream filename;
	if (argc < 3)
		filename << std::string(argv[1]).substr(0, std::string(argv[1]).find_last_of(".")) << ".xy";
	else
		filename << argv[2];

	// file open error check
	std::ofstream output(filename.str().c_str());
	if (!output) {
		std::cerr << "File output error: could not open ";
		std::cerr << filename.str() << "." << std::endl;
		exit(-1);
	}

	// read data type
	std::string type;
	getline(input, type, '\n');

	// grid type error check
	if (type.substr(0, 4) != "grid") {
		std::cerr << "File input error: file does not contain grid data." << std::endl;
		exit(-1);
	}

	// data type error check
	bool vector_type = (type.find("vector") != std::string::npos);
	if (not vector_type) {
		std::cerr << "File input error: grid does not contain vector data." << std::endl;
		exit(-1);
	}


	// parse data type
	bool bool_type = (type.find("bool") != std::string::npos);
	bool char_type = (type.find("char") != std::string::npos);
	bool unsigned_char_type = (type.find("unsigned char") != std::string::npos);
	bool int_type = (type.find("int") != std::string::npos);
	bool unsigned_int_type = (type.find("unsigned int") != std::string::npos);
	bool long_type = (type.find("long") != std::string::npos);
	bool unsigned_long_type = (type.find("unsigned long") != std::string::npos);
	bool short_type = (type.find("short") != std::string::npos);
	bool unsigned_short_type = (type.find("unsigned short") != std::string::npos);
	bool float_type = (type.find("float") != std::string::npos);
	bool double_type = (type.find("double") != std::string::npos);
	bool long_double_type = (type.find("long double") != std::string::npos);


	if (not bool_type    and
	    not char_type    and  not unsigned_char_type   and
	    not int_type     and  not unsigned_int_type    and
	    not long_type    and  not unsigned_long_type   and
	    not short_type   and  not unsigned_short_type  and
	    not float_type   and
	    not double_type  and  not long_double_type) {
		std::cerr << "File input error: unknown grid data type." << std::endl;
		exit(-1);
	}

	// read grid dimension
	int dim;
	input >> dim;

	// read number of fields
	int fields;
	input >> fields;

	std::vector<double> idx;
	std::vector<double> xcr;
	std::vector<double> xnb;
	std::vector<double> P;

	// write grid data
	if (vector_type && fields>1) {
		if (float_type) {
			if (dim == 1) {
				MMSP::grid<1, MMSP::vector<float> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			} else if (dim == 2) {
				MMSP::grid<2, MMSP::vector<float> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			} else if (dim == 3) {
				MMSP::grid<3, MMSP::vector<float> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			}
		}
		if (double_type) {
			if (dim == 1) {
				MMSP::grid<1, MMSP::vector<double> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			} else if (dim == 2) {
				MMSP::grid<2, MMSP::vector<double> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			} else if (dim == 3) {
				MMSP::grid<3, MMSP::vector<double> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			}
		}
		if (long_double_type) {
			if (dim == 1) {
				MMSP::grid<1, MMSP::vector<long double> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			} else if (dim == 2) {
				MMSP::grid<2, MMSP::vector<long double> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			} else if (dim == 3) {
				MMSP::grid<3, MMSP::vector<long double> > GRID(argv[1]);
				vectorComp(GRID, idx, xcr, xnb, P);
			}
		}
	}

	for (unsigned int i=0; i<xcr.size(); i++)
		output << idx[i] << ',' << xcr[i] << ',' << xnb[i] << ',' << P[i] << '\n';

	output.close();

	return 0;
}
