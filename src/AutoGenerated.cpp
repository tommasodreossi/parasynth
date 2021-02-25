#include "AutoGenerated.h"

using namespace AbsSyn;

ex toEx(Expr *e, InputModel& m, const lst& vars, const lst& params);		// converts an Expr to a GiNaC ex

AutoGenerated::AutoGenerated(InputModel m){ 

	strcpy(this->name,"AutoGenerated");

	lst vars, params, dyns;
	for (unsigned i = 0; i < m.getVarNum(); i++)
		vars.append(symbol(m.getVar(i)->getName()));

	for (unsigned i = 0; i < m.getParamNum(); i++)
		params.append(symbol(m.getParam(i)->getName()));

	for (unsigned i = 0; i < m.getVarNum(); i++)
		dyns.append(m.getVar(i)->getDynamic()->toEx(m, vars, params));

	
	// symbols
	this->vars = vars;
	this->params = params;
	this->dyns = dyns;
/*	
	cout << "vars = " << vars << endl;
	cout << "params = " << params << endl;
	cout << "dyns = " << dyns << endl;
*/	
	// variable directions
	vector<double> LBs = m.getLB();
	transform(LBs.begin(), LBs.end(), LBs.begin(), negate<double>());
	
	vector<vector<int>> templ(1, vector<int>{1,0});
	this->reachSet = new Bundle(m.getDirections(), m.getUB(), LBs, m.getTemplate());
//	this->reachSet = new Bundle(m.getDirections(), m.getUB(), LBs, templ);
/*	
	cout << "directions: {" << endl;
	for (unsigned i = 0; i < m.directionsNum(); i++)
	{
		cout << "{";
		for (unsigned j = 0; j < m.getVarNum(); j++)
			cout << m.getDirections()[i][j] << ", ";
		cout << "}" << endl;
	}
	
	cout << "LBs: {";
	for (unsigned i = 0; i < m.directionsNum(); i++)
		cout << LBs[i] << ", ";
	cout << "}" << endl;
	
	cout << "UBs: {";
	for (unsigned i = 0; i < m.directionsNum(); i++)
		cout << m.getUB()[i] << ", ";
	cout << "}" << endl;
	
	cout << "template: {" << endl;
	for (unsigned i = 0; i < m.templateRows(); i++)
	{
		cout << "{";
		for (unsigned j = 0; j < m.templateCols(); j++)
			cout << m.getTemplate()[i][j] << ", ";
		cout << "}" << endl;
	}
*/	
	
	
	
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
/*		
		cout << "pA = {" << endl;
		for (unsigned i = 0; i < pA.size(); i++)
		{
			cout << "{";
			for (unsigned j = 0; j < pA[i].size(); j++)
				cout << pA[i][j] << ", ";
			cout << "}" << endl;
		}
		cout << "}" << endl;
		
		cout << "pb = {";
			for (unsigned j = 0; j < pb.size(); j++)
				cout << pb[j] << ", ";
			cout << "}" << endl;
*/		
		this->paraSet = new LinearSystemSet(new LinearSystem(pA, pb));
	}
	
	
	// formula
	if (m.isSpecDefined())
	{
		this->spec = m.getSpec()->toSTL(m, vars, params);
		//STL *f = this->spec->getSubFormula();
		cout << "spec = ";
		this->spec->print();
		cout << endl;
	}
	else
	{
		cout << "no spec" << endl;
		this->spec = NULL;
	}
}
