/**
 * @file main.cpp
 * main: This main file reproduces the experiments reported in "Sapo:
 * Reachability Computation and Parameter Synthesis of Polynomial Dynamical
 * Systems"
 * @author Tommaso Dreossi <tommasodreossi@berkeley.edu>
 * @version 0.1
 */

#include <fstream>
#include <iostream>
#include <sstream>

#include <SapoThreads.h>
#include <Bundle.h>
#include <Sapo.h>
#include <Version.h>

#include <ProgressAccounter.h>

#include "AutoGenerated.h"
#include "driver.h"


using namespace std;

#define BAR_LENGTH 50

Sapo init_sapo(const Model& model, const AbsSyn::InputData &data,
               const unsigned int num_of_presplits)
{
  Sapo sapo(model);

  sapo.tmode = (data.getTransValue() == AbsSyn::transType::OFO ? Bundle::OFO
                                                               : Bundle::AFO);
  sapo.decomp = data.getDecomposition() ? 1 : 0;
  sapo.decomp_weight = data.getAlpha();
  sapo.time_horizon = data.getIterations();
  sapo.max_param_splits = data.getMaxParameterSplits();
  if (data.isPreSplitsSet()) {
    sapo.num_of_presplits = num_of_presplits;
  } else {
    sapo.num_of_presplits = 0;
  }
  sapo.max_bundle_magnitude = data.getMaxVersorMagnitude();

  return sapo;
}

template<typename OSTREAM>
void print_symbol_vector(OSTREAM &os,
                         const std::vector<SymbolicAlgebra::Symbol<>> &vect)
{
  using OF = OutputFormatter<OSTREAM>;

  os << OF::short_list_begin();
  bool not_first = false;
  for (auto v_it = std::begin(vect); v_it != std::end(vect); ++v_it) {
    if (not_first) {
      os << OF::short_list_separator();
    } else {
      not_first = true;
    }
    {
      ostringstream buffer;
      buffer << "\"" << *v_it << "\"";

      os << buffer.str();
    }
  }
  os << OF::short_list_end();
}

template<typename OSTREAM>
void print_variables_and_parameters(OSTREAM &os, const Model& model)
{
  using OF = OutputFormatter<OSTREAM>;

  os << OF::field_header("variables");
  print_symbol_vector(os, model.variables());
  os << OF::field_footer();
  if (model.parameters().size() != 0) {
    os << OF::field_separator() << OF::field_header("parameters");
    print_symbol_vector(os, model.parameters());
    os << OF::field_footer();
  }
}

template<typename OSTREAM>
void reach_analysis(OSTREAM &os, Sapo &sapo, const Model& model,
                    const bool display_progress)
{
  using OF = OutputFormatter<OSTREAM>;

  os << OF::object_header();
  print_variables_and_parameters(os, model);
  os << OF::field_separator() << OF::field_header("data");

  os << OF::list_begin() << OF::object_header()
     << OF::field_header("flowpipe");

  ProgressAccounter *accounter = NULL;
  if (display_progress) {
    accounter = (ProgressAccounter *)new ProgressBar(
        sapo.time_horizon, BAR_LENGTH, std::ref(std::cerr));
  }

  // if the model does not specify any parameter set
  if (model.parameters().size() == 0) {

    // perform the reachability analysis
    os << sapo.reach(*(model.initial_set()), sapo.time_horizon, accounter);
  } else {

    // perform the parametric reachability analysis
    os << sapo.reach(*(model.initial_set()), model.parameter_set(),
                     sapo.time_horizon, accounter);
  }

  os << OF::field_footer() << OF::object_footer() << OF::list_end()
     << OF::field_footer() << OF::object_footer();

  if (display_progress) {
    delete accounter;
  }
}

