/**
 * @file Disjunction.h
 * Disjunction STL formula
 *
 * @author Tommaso Dreossi <tommasodreossi@berkeley.edu>
 * @author Alberto Casagrande <acasagrande@units.it>
 * @version 0.2
 */

#ifndef DISJUNCTION_H_
#define DISJUNCTION_H_

#include <memory>

#include "STL.h"

namespace STL
{

/**
 * @brief The STL disjunction formula
 * 
 * This class represents STL disjunction formulas.
 */
class Disjunction : public STL
{

private:
  const std::shared_ptr<STL> _left;   //!< left-side dicjunction subformula
  const std::shared_ptr<STL> _right;  //!< right-side dicjunction subformula

protected:

  /**
   * @brief Print the STL formula in a stream
   * 
   * This method is publicly wrapped by the function
   * `operator<<(std::ostream&, const STL::STL &)`.
   * 
   * @param os is the output stream
   * @return a reference to the output stream
   */
  std::ostream &print(std::ostream &os) const;

public:
  /**
   * @brief A constructor for STL dicjunction formulas
   * 
   * This constructor creates an object of the type 
   * Disjunction to represent the STL formula 
   * \f$\textrm{left} \lor \textrm{right}\f$.
   *
   * @param[in] left is the left-side disjunction subformula
   * @param[in] right is the right-side disjunction subformula
   */
  Disjunction(const std::shared_ptr<STL> left,
              const std::shared_ptr<STL> right);

  /**
   * @brief Get the left-side disjunction subformula
   * 
   * @return the left-side disjunction subformula
   */
  inline const std::shared_ptr<STL> get_left_subformula() const
  {
    return _left;
  }

  /**
   * @brief Get the right-side disjunction subformula
   * 
   * @return the right-side disjunction subformula
   */
  inline const std::shared_ptr<STL> get_right_subformula() const
  {
    return _right;
  }

  /**
   * @brief Get an equivalent formula in Positive Normal Form (PNF)
   * 
   * A STL formula is in Positive Normal Form (PNF) if it does 
   * not use the negation operator. 
   * For any STL formula \f$\phi\f$ there exists a STL 
   * formula \f$\varphi\f$ in PNF such that 
   * \f[\models \phi \Longleftrightarrow \models \varphi.\f]
   * This method computes the STL formula in PNF of the 
   * current formula.
   * 
   * @return The equivalent STL formula in PNF
   */
  inline const std::shared_ptr<STL> get_PNF() const
  {
    return std::make_shared<Disjunction>(_left->get_PNF(),
                                         _right->get_PNF());
  }

  /**
   * @brief Get the formula time bounds 
   * 
   * @return the time interval affecting the formula sematics
   */
  TimeInterval time_bounds() const;

  /**
   * @brief Destroy the STL disjunction formula
   */
  ~Disjunction();
};
}

#endif /* DISJUNCTION_H_ */
