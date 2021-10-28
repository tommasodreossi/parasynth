#include "AutoGenerated.h"

using namespace AbsSyn;

ex toEx(Expr *e, InputData& m, const lst& vars, const lst& params);		// converts an Expr to a GiNaC ex

AutoGenerated::AutoGenerated(const InputData& m) {

	this->name = "AutoGenerated";

	for (unsigned i = 0; i < m.getVarNum(); i++)
		this->vars.append(symbol(m.getVar(i)->getName()));

	for (unsigned i = 0; i < m.getParamNum(); i++)
		this->params.append(symbol(m.getParam(i)->getName()));

	for (unsigned i = 0; i < m.getVarNum(); i++)
		this->dyns.append(m.getVar(i)->getDynamic()->toEx(m, this->vars, this->params));
	
	this->reachSet = new Bundle(m.getDirections(), m.getUB(),
							    get_complementary(m.getLB()), m.getTemplate());	
	
	// parameter directions
	if (m.paramDirectionsNum() != 0)
	{
		vector<vector<double>> pA(2 * m.paramDirectionsNum(), vector<double>(m.getParamNum(), 0));
		for (unsigned i = 0; i < m.paramDirectionsNum(); i++)
		{
			pA[2*i] = m.getParamDirection(i);
			for (unsigned j = 0; j < m.getParamNum(); j++)
				pA[2*i+1][j] = -pA[2*i][j];
		}
		
		vector<double> pb(pA.size(), 0);
		for (unsigned i =0; i < m.paramDirectionsNum(); i++)
		{
			pb[2*i] = m.getParamUB()[i];
			pb[2*i+1] = -m.getParamLB()[i];
		}

		this->paraSet = new LinearSystemSet(std::make_shared<LinearSystem>(pA, pb));
	}
	
	
	// formula
	if (m.isSpecDefined())
	{
		this->spec = m.getSpec()->toSTL(m, vars, params);
	} else {
		this->spec = NULL;
	}
}