template<typename OSTREAM>
void output_synthesis(OSTREAM &os, const Model& model,
                      const std::list<PolytopesUnion> &synth_params,
                      const std::vector<Flowpipe> &flowpipes)
{
  using OF = OutputFormatter<OSTREAM>;

  os << OF::object_header();
  print_variables_and_parameters(os, model);
  os << OF::field_separator() << OF::field_header("data");

  if (every_set_is_empty(synth_params)) {
    os << OF::empty_list();
  } else {
    os << OF::list_begin();
    bool not_first = false;
    unsigned int params_idx = 0;
    for (auto p_it = std::cbegin(synth_params);
         p_it != std::cend(synth_params); ++p_it) {
      if (p_it->size() != 0) {
        if (not_first) {
          os << OF::list_separator();
        }
        not_first = true;
        os << OF::object_header() << OF::field_header("parameter set") << *p_it
           << OF::field_footer() << OF::field_separator()
           << OF::field_header("flowpipe") << flowpipes[params_idx++]
           << OF::field_footer() << OF::object_footer();
      } else {
        ++params_idx;
      }
    }
    os << OF::list_end();
  }
  os << OF::field_footer() << OF::object_footer();
}

unsigned int get_max_steps(const Sapo &sapo, const Model& model)
// const unsigned int &max_param_splits, const unsigned int &num_of_params,
// const unsigned int &time_horizon)
{
  unsigned int max_steps = 0;
  const unsigned int num_of_params = model.parameters().size();

  for (unsigned int splits = 0; splits <= sapo.max_param_splits; ++splits) {
    max_steps += std::pow(1 << splits, num_of_params);
  }

  return max_steps * model.specification()->time_bounds().end()
         + sapo.time_horizon
               * std::pow(1 << sapo.max_param_splits, num_of_params);
}

template<typename OSTREAM>
void synthesis(OSTREAM &os, Sapo &sapo, const Model& model,
               const bool display_progress)
{
  ProgressAccounter *accounter = NULL;
  unsigned int max_steps = 0;
  if (display_progress) {
    max_steps = get_max_steps(sapo, model);

    accounter = (ProgressAccounter *)new ProgressBar(max_steps, BAR_LENGTH,
                                                     std::ref(std::cerr));
  }

  // Synthesize parameters
  std::list<PolytopesUnion> synth_params = sapo.synthesize(
      *(model.initial_set()), model.parameter_set(), model.specification(),
      sapo.max_param_splits, sapo.num_of_presplits, accounter);

  if (display_progress) {
    accounter->increase_performed_to(
        max_steps - sapo.time_horizon * synth_params.size());
  }

  std::vector<Flowpipe> flowpipes(synth_params.size());

  if (!every_set_is_empty(synth_params)) {
#ifdef WITH_THREADS
    auto compute_reachability
        = [&flowpipes, &sapo, &model, &accounter](const PolytopesUnion &pSet,
                                                  unsigned int params_idx) {
            flowpipes[params_idx] = sapo.reach(*(model.initial_set()), pSet,
                                               sapo.time_horizon, accounter);
          };

    ThreadPool::BatchId batch_id = thread_pool.create_batch();

    unsigned int res_idx = 0;
    for (auto p_it = std::cbegin(synth_params);
         p_it != std::cend(synth_params); ++p_it) {
      // submit the task to the thread pool
      thread_pool.submit_to_batch(
          batch_id,
          std::bind(compute_reachability, std::ref(*p_it), res_idx++));
    }

    // join to the pool threads
    thread_pool.join_threads(batch_id);

    // close the batch
    thread_pool.close_batch(batch_id);
#else
    unsigned int params_idx = 0;
    for (auto p_it = std::cbegin(synth_params);
         p_it != std::cend(synth_params); ++p_it) {
      flowpipes[params_idx++] = sapo.reach(*(model.initial_set()), *p_it,
                                           sapo.time_horizon, accounter);
    }
#endif
  }

  if (display_progress) {
    accounter->increase_performed_to(max_steps);

    delete accounter;
  }

  output_synthesis(os, model, synth_params, flowpipes);
}

template<typename OSTREAM>
void perform_computation_and_get_output(OSTREAM &os, Sapo &sapo, const Model& model,
                                        const AbsSyn::problemType &type,
                                        const bool display_progress)
{
  try {
    switch (type) {
    case AbsSyn::problemType::REACH:
      reach_analysis(os, sapo, model, display_progress);
      break;
    case AbsSyn::problemType::SYNTH:
      synthesis(os, sapo, model, display_progress);
      break;
    default:
      std::cerr << "Unsupported problem type" << std::endl;
      exit(EXIT_FAILURE);
    }
  } catch (std::exception &e) {
    std::cerr << e.what() << std::endl;

    exit(EXIT_FAILURE);
  }
}

