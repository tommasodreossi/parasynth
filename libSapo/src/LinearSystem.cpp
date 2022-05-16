/**
 * @file LinearSystem.cpp
 * Represent and manipulate a linear system
 * It can be used to represent polytopes (reached states, parameters, etc.)
 *
 * @author Tommaso Dreossi <tommasodreossi@berkeley.edu>
 * @version 0.1
 */

#include <iostream>

#include <glpk.h>
#include <limits>
#include <cmath>

#include "LinearSystem.h"
#include "SymbolicAlgebra.h"

/**
 * @brief Print a linear system in a stream
 * 
 * @param out is the output stream
 * @param ls is the linear system to be print
 * @return a reference to the output stream
 */
std::ostream &operator<<(std::ostream &out, const LinearSystem &ls)
{
  for (unsigned int row_idx = 0; row_idx < ls.size(); row_idx++) {
    if (row_idx != 0) {
      out << std::endl;
    }

    for (unsigned int col_idx = 0; col_idx < ls.dim(); col_idx++) {
      out << ls.A(row_idx, col_idx) << " ";
    }
    out << "<= " << ls.b(row_idx);
  }

  return out;
}

/**
 * @brief Print a linear system in a JSON stream
 * 
 * @param out is the output JSON stream
 * @param ls is the linear system to be print
 * @return a reference to the output JSON stream
 */
JSON::ostream &operator<<(JSON::ostream &out, const LinearSystem &ls)
{
  out << "{\"A\":" << ls.A() << ","
      << "\"b\":" << ls.b() << "}";

  return out;
}

OptimizationResult<double> optimize(const std::vector<LinearAlgebra::Vector<double>> &A,
                                    const LinearAlgebra::Vector<double> &b,
                                    const LinearAlgebra::Vector<double> &obj_fun,
                                    const bool maximize)
{
  unsigned int num_rows = A.size();
  unsigned int num_cols = obj_fun.size();
  unsigned int size_lp = num_rows * num_cols;

  int *ia, *ja;
  double *ar;

  ia = (int *)calloc(size_lp + 1, sizeof(int));
  ja = (int *)calloc(size_lp + 1, sizeof(int));
  ar = (double *)calloc(size_lp + 1, sizeof(double));

  glp_prob *lp;
  lp = glp_create_prob();
  glp_set_obj_dir(lp, (maximize ? GLP_MAX : GLP_MIN));

  // Turn off verbose mode
  glp_smcp lp_param;
  glp_init_smcp(&lp_param);
  lp_param.msg_lev = GLP_MSG_ERR;

  glp_add_rows(lp, num_rows);
  for (unsigned int i = 0; i < num_rows; i++) {
    glp_set_row_bnds(lp, i + 1, GLP_UP, 0.0, b[i]);
  }

  glp_add_cols(lp, num_cols);
  for (unsigned int i = 0; i < num_cols; i++) {
    glp_set_col_bnds(lp, i + 1, GLP_FR, 0.0, 0.0);
  }

  for (unsigned int i = 0; i < num_cols; i++) {
    glp_set_obj_coef(lp, i + 1, obj_fun[i]);
  }

  unsigned int k = 1;
  for (unsigned int i = 0; i < num_rows; i++) {
    for (unsigned int j = 0; j < num_cols; j++) {
      ia[k] = i + 1;
      ja[k] = j + 1;
      ar[k] = A[i][j]; /* a[i+1,j+1] = A[i][j] */
      k++;
    }
  }

  glp_load_matrix(lp, size_lp, ia, ja, ar);
  glp_exact(lp, &lp_param);
  //	glp_simplex(lp, &lp_param);

  double res;
  switch (glp_get_status(lp)) {
  case GLP_UNBND:
    if (maximize) {
      res = std::numeric_limits<double>::infinity();
    } else {
      res = -std::numeric_limits<double>::infinity();
    }
    break;
  default:
    res = glp_get_obj_val(lp);
  }

  OptimizationResult<double> opt_res(res, glp_get_status(lp));

  glp_delete_prob(lp);
  glp_free_env();
  free(ia);
  free(ja);
  free(ar);

  return opt_res;
}

