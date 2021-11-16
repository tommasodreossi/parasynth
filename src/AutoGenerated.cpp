#include "AutoGenerated.h"

using namespace AbsSyn;


/* creates the bundle defined by the model, adding directions and template rows according to assertions
 * and creating a template matrix if needed
 */
Bundle *computeBundle(const InputData &id);

// return i s.t. M[i] = v, or -1 if M does not contain v
int find(const std::vector<std::vector<double>> M, const std::vector<double> v);

// computes a vector having the same direction and length 1
std::vector<double> normalize(std::vector<double> v);

// computes the multiplicative coefficient to normalize vector
double getNormalizationCoefficient(std::vector<double> v);

// given a vector v1 and an offset val, return the new offset of the same constraint
// rescaled wrt v2
double rescale(double val, std::vector<double> v1, std::vector<double> v2);

// checks if two vectors are equal
bool compare (std::vector<double> v1, std::vector<double> v2, double tol);

// adds to lp the constraints that each LP var is in [0,1]
void binaryConstraints(glp_prob **lp, const std::vector<std::vector<double>> A,
													const std::vector<std::vector<double>> C, unsigned *startingIndex);

// adds to lp the constraints that each polytope has exactly as many directions as the number of vars
void paralCardConstraints(glp_prob **lp, const std::vector<std::vector<double>> A,
													const std::vector<std::vector<double>> C, unsigned *startingIndex);

// adds to lp the constraints that each variable is covered in each polytope
void varCoverConstraints(glp_prob **lp, const std::vector<std::vector<double>> A,
													const std::vector<std::vector<double>> C, unsigned *startingIndex);

// adds to lp the constraints that each direction is used
void directionConstraints(glp_prob **lp, const std::vector<std::vector<double>> A,
													const std::vector<std::vector<double>> C, unsigned *startingIndex);

// maps a direction and a Parallelotope to the corresponding variable
int map(int d, int P, int Pn)
{
	return d*(Pn) + P;
}


/*
 * This method computes a new template for the polytope.
 * The idea is that we want each parallelotope to contain directions covering
 * each variable, so that they are not singular. Moreover, each direction should be used at least once.
 * 
 * We use a LP problem to compute which direction goes in which parallelotope.
 * We define boolean variables X_i_j where i is a direction and j is a parallelotope,
 * meaning that the direction i is contained in the parallelotope j.
 * 
 * We add four families of constraints:
 *  - we bound each varaible to be in [0, 1]
 *  - we require that each parallelotope has the correct number of directions
 *  - we require that each variable of the dynamical system is covered in each parallelotope
 *  - we require that each direction appears in at least one parallelotope.
 * 
 * The objective function is simply the sum of all variables, since we are interested in satisfiability
 * and not in optimization
 */
std::vector<std::vector<int>> computeTemplate(const std::vector<std::vector<double>> A,
																		const std::vector<std::vector<double>> C);


AutoGenerated::AutoGenerated(const InputData &m)
{
  using namespace std;
  using namespace GiNaC;

  this->name = "AutoGenerated";

  for (unsigned i = 0; i < m.getVarNum(); i++) {
    this->vars.append(symbol(m.getVar(i)->getName()));
	}

  for (unsigned i = 0; i < m.getParamNum(); i++) {
    this->params.append(symbol(m.getParam(i)->getName()));
	}

  for (unsigned i = 0; i < m.getVarNum(); i++) {
    this->dyns.append(
        m.getVar(i)->getDynamic()->toEx(m, this->vars, this->params));
	}

	this->reachSet = computeBundle(m);

  // parameter directions
  if (m.paramDirectionsNum() != 0) {
    vector<vector<double>> pA(2 * m.paramDirectionsNum(),
                              vector<double>(m.getParamNum(), 0));
    for (unsigned i = 0; i < m.paramDirectionsNum(); i++) {
      pA[2 * i] = m.getParamDirection(i);
      for (unsigned j = 0; j < m.getParamNum(); j++)
        pA[2 * i + 1][j] = -pA[2 * i][j];
    }

    vector<double> pb(pA.size(), 0);
    for (unsigned i = 0; i < m.paramDirectionsNum(); i++) {
      pb[2 * i] = m.getParamUB()[i];
      pb[2 * i + 1] = -m.getParamLB()[i];
    }

    this->paraSet
        = new LinearSystemSet(std::make_shared<LinearSystem>(pA, pb));
  } else {
		this->paraSet = nullptr;
	}

  // formula
  if (m.isSpecDefined()) {
    this->spec = m.getSpec()->toSTL(m, vars, params);
  } else {
    this->spec = NULL;
  }
}

