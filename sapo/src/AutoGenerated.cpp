#include "AutoGenerated.h"

#include <SymbolicAlgebra.h>
#include <LinearAlgebra.h>
#include <DifferentialSystem.h>
#include <Integrator.h>

#include <ErrorHandling.h>

using namespace AbsSyn;

// ALL THE CODE IN THIS FILE MUST BE REVIEWED

std::set<std::vector<unsigned int>>
trim_unused_directions(std::vector<std::vector<double>> &directions,
                       std::vector<double> &LB, std::vector<double> &UB,
                       std::set<size_t> &adaptive_directions,
                       const std::set<std::vector<unsigned int>> &templates)
{
  /* init new_pos by assigning 1 to any useful direction and
   * 0 to those not mentioned by any template */
  std::vector<int> new_pos(directions.size(), 0);
  for (const std::vector<unsigned int> &T: templates) {
    for (const unsigned int &dir: T) {
      new_pos[dir] = 1;
    }
  }

  /* ala CountingSort we sum the content of all the cells
   * with the previous partial sum to get the new position
   * of the used direction */
  for (unsigned int i = 1; i < directions.size(); i++) {
    new_pos[i] += new_pos[i - 1];
  }

  /* The overall number of useful directions is the last new position */
  unsigned int num_of_directions = new_pos[directions.size() - 1];

  for (unsigned int i = 0; i < directions.size(); i++) {

    /* if i is a used direction */
    if ((i == 0 && new_pos[i] == 1) || (new_pos[i] != new_pos[i - 1])) {

      /* reassign i to its new position */
      const int new_i = new_pos[i] - 1;
      directions[new_i] = directions[i];
      LB[new_i] = LB[i];
      UB[new_i] = UB[i];
    }
  }

  /* resize all the vectors */
  directions.resize(num_of_directions);
  LB.resize(num_of_directions);
  UB.resize(num_of_directions);

  /* re-map the template matrix */

  std::set<std::vector<unsigned int>> output;
  for (const std::vector<unsigned int> &T: templates) {
    auto new_T = T;
    for (unsigned int &dir: new_T) {
      dir = new_pos[dir] - 1;
    }

    output.insert(new_T);
  }

  std::set<size_t> new_adaptive_directions;
  for (const auto &dir: adaptive_directions) {
    new_adaptive_directions.insert(new_pos[dir] - 1);
  }
  std::swap(adaptive_directions, new_adaptive_directions);

  return output;
}

size_t find_linearly_dependent_row(const std::vector<std::vector<double>> &A,
                                   const std::vector<double> &v)
{
  for (size_t i = 0; i < A.size(); ++i) {
    if (LinearAlgebra::are_linearly_dependent(A[i], v)) {
      return i;
    }
  }

  return A.size();
}

std::set<std::vector<unsigned int>>
get_templates(const InputData &id, const std::vector<size_t> &template_id)
{
  std::set<std::vector<unsigned int>> templates;

  // map templates on the filtered directions set
  for (std::vector<unsigned int> b_template: id.getTemplate()) {
    for (auto &dir_id: b_template) {
      dir_id = template_id[dir_id];
    }

    templates.insert(b_template);
  }

  return templates;
}

/**
 * @brief Collect directions and boundaries
 *
 * This method collects directions and boundaries and removes
 * all the linearly equivalent directions. This method
 * returns the template set.
 *
 * @param[out] directions is the vector of the collected directions
 * @param[out] LB is the vector of the lower bounds
 * @param[out] UB is the vector of the upper bounds
 * @param[in] id is the input data object
 * @return the template set
 */
std::set<std::vector<unsigned int>>
collect_constraints(std::vector<std::vector<double>> &directions,
                    std::vector<double> &LB, std::vector<double> &UB,
                    const InputData &id)
{
  using namespace LinearAlgebra;

  auto variables = id.getVarSymbols();

  std::vector<size_t> template_ids(id.getDirectionsNum());

  // get directions and boundaries from input data
  for (unsigned i = 0; i < id.getDirectionsNum(); i++) {
    auto dir = id.getDirection(i)->get_variable_coefficients(variables);
    auto pos = find_linearly_dependent_row(directions, dir);

    template_ids[i] = pos;
    if (pos == directions.size()) {
      directions.push_back(std::move(dir));
      UB.push_back(id.getDirection(i)->get_upper_bound());
      LB.push_back(id.getDirection(i)->get_lower_bound());
    } else { // not necessary if we assume that bound
             // optimization has been performed
      double coeff = directions[pos] / dir;

      auto new_UB = coeff * id.getDirection(i)->get_upper_bound();
      auto new_LB = coeff * id.getDirection(i)->get_lower_bound();

      if (coeff < 0) {
        std::swap(new_UB, new_LB);
      }

      if (LB[pos] > new_LB) {
        LB[pos] = new_LB;
      }

      if (UB[pos] < new_UB) {
        UB[pos] = new_UB;
      }
    }
  }

  return get_templates(id, template_ids);
}