OptimizationResult<double>
LinearSystem::optimize(const LinearAlgebra::Vector<double> &obj_fun,
                       const bool maximize) const
{
  return ::optimize(this->_A, this->_b, obj_fun, maximize);
}

/**
 * Minimize the linear system
 *
 * @param[in] obj_fun objective function
 * @return minimum
 */
OptimizationResult<double>
LinearSystem::minimize(const LinearAlgebra::Vector<double> &obj_fun) const
{
  return ::optimize(this->_A, this->_b, obj_fun, false);
}

/**
 * Maximize the linear system
 *
 * @param[in] obj_fun objective function
 * @return maximum
 */
OptimizationResult<double>
LinearSystem::maximize(const LinearAlgebra::Vector<double> &obj_fun) const
{
  return ::optimize(this->_A, this->_b, obj_fun, true);
}

/**
 * Constructor
 *
 * @param[in] A is the matrix 
 * @param[in] b is the offset vector
 */
LinearSystem::LinearSystem(const std::vector<LinearAlgebra::Vector<double>> &A,
                           const LinearAlgebra::Vector<double> &b)
{
  bool smart_insert = false;

  if (!smart_insert) {
    this->_A = A;
    this->_b = b;
  } else {
    for (unsigned int i = 0; i < A.size(); i++) {
      if ((LinearAlgebra::norm_infinity(A[i]) > 0) &&
          !this->satisfies(A[i], b[i])) {
        this->_A.push_back(A[i]);
        this->_b.push_back(b[i]);
      }
    }
  }
}

/**
 * Move constructor
 *
 * @param[in] A is the matrix 
 * @param[in] b is the offset vector
 */
LinearSystem::LinearSystem(std::vector<LinearAlgebra::Vector<double>> &&A,
                           LinearAlgebra::Vector<double> &&b):
    _A(std::move(A)),
    _b(std::move(b))
{
}

/**
 * Constructor that instantiates an empty linear system
 */
LinearSystem::LinearSystem(): _A(), _b() {}

/**
 * Copy constructor
 *
 * @param[in] ls is the original linear system
 */
LinearSystem::LinearSystem(const LinearSystem &ls): _A(ls._A), _b(ls._b) {}

/**
 * Swap constructor
 *
 * @param[in] ls is the linear system
 */
LinearSystem::LinearSystem(LinearSystem &&ls)
{
  swap(*this, ls);
}

/**
 * @brief Check whether two linear equations are the same
 *
 * @param A1 non-constant coefficient of the first equation
 * @param b1 constant coefficient of the first equation
 * @param A2 non-constant coefficient of the second equation
 * @param b2 constant coefficient of the second equation
 * @return whether the two linear equations are the same
 */
bool same_constraint(const LinearAlgebra::Vector<double> &A1, const double &b1,
                     const LinearAlgebra::Vector<double> &A2, const double &b2)
{
  if (A1.size() != A2.size()) {
    return false;
  }

  if (b1 != b2) {
    return false;
  }

  for (unsigned int i = 0; i < A1.size(); ++i) {
    if (A1[i] != A2[i]) {
      return false;
    }
  }

  return true;
}

bool LinearSystem::contains(const LinearAlgebra::Vector<double> &Ai, const double &bi) const
{
  for (unsigned int i = 0; i < this->_A.size(); i++) {
    if (same_constraint(this->_A[i], this->_b[i], Ai, bi)) {
      return true;
    }
  }
  return false;
}