Bundle *computeBundle(const InputData &id)
{
	std::vector<std::vector<double>> directions = id.getDirections();
	std::vector<double> LB = id.getLB(), UB = id.getUB();
	std::vector<std::vector<int>> template_matrix = id.getTemplate();
	
	// rows that are not covered in the current template
	std::vector<bool> isRowCovered(directions.size(), false);
	for (unsigned i = 0; i < template_matrix.size(); i++) {
		for (unsigned j = 0; j < template_matrix[i].size(); j++) {
			isRowCovered[template_matrix[i][j]] = true;
		}
	}
	std::vector<std::vector<double>> uncoveredRows{};
	for (unsigned i = 0; i < isRowCovered.size(); i++) {
		if (!isRowCovered[i]) {
			uncoveredRows.push_back(directions[i]);
		}
	}
	
//	std::cout << "directions: " << directions << std::endl;
//	std::cout << "template: " << template_matrix << std::endl;
//	std::cout << "uncovered rows: " << uncoveredRows << std::endl;
	
	// prepare linear system to find LBs of new directions of asserts
	GiNaC::lst symbols{};
	for (unsigned i = 0; i < id.getVarNum(); i++) {
		GiNaC::symbol s(id.getVar(i)->getName());
		symbols.append(s);
	}
	
	// matrix and offsets of the LS
	std::vector<std::vector<double>> A = directions;
	for (unsigned i = 0; i < directions.size(); i++) {
		A.push_back(get_complementary(directions[i]));
	}
	
	std::vector<double> b = UB;
	for (unsigned i = 0; i < LB.size(); i++) {
		b.push_back(-LB[i]);
	}
	
	LinearSystem LS(A, b);
	
	// directions affected by constraints, and their offsets
	std::vector<std::vector<double>> constrDirs{};
	std::vector<double> constrOffsets{};
	
	// new directions to be added
	std::vector<std::vector<double>> C{};
	
	for (unsigned i = 0; i < id.getAssertNumber(); i++) {
		
		std::vector<double> new_dir = id.getAssert(i)->getDirection(id);
		std::vector<double> negated_dir = get_complementary(new_dir);
		
		int pos_dir = find(directions, new_dir);
		int pos_negated_dir = find(directions, negated_dir);
		
		if (pos_dir != -1) {			// constrain a direction which is in the L matrix

			constrDirs.push_back(directions[pos_dir]);
			constrOffsets.push_back(rescale(id.getAssert(i)->getOffset(id), new_dir, constrDirs[i]));
			UB[pos_dir] = std::min(UB[pos_dir], constrOffsets[i]);

		} else if (pos_negated_dir != -1) {		// constrain a direction opposite of one in L matrix

			constrDirs.push_back(get_complementary(directions[pos_negated_dir]));
			constrOffsets.push_back(rescale(id.getAssert(i)->getOffset(id), new_dir, constrDirs[i]));
			LB[pos_negated_dir] = std::max(LB[pos_negated_dir], constrOffsets[i]);

		} else {									// constrain a direction not in L matrix

			int C_pos = find(C, new_dir);
			int C_negated_pos = find(C, negated_dir);
			
			if (C_pos != -1) {			// direction is already constrained, change offset and UB

				constrOffsets[directions.size() + C_pos] = std::min(
					constrOffsets[directions.size() + C_pos], id.getAssert(i)->getOffset(id));
				UB[directions.size() + C_pos] = constrOffsets[directions.size() + C_pos];

			} else if (C_negated_pos != -1) {			// negated direction is constrained, change LB and add new direction

				constrDirs.push_back(new_dir);
				constrOffsets.push_back(id.getAssert(i)->getOffset(id));
				LB[directions.size() + C_negated_pos] = -id.getAssert(i)->getOffset(id);

			} else {								// new direction

				constrDirs.push_back(new_dir);
				constrOffsets.push_back(id.getAssert(i)->getOffset(id));
				C.push_back(new_dir);
				
				UB.push_back(constrOffsets[i]);
			
				// get LB s.t. it doesn't cut points from the polytope
				GiNaC::ex obj_function = 0;
				for (unsigned j = 0; j < new_dir.size(); j++) {
					obj_function += new_dir[j] * symbols[j];
				}
				double min_val = LS.minLinearSystem(symbols, obj_function);
				LB.push_back(min_val);
			}
		}	// end else dir_pos
	}
	
	std::vector<std::vector<int>> templ = computeTemplate(uncoveredRows, C);
	
//	std::cout << "computed template: " << templ << std::endl;
	
	// add C to directions
	for (unsigned i = 0; i < C.size(); i++) {
		directions.push_back(C[i]);
	}
	
	// append computed template to the existing one
	template_matrix.insert(
		template_matrix.end(),
		std::make_move_iterator(templ.begin()),
		std::make_move_iterator(templ.end())
	);
	
//	std::cout << "final template: " << template_matrix << std::endl;

  return new Bundle(directions, UB, get_complementary(LB), template_matrix, constrDirs, constrOffsets);
}

