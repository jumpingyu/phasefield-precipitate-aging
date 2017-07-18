// mmsp2frac.cpp
// INPUT: MMSP grid containing vector data with at least two fields
// OUTPUT: comma-separated values representing phase fraction
// Questions/comments to trevor.keller@nist.gov (Trevor Keller)

#include"MMSP.hpp"
#include<vector>
#include<algorithm>

#define NC 2
#define NP 2

template<class T>
double h(const T& p)
{
	return p * p * p * (6.0 * p * p - 15.0 * p + 10.0);
}

template<int dim, typename T>
void vectorFrac(const MMSP::grid<dim,MMSP::vector<T> >& GRID, MMSP::vector<double>& f)
{
	double dV = 1.0;
	for (int d=0; d<dim; d++)
		dV *= dx(GRID,d);

	for (int n = 0; n < MMSP::nodes(GRID); n++) {
		f[NP] += dV;
		for (int i = NC; i < NC + NP; i++) {
			// Update phase fractions
			f[i - NC] += dV * h(GRID(n)[i]);
			f[NP] -= dV * h(GRID(n)[i]);
		}
	}
}

int main(int argc, char* argv[]) {
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

	// parse timestamp
	std::string filename(argv[1]);
	size_t lastdot = filename.find_last_of(".");
	size_t firstdot = filename.find_last_of(".", lastdot - 1);
	int timestamp = std::atoi(filename.substr(firstdot+1, lastdot-firstdot-1).c_str());

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


	MMSP::vector<double> f(NP+1, 0.0);

	// write grid data
	if (vector_type && fields>1) {
		if (float_type) {
			if (dim == 1) {
				MMSP::grid<1, MMSP::vector<float> > GRID(argv[1]);
				vectorFrac(GRID, f);
			} else if (dim == 2) {
				MMSP::grid<2, MMSP::vector<float> > GRID(argv[1]);
				vectorFrac(GRID, f);
			} else if (dim == 3) {
				MMSP::grid<3, MMSP::vector<float> > GRID(argv[1]);
				vectorFrac(GRID, f);
			}
		}
		if (double_type) {
			if (dim == 1) {
				MMSP::grid<1, MMSP::vector<double> > GRID(argv[1]);
				vectorFrac(GRID, f);
			} else if (dim == 2) {
				MMSP::grid<2, MMSP::vector<double> > GRID(argv[1]);
				vectorFrac(GRID, f);
			} else if (dim == 3) {
				MMSP::grid<3, MMSP::vector<double> > GRID(argv[1]);
				vectorFrac(GRID, f);
			}
		}
		if (long_double_type) {
			if (dim == 1) {
				MMSP::grid<1, MMSP::vector<long double> > GRID(argv[1]);
				vectorFrac(GRID, f);
			} else if (dim == 2) {
				MMSP::grid<2, MMSP::vector<long double> > GRID(argv[1]);
				vectorFrac(GRID, f);
			} else if (dim == 3) {
				MMSP::grid<3, MMSP::vector<long double> > GRID(argv[1]);
				vectorFrac(GRID, f);
			}
		}
	}

	std::cout << timestamp;
	for (unsigned int i=0; i<NP+1; i++)
		std::cout << ',' << f[i];
	std::cout << '\n';

	return 0;
}