LinearSystem::LinearSystem(
    const std::vector<SymbolicAlgebra::Symbol<>> &x,
    const std::vector<SymbolicAlgebra::Expression<>> &expressions)
{
  using namespace SymbolicAlgebra;
  std::vector<Expression<>> lexpressions = expressions;
  // lconstraints.unique();  // remove multiple copies of the same expression

  for (auto e_it = begin(lexpressions); e_it != end(lexpressions); ++e_it) {
    LinearAlgebra::Vector<double> Ai;
    Expression<> const_term(*e_it);

    for (auto x_it = begin(x); x_it != end(x); ++x_it) {
      if (e_it->degree(*x_it)>1) {
        throw std::domain_error("Non-linear expression cannot be mapped "
                                "in a linear system.");
      }

      // Extract the coefficient of the i-th variable (grade 1)
      double coeff = (e_it->get_coeff(*x_it, 1)).evaluate();
      Ai.push_back(coeff);

      // Project to obtain the constant term
      const_term = const_term.get_coeff(*x_it, 0);
    }

    double bi = const_term.evaluate();

    if (!this->contains(Ai, -bi)) {
      this->_A.push_back(Ai);
      this->_b.push_back(-bi);
    }
  }
}

/**
 * Return the (i,j) element of the template matrix
 *
 * @param[in] i row index
 * @param[in] j column index
 * @return (i,j) element
 */
const double &LinearSystem::A(unsigned int i, unsigned int j) const
{
  if (i < this->_A.size() && j < this->_A[j].size()) {
    return this->_A[i][j];
  }
  throw std::domain_error("LinearSystem::getA: i and j must be valid "
                          "indices for the system matrix A");
}

/**
 * Return the i-th element of the offset vector
 *
 * @param[in] i column index
 * @return i-th element
 */
const double &LinearSystem::b(unsigned int i) const
{
  if (i < this->_b.size()) {
    return this->_b[i];
  }
  throw std::domain_error("LinearSystem::getb: i must be a valid "
                          "index for the system");
}

bool LinearSystem::has_solutions(const bool strict_inequality) const
{
  if (this->size() == 0) {
    return true;
  }

  if (!strict_inequality) {
    OptimizationResult<double> res = maximize(_A[0]);

    return res.status() == GLP_OPT || res.status() == GLP_UNBND;
  }

  for (auto row_it = std::begin(_A); row_it != std::end(_A); ++row_it) {
    OptimizationResult<double> res = maximize(*row_it);
    if ((res.status() == GLP_NOFEAS) || (res.status() == GLP_INFEAS)) {
      return false;
    }
    OptimizationResult<double> res2 = minimize(*row_it);
    if ((res2.status() == GLP_NOFEAS) || (res2.status() == GLP_INFEAS)) {
      return false;
    }

    if (res.optimum()==res2.optimum()) {
      return false;
    }
  }

  return true;
}

/**
 * Minimize the linear system
 *
 * @param[in] symbols array of symbols
 * @param[in] obj_fun objective function
 * @return minimum
 */
OptimizationResult<double>
LinearSystem::minimize(const std::vector<SymbolicAlgebra::Symbol<>> &symbols,
                       const SymbolicAlgebra::Expression<> &obj_fun) const
{
  using namespace SymbolicAlgebra;

  LinearAlgebra::Vector<double> obj_fun_coeffs;
  Expression<> const_term(obj_fun);

  // Extract the coefficient of the i-th variable (grade 1)
  for (auto s_it = begin(symbols); s_it != end(symbols); ++s_it) {
    if (obj_fun.degree(*s_it)>1) {
      throw std::domain_error("Non-linear objective function is not supported yet.");
    }
    double coeff = (obj_fun.get_coeff(*s_it, 1)).evaluate();

    obj_fun_coeffs.push_back(coeff);
    const_term = const_term.get_coeff(*s_it, 0);
  }

  const double c = const_term.evaluate();
  auto res = minimize(obj_fun_coeffs);

  return OptimizationResult<double>(res.optimum() + c, res.status());
}

/**
 * Maximize the linear system
 *
 * @param[in] symbols array of symbols
 * @param[in] obj_fun objective function
 * @return maximum
 */