Bundle getBundle(const InputData &id)
{
  using namespace LinearAlgebra;

  std::vector<std::vector<double>> directions;
  std::vector<double> LB, UB;
  std::set<size_t> adaptive_directions = id.getAdaptiveDirections();

  // the following call also filter linearly dependent direction.
  // These should be already removed by InputData methods, but
  // better check twice that none
  auto templates = collect_constraints(directions, LB, UB, id);

  /* if users have specified at least one template, ... */
  if (templates.size() > 0) {
    /* ... they really want to exclusively use those templates.
       Thus, trim the unused directions. */

    /* Why can't simply uses the template themselve?
       Because Bundle:transform in AFO mode assumes that
       each of the directions belongs to a template at least. */
    templates = trim_unused_directions(directions, LB, UB, adaptive_directions,
                                       templates);
  }

  Bundle bundle = Bundle(directions, LB, UB, templates, adaptive_directions);

  if (id.getUseInvariantDirections()) {
    const auto variables = id.getVarSymbols();
    auto ls = getConstraintsSystem(id.getInvariant(), variables);

    bundle.intersect_with(ls);
  }

  return bundle;
}

std::vector<Expression<>> &
compose_dynamics(const std::vector<Symbol<>> &variables,
                 std::vector<Expression<>> &dynamics,
                 const unsigned int dynamic_degree)
{
  // replace each variable with its dynamic law
  SymbolicAlgebra::Expression<>::replacement_type rep_symb;
  for (unsigned var = 0; var < variables.size(); var++) {
    rep_symb[variables[var]] = dynamics[var];
  }

  for (unsigned i = 1; i < dynamic_degree; i++) {
    for (auto &dyn: dynamics) {
      dyn.replace(rep_symb);
    }
  }

  return dynamics;
}

SetsUnion<Polytope> getParameterSet(const InputData &id)
{
  if (id.paramDirectionsNum() == 0) {
    return SetsUnion<Polytope>();
  }

  vector<vector<double>> pA(2 * id.paramDirectionsNum(),
                            vector<double>(id.getParamNum(), 0));
  for (unsigned i = 0; i < id.paramDirectionsNum(); i++) {
    pA[2 * i] = id.getParamDirection(i)->get_variable_coefficients(
        id.getParamSymbols());
    for (unsigned j = 0; j < id.getParamNum(); j++)
      pA[2 * i + 1][j] = -pA[2 * i][j];
  }

  vector<double> pb(pA.size(), 0);
  for (unsigned i = 0; i < id.paramDirectionsNum(); i++) {
    pb[2 * i] = id.getParamDirection(i)->get_upper_bound();
    pb[2 * i + 1] = -id.getParamDirection(i)->get_lower_bound();
  }

  return SetsUnion<Polytope>(Polytope(pA, pb));
}

DiscreteSystem<double> get_integrated_dynamics(const InputData &id,
                                               const ODE<double> &ode)
{
  if (!id.isIntegrationStepSet()) {
    std::cerr << "Integration step is required for ODEs" << std::endl;
    exit(1);
  }

  InputData::IntegratorType integrator_type;
  if (id.isIntegratorTypeSet()) {
    integrator_type = id.getIntegratorType();
  } else {
    std::cerr << "Integator not specified. Using Euler method." << std::endl;
    integrator_type = InputData::EULER;
  }

  switch (integrator_type) {
  case InputData::EULER: {
    EulerIntegrator euler;
    return euler(ode, id.getIntegrationStep());
  } break;
  case InputData::RUNGE_KUTTA_4: {
    RungeKutta4Integrator rk4;
    return rk4(ode, id.getIntegrationStep());
  }
  default:
    SAPO_ERROR("Unsupported intergrator", std::domain_error);
  }
}

Model *get_model(const InputData &id)
{
  using namespace SymbolicAlgebra;

  std::string name = "AutoGenerated";

  // variables
  auto variables = id.getVarSymbols();

  // parameters
  auto parameters = id.getParamSymbols();

  // dynamics
  std::vector<Expression<>> dynamics(id.getVarNum());
  for (unsigned v = 0; v < id.getVarNum(); v++) {
    dynamics[v] = id.getVar(v)->getDynamic();
  }

  // compose dynamics
  if (id.isDynamicCompositionEnabled() && id.getDynamicDegree() > 1) {
    compose_dynamics(variables, dynamics, id.getDynamicDegree());
  }

  if (id.getDirectionsNum()<id.getVarNum()) {
    std::cerr << "Not enough bundle directions: the directions "
              << "must be as many as the variables at least"
              << std::endl;

    return nullptr;
  }

  Bundle init_set = getBundle(id);

  SetsUnion<Polytope> param_set = getParameterSet(id);

  Model *model;

  if (id.getSpecificationType() == InputData::DISCRETE) {
    model = new DiscreteModel(variables, parameters, dynamics, init_set,
                              param_set);
  } else {
    SymbolicAlgebra::Symbol time("time");
    ODE<double> ode(variables, parameters, dynamics, time);

    DiscreteSystem<double> ds = get_integrated_dynamics(id, ode);

    for (size_t i = 0; i < variables.size(); ++i) {
      if (getDegree(ds.dynamics()[i], id.getParamSymbols()) > 1) {
        std::cerr << "The solution of the ODE for \"" << variables[i] << "\" ("
                  << ds.dynamics()[i] << ") is not linear in the parameters "
                  << std::set(parameters.begin(), parameters.end())
                  << std::endl;
        exit(1);
      }
    }

    model = new DiscreteModel(ds.variables(), ds.parameters(), ds.dynamics(),
                              init_set, param_set);
  }

  if (id.isSpecDefined()) {
    model->set_specification(id.specification());
  }

  auto ls = getConstraintsSystem(id.getAssumptions(), variables);

  model->set_assumptions(ls);

  ls = getConstraintsSystem(id.getInvariant(), variables);
  model->set_invariant(ls);

  return model;
}