struct prog_opts {
  std::string input_filename;
  bool JSON_output;
  bool get_help;
  bool progress;
  unsigned int num_of_threads;
};

void print_help(std::ostream &os, const std::string exec_name)
{
  os << "Sapo " << sapo_version << std::endl
     << "Usage: " << exec_name << " [options] [input filename]" << std::endl
     << "Options:" << std::endl
     << "  -j\t\t\t\tGet the output in JSON format" << std::endl
#ifdef WITH_THREADS
     << "  -t [num of active threads]\tEnable multi-threading and set the "
     << "number of " << std::endl
     << "\t\t\t\t  active threads (default: "
     << std::thread::hardware_concurrency() << ")" << std::endl
#endif
     << "  -b\t\t\t\tDisplay a progress bar" << std::endl
     << "  -h\t\t\t\tPrint this help" << std::endl
     << std::endl
     << "If either the filename is \"-\" or no filename is provided, "
     << "the input is taken " << std::endl
     << " from the standard input." << std::endl;
}

bool is_number(const char *str)
{
  unsigned int i = 0;
  while (str[i] != '\0') {
    if (!std::isdigit(str[i])) {
      return false;
    }

    i++;
  }

  return true;
}

void parser_option(prog_opts &opts, const int argc, char **argv, int &arg_pos)
{
  std::string argv_str = std::string(argv[arg_pos]);
  if (std::string("-h") == argv_str) {
    opts.get_help = true;
    return;
  }
  if (std::string("-j") == argv_str) {
    opts.JSON_output = true;
    return;
  }
  if (std::string("-b") == argv_str) {
    opts.progress = true;
    return;
  }
#ifdef WITH_THREADS
  if (std::string("-t") == argv_str) {
    if (arg_pos + 1 < argc && is_number(argv[arg_pos + 1])) {
      opts.num_of_threads = atoi(argv[++arg_pos]);
    } else {
      opts.num_of_threads = std::thread::hardware_concurrency();
    }
    return;
  }
#else
  (void)argc;
#endif

  opts.input_filename = argv_str;
}

prog_opts parse_opts(const int argc, char **argv)
{
  prog_opts opts = {"-", false, false, false, 1};

#ifdef WITH_THREADS
  if (argc > 6) {
#else
  if (argc > 4) {
#endif
    std::cerr << "Syntax error: Too many parameters" << std::endl;
    print_help(std::cerr, argv[0]);

    exit(EXIT_FAILURE);
  }

  for (int i = 1; i < argc; i++) {
    parser_option(opts, argc, argv, i);
  }

  return opts;
}

int main(int argc, char **argv)
{
  driver drv;
  string file;

  prog_opts opts = parse_opts(argc, argv);
#ifdef WITH_THREADS
  // add all the aimed threads, but the current
  // one to the thread pool
  thread_pool.reset(opts.num_of_threads - 1);
#endif

  if (opts.get_help) {
    print_help(std::cout, argv[0]);

    exit(EXIT_SUCCESS);
  }

  //  drv.trace_parsing = true;

  if (drv.parse(opts.input_filename) != 0) {
    std::cerr << "Error in loading " << opts.input_filename << std::endl;
    exit(EXIT_FAILURE);
  }

  Model model = get_model(drv.data);

#ifdef WITH_THREADS
  Sapo sapo = init_sapo(model, drv.data, opts.num_of_threads);
#else
  Sapo sapo = init_sapo(model, drv.data, 0);
#endif

  if (opts.JSON_output) {
    JSON::ostream os(std::cout);
    perform_computation_and_get_output(os, sapo, model, drv.data.getProblem(),
                                       opts.progress);
  } else {
    perform_computation_and_get_output(std::cout, sapo, model,
                                       drv.data.getProblem(), opts.progress);
  }

  exit(EXIT_SUCCESS);
}