int find(const std::vector<std::vector<double>> M, const std::vector<double> v)
{
	std::vector<double> v_norm = normalize(v);
	for (unsigned i = 0; i < M.size(); i++) {
		std::vector<double> M_norm = normalize(M[i]);
		if (compare(v_norm, M_norm, 0.00001)) {
			return i;
		}
	}
	return -1;
}

double getNormalizationCoefficient(std::vector<double> v)
{
	double res = 0;
	for (unsigned i = 0; i < v.size(); i++) {
		res += v[i]*v[i];
	}
	return 1.0 / sqrt(res);
}

std::vector<double> normalize(std::vector<double> v)
{
	double coeff = getNormalizationCoefficient(v);
	std::vector<double> res{};
	for (unsigned i = 0; i < v.size(); i++) {
		res.push_back(coeff * v[i]);
	}
	return res;
}

bool compare (std::vector<double> v1, std::vector<double> v2, double tol)
{
	if (v1.size() != v2.size()) {
		return false;
	}
	
	for (unsigned i = 0; i < v1.size(); i++) {
		if (abs(v1[i] - v2[i]) > tol) {
			return false;
		}
	}
	
	return true;
}

double rescale(double val, std::vector<double> v1, std::vector<double> v2)
{
	return val * getNormalizationCoefficient(v1) / getNormalizationCoefficient(v2);
}