OptimizationResult<double>
LinearSystem::maximize(const std::vector<SymbolicAlgebra::Symbol<>> &symbols,
                       const SymbolicAlgebra::Expression<> &obj_fun) const
{
  using namespace SymbolicAlgebra;

  LinearAlgebra::Vector<double> obj_fun_coeffs;
  Expression<> const_term(obj_fun);

  // Extract the coefficient of the i-th variable (grade 1)
  for (auto s_it = begin(symbols); s_it != end(symbols); ++s_it) {
    if (obj_fun.degree(*s_it)>1) {
      throw std::domain_error("Non-linear objective function is not supported yet.");
    }
    const double coeff = obj_fun.get_coeff(*s_it, 1).evaluate();
    obj_fun_coeffs.push_back(coeff);
    const_term = const_term.get_coeff(*s_it, 0);
  }

  const double c = const_term.evaluate();
  auto res = maximize(obj_fun_coeffs);

  return OptimizationResult<double>(res.optimum() + c, res.status());
}

/**
 * @brief Check whether the solutions of a linear system satisfy a constraint
 *
 * This method establishes whether all the solutions \f$s\f$ of the linear 
 * system satisfy a constraint \f$\textrm{Ai} \cdot s \leq \textrm{bi}\f$.
 * 
 * @param[in] Ai is a value vector
 * @param[in] bi is a scalar value
 * @return `true` if and only if \f$\textrm{Ai} \cdot s \leq \textrm{bi}\f$
 *         for any the solution \f$s\f$ of the linear system
 */
bool LinearSystem::satisfies(const LinearAlgebra::Vector<double> &Ai, 
                             const double &bi) const
{
  if (size() == 0) {
    return false;
  }

  OptimizationResult<double> res = this->maximize(Ai);
  if (res.status() == GLP_OPT && res.optimum() <= bi) { 
    return true;
  }

  return false;
}

bool LinearSystem::satisfies(const LinearSystem& ls) const
{
  if (!this->has_solutions()) {
    return true;
  }

  for (unsigned int i = 0; i < ls.size(); i++) {
    if (!this->satisfies(ls._A[i], ls._b[i])) {
      return false;
    }
  }

  return true;
}

bool LinearSystem::constraint_is_redundant(const unsigned int i) const
{
  LinearSystem tmp(*this);
  LinearAlgebra::Vector<double> Ai(dim(), 0);
  double bi(0);

  // replace the i-th constraint with the empty constraint
  std::swap(Ai, tmp._A[i]);
  std::swap(bi, tmp._b[i]);

  // check whether the i-th constraint is redundant
  bool result(tmp.satisfies(Ai, bi));

  // reinsert the i-th into the system
  std::swap(Ai, tmp._A[i]);
  std::swap(bi, tmp._b[i]);

  return result;
}

/**
 * Remove redundant constraints from a linear system
 *
 * This method removes redundant constraints from the system.
 * The order of the non-redundant constraints can be shuffled after
 * the call.
 *
 * @return A reference to this object after removing all the
 *         redundant constraints
 */
LinearSystem &LinearSystem::simplify()
{
  if (size() == 0) {
    return *this;
  }

  unsigned int i = 0, last_non_redundant = size() - 1;

  while (i < last_non_redundant) { // for every unchecked constraint

    // if it is redundant
    if (constraint_is_redundant(i)) {
      // swap it with the last non-reduntant constraint
      std::swap(_A[i], _A[last_non_redundant]);
      std::swap(_b[i], _b[last_non_redundant]);

      // decrease the number of the non-reduntant constraints
      last_non_redundant--;
    } else { // otherwise, i.e. if it is not redundant

      // increase the index of the next constraint to be checked
      i++;
    }
  }

  // if the last constraint to be checked is redundant
  if (constraint_is_redundant(last_non_redundant)) {
    // reduce the number of non-reduntant constraints
    last_non_redundant--;
  }

  // remove the redundant constraints that are at the end of the system
  _A.resize(last_non_redundant + 1);
  _b.resize(last_non_redundant + 1);

  return *this;
}

/**
 * Remove redundant constraints
 */
LinearSystem LinearSystem::get_simplified() const
{
  LinearSystem simplier(*this);

  return simplier.simplify();
}
