/**
 * @file Model.h
 * Represent a discrete-time (eventually parameteric) dynamical system
 *
 * @author Tommaso Dreossi <tommasodreossi@berkeley.edu>
 * @version 0.1
 */

#ifndef MODEL_H_
#define MODEL_H_

#include "Bundle.h"
#include "Common.h"

class Model {

protected:
	lst vars;		// variables
	lst params;		// parameters
	lst dyns;		// dynamics
	Bundle *reachSet; // Initial reach set

public:

	lst getVars(){ return this->vars; }
	lst getParams(){ return this->params; }
	lst getDyns(){ return this->dyns; }
	Bundle* getReachSet(){ return this->reachSet; }

};

#endif /* MODEL_H_ */