std::vector<std::vector<int>> computeTemplate(const std::vector<std::vector<double>> A,
																		const std::vector<std::vector<double>> C)
{
	if (A.size() == 0 && C.size() == 0) {
		return {};
	}
	
	unsigned n = (A.size() != 0 ? A[0].size() : C[0].size());			// var num, = cols of A (or of C)
	unsigned m = A.size();				// dirs num = rows of A
	unsigned c = C.size();				// constr num = assertions = rows of C
	unsigned Pn = ceil(((double) (m + c))/n);		// number of parallelotopes required
	
	unsigned cols = Pn * (m+c);
	unsigned rows = Pn + Pn*n + (m + c);
	
//	std::cout << "\tn = " << n << ", m = " << m << ", c = " << c << ", Pn = " << Pn << ", cols = " << cols << ", rows = " << rows << std::endl;
	
	// create ILP problem
	glp_prob *lp;
  lp = glp_create_prob();
  glp_set_obj_dir(lp, GLP_MIN);	
	
	// continuous case parameters
  glp_smcp lp_param;
  glp_init_smcp(&lp_param);
  lp_param.msg_lev = GLP_MSG_ERR;
	
	// integer/boolean case parameters
/*	glp_iocp ilp_param;
	glp_init_iocp(&ilp_param);
	ilp_param.presolve = GLP_ON;
  ilp_param.msg_lev = GLP_MSG_OFF;*/
	
	// add rows
	glp_add_rows(lp, rows);
	
	// add columns (unbounded)
	glp_add_cols(lp, cols);
  for (unsigned int i = 0; i < cols; i++) {
		// columns are unbounded (continuous case)
		glp_set_col_bnds(lp, i + 1, GLP_DB, 0, 1);
		// set columns to be boolean (discrete case)
//		glp_set_col_kind(lp, i + 1, GLP_BV);
  }
  
  // add objective function (all ones, we want only satisfiability)
  for (unsigned i = 0; i < cols; i++) {
		glp_set_obj_coef(lp, i+1, 0);
	}
	
	unsigned globalIndex = 1;
//	binaryConstraints(&lp, A, C, &globalIndex);
	paralCardConstraints(&lp, A, C, &globalIndex);
	varCoverConstraints(&lp, A, C, &globalIndex);
	directionConstraints(&lp, A, C, &globalIndex);
	
	/* TODO: check that LP (not integer) is sufficient ->
	 * matrix is not always TUM, but solution so far has always been int
	 */
	glp_simplex(lp, &lp_param);
//	glp_intopt(lp, &ilp_param);
	
	
	std::vector<std::vector<int>> T(Pn, std::vector<int>{});
	for (unsigned P = 0; P < Pn; P++) {
		for (unsigned d = 0; d < m+c; d++) {
//			std::cout << "X_" << d << "_" << P << " = " << glp_get_col_prim(lp, map(d, P, Pn) + 1) << std::endl;
			if (glp_get_col_prim(lp, map(d, P, Pn) + 1) == 1) {
//			if (glp_mip_col_val(lp, map(d, P, Pn) + 1) == 1) {
				T[P].push_back(d);
			}
		}
	}
	
	return T;
}

void binaryConstraints(glp_prob **lp, const std::vector<std::vector<double>> A,
													const std::vector<std::vector<double>> C, unsigned *startingIndex)
{
	unsigned n = A[0].size();		// var num, = cols of A
	unsigned m = A.size();				// dirs num = rows of A
	unsigned c = C.size();				// constr num = assertions = rows of C
	unsigned Pn = ceil(((double) (m + c))/n);		// number of parallelotopes required
	
	unsigned index = *startingIndex;
	
	// for each LP var x, x is in [0,1]
	for (unsigned P = 0; P < Pn; P++) {
		for (unsigned d = 0; d < m+c; d++) {
			int *indeces = (int *) malloc(2 * sizeof(int));
			double *constr = (double *) malloc(2 * sizeof(double));
			indeces[0] = 0;
			constr[0] = 0;
			indeces[1] = map(d, P, Pn) + 1;
			constr[1] = 1;
			
			glp_set_row_bnds(*lp, index, GLP_DB, 0, 1);
			glp_set_mat_row(*lp, index, 1, indeces, constr);

			free(constr);
			free(indeces);
			index++;
		}
	}
	*startingIndex = index;
}

void paralCardConstraints(glp_prob **lp, const std::vector<std::vector<double>> A,
													const std::vector<std::vector<double>> C, unsigned *startingIndex)
{
	unsigned n = A[0].size();		// var num, = cols of A
	unsigned m = A.size();				// dirs num = rows of A
	unsigned c = C.size();				// constr num = assertions = rows of C
	unsigned Pn = ceil(((double) (m + c))/n);		// number of parallelotopes required
	
	unsigned cols = Pn * (m+c);
	
	unsigned index = *startingIndex;
	
	// for each Parallelotope P, P has n directions
	for (unsigned P = 0; P < Pn; P++) {
		int *indeces = (int *) malloc((cols + 1) * sizeof(int));
		double *constr = (double *) malloc((cols + 1) * sizeof(double));
		int len = 0;
		indeces[0] = 0;
		constr[0] = 0;
		for (unsigned d = 0; d < m+c; d++) {
			len++;
			indeces[len] = map(d, P, Pn) + 1;
			constr[len] = 1;
		}
		glp_set_row_bnds(*lp, index, GLP_FX, n, n);
		glp_set_mat_row(*lp, index, len, indeces, constr);
		free(constr);
		free(indeces);
		index++;
	}
	*startingIndex = index;
}

