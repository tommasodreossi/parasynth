/**
 * @file Flowpipe.cpp
 * Represent and manipulate flowpipes of bundle
 * Used to represent the reachable set of a dynamical system
 *
 * @author Tommaso Dreossi <tommasodreossi@berkeley.edu>
 * @version 0.1
 */

#include <fstream>
#include <string>

#include "Flowpipe.h"

/**
 * Constructor that instantiates Flowpipe
 */
Flowpipe::Flowpipe(): v_templates(), flowpipe() {}

/**
 * Constructor that instantiates Flowpipe
 */
Flowpipe::Flowpipe(const std::vector<std::vector<double>> &variable_templates):
    v_templates(variable_templates), flowpipe()
{
}

/**
 * Return the i-th linear system
 *
 * @param[in] i index
 * @return i-th linear system
 */
const LinearSystemSet &Flowpipe::get(const unsigned int i) const
{
  if (i < this->size()) {
    return this->flowpipe[i];
  }
  std::cerr << "Flowpipe::get : i must be between 0 and the flowpipe size"
            << std::endl;
  exit(EXIT_FAILURE);
}

/**
 * Append a bundle to the flowpipe
 *
 * @param[in] bundle bundle to be appended
 * @return a reference to the new flowpipe
 */
Flowpipe &Flowpipe::append(const Bundle &bundle)
{
  this->flowpipe.push_back(LinearSystemSet(bundle.getLinearSystem()));

  return *this;
}

/**
 * Append a linear system set to the flowpipe
 *
 * @param[in] ls is the linear system set to be appended
 * @return a reference to the new flowpipe
 */
Flowpipe &Flowpipe::append(const LinearSystemSet &ls)
{
  this->flowpipe.push_back(ls);

  return *this;
}

/**
 * Append a linear system to the flowpipe
 *
 * @param[in] ls is the linear system set to be appended
 * @return a reference to the new flowpipe
 */
Flowpipe &Flowpipe::append(const LinearSystem &ls)
{
  this->flowpipe.push_back(LinearSystemSet(ls));

  return *this;
}

unsigned int Flowpipe::dim() const
{
  if (this->flowpipe.empty()) {
    return 0;
  }

  return (this->flowpipe[0]).dim();
}

/**
 * Display the flowpipe
 */
void Flowpipe::print() const
{
  for (auto it = std::begin(flowpipe); it != std::end(flowpipe); ++it) {
    std::cout << *it << std::endl << std::endl;
  }
}

/**
 * Print the linear system in Matlab format (for plotregion script)
 *
 * @param[in] os is the output stream
 * @param[in] color color of the polytope to plot
 */
void Flowpipe::plotRegion(std::ostream &os, const char color) const
{
  for (auto it = std::begin(flowpipe); it != std::end(flowpipe); ++it) {
    it->plotRegion(os, color);
  }
}

/**
 * Print the projection of the variable in time using Matlab format into a file
 *
 * @param[in] os is the output stream
 * @param[in] var variable to be plotted
 * @param[in] time_step time step for time projection
 * @param[in] color color of the polytope to plot
 */
void Flowpipe::plotProj(std::ostream &os, const unsigned int var,
                        const double time_step, const char color) const
{

  if (size() == 0 || this->flowpipe[0].isEmpty()) {
    std::cerr << "Flowpipe::plotProjToFile : i must be between "
              << "0 and the system dimension" << std::endl;
    exit(EXIT_FAILURE);
  }

  if (var >= this->flowpipe[0].dim()) {
    std::cerr << "Flowpipe::plotProjToFile : i must be between "
              << "0 and the system dimension" << std::endl;
    exit(EXIT_FAILURE);
  }

  // select figure
  os << "figure(" << var + 1 << ")" << std::endl;

  // print time
  os << "t = [ ";
  for (unsigned int i = 0; i < this->size(); i++) {
    os << i * time_step << " ";
  }
  os << " ];" << std::endl;

  // print lower offsets
  os << "varm = [";
  for (auto it = std::begin(this->flowpipe); it != std::end(this->flowpipe);
       ++it) {
    LinearSystemSet::const_iterator ls_it(it->begin());

    double min_value = ls_it->minLinearSystem(v_templates[var]);

    for (; ls_it != it->end(); ++ls_it) {
      double min_var_value = ls_it->minLinearSystem(v_templates[var]);

      min_value = std::min(min_var_value, min_value);
    }
    os << " " << min_value;
  }
  os << " ];" << std::endl;

  // print upper offsets
  os << "varp = [";
  for (auto it = std::begin(this->flowpipe); it != std::end(this->flowpipe);
       ++it) {
    std::vector<double> obj_funct(dim(), 0.0);
    obj_funct[var] = 1.0;

    LinearSystemSet::const_iterator ls_it(it->begin());

    double max_value = ls_it->maxLinearSystem(v_templates[var]);

    for (; ls_it != it->end(); ++ls_it) {
      double max_var_value = ls_it->maxLinearSystem(v_templates[var]);

      max_value = std::min(max_var_value, max_value);
    }
    os << " " << max_value;
  }
  os << " ];" << std::endl;

  os << "T = [t,fliplr(t)];" << std::endl;
  os << "X = [varm,fliplr(varp)];" << std::endl;
  os << "fill(T,X,'" << color << "');" << std::endl;
}

Flowpipe::~Flowpipe()
{
  // TODO Auto-generated destructor stub
}