void varCoverConstraints(glp_prob **lp, const std::vector<std::vector<double>> A,
													const std::vector<std::vector<double>> C, unsigned *startingIndex)
{
	unsigned n = A[0].size();		// var num, = cols of A
	unsigned m = A.size();				// dirs num = rows of A
	unsigned c = C.size();				// constr num = assertions = rows of C
	unsigned Pn = ceil(((double) (m + c))/n);		// number of parallelotopes required
	
	unsigned cols = Pn * (m+c);
	
	unsigned index = *startingIndex;
	
	// each parallelotope covers each variable
	for (unsigned v = 0; v < n; v++) {		// for each variable v
		for (unsigned P = 0; P < Pn; P++) { // for each Parallelotope
			int *indeces = (int *) malloc((cols + 1) * sizeof(int));
			double *constr = (double *) malloc((cols + 1) * sizeof(double));
			int len = 0;
			indeces[0] = 0;
			constr[0] = 0;
			for (unsigned d = 0; d < m; d++) {
				if (A[d][v] != 0) {								// for each direction in A covering v
					len++;
					indeces[len] = map(d, P, Pn) + 1;
					constr[len] = 1;
				}
			}
		
			for (unsigned d = 0; d < c; d++) {
				if (C[d][v] != 0) {								// for each direction in C covering v
					len++;
					indeces[len] = map(d+m, P, Pn) + 1;
					constr[len] = 1;
				}
			}
			
			glp_set_row_bnds(*lp, index, GLP_LO, 1, 0);
			glp_set_mat_row(*lp, index, len, indeces, constr);
			
			free(constr);
			free(indeces);
			index++;
		}
	}
	*startingIndex = index;
}


void directionConstraints(glp_prob **lp,
							std::vector<std::vector<double>> A, std::vector<std::vector<double>> C, unsigned *startingIndex)
{
	unsigned n = A[0].size();		// var num, = cols of A
	unsigned m = A.size();				// dirs num = rows of A
	unsigned c = C.size();				// constr num = assertions = rows of C
	unsigned Pn = ceil(((double) (m + c))/n);		// number of parallelotopes required
	
	unsigned cols = Pn * (m+c);
	
	unsigned index = *startingIndex;
	
	// directions in A
	for (unsigned d = 0; d < m; d++) {
		int *indeces = (int *) malloc((cols + 1) * sizeof(int));
		double *constr = (double *) malloc((cols + 1) * sizeof(double));
		int len = 0;
		indeces[0] = 0;
		constr[0] = 0;
		for (unsigned P = 0; P < Pn; P++) {
			len++;
			indeces[len] = map(d, P, Pn) + 1;
			constr[len] = 1;
		}
		glp_set_row_bnds(*lp, index, GLP_LO, 1, 0);
		glp_set_mat_row(*lp, index, len, indeces, constr);
		free(constr);
		free(indeces);
		index++;
	}
	
	// directions in C
	for (unsigned d = 0; d < c; d++) {
		int *indeces = (int *) malloc((cols + 1) * sizeof(int));
		double *constr = (double *) malloc((cols + 1) * sizeof(double));
		int len = 0;
		indeces[0] = 0;
		constr[0] = 0;
		for (unsigned P = 0; P < Pn; P++) {
			len++;
			indeces[len] = map(d + m, P, Pn) + 1;
			constr[len] = 1;
		}
		glp_set_row_bnds(*lp, index, GLP_LO, 1, 0);
		glp_set_mat_row(*lp, index, len, indeces, constr);
		free(constr);
		free(indeces);
		index++;
	}
	*startingIndex = index;
}
