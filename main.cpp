/*
 * main.cpp
 *
 *  Created on: May 20, 2014
 *      Author: Tommaso Dreossi
 */

#include <stdio.h>
#include <iostream>

#include "Common.h"
#include "BaseConverter.h"
#include "LinearSystem.h"
#include "Polyhedron.h"
#include "Parallelotope.h"
#include "Box.h"
#include "ConvexHull.h"

#include "DynamicalSystem.h"
#include "DiscreteDynamicalSystem.h"

#include "ParameterSynthesizer.h"
#include "Reacher.h"

#include "STL.h"
#include "Atom.h"
#include "Conjunction.h"
#include "Disjunction.h"
#include "Until.h"

#include "Ebola.h"
#include "SIRconv.h"

using namespace std;

int main(int argc,char** argv){

	// Ebola model
	Model *model = new Ebola();

	lst vars, params, dyns;
	vars = model->getVars();
	params = model->getParams();
	dyns = model->getDyns();

	cout<<"Model name: "<<model->getName()<<"\n";
	cout<<"Dynamics:\n";
	for(int i=0; i<dyns.nops(); i++){
		cout<<"\td"<<vars[i]<<" = "<<dyns[i]<<"\n";
	}
	cout<<"Specification:";
	model->getSpec()->print();
	cout<<"\n\n";


	// Create a new dynamic system with model's dynamics
	DynamicalSystem *cont_ebola = new DynamicalSystem(vars,params,dyns);
	DiscreteDynamicalSystem *disc_ebola = new DiscreteDynamicalSystem(vars,params,cont_ebola->eulerDisc(0.5), model->getReachSet());


	// Initializer the initial set
	// base vertex
	vector< double > base_vertex (5,0);
	base_vertex[0] = 800;
	base_vertex[1] = 0;
	base_vertex[2] = 0;
	base_vertex[3] = 200;
	base_vertex[4] = 0;

	// and versors lenghts
	vector< double > lenghts (5,0);
	lenghts[0] = 0.001;
	lenghts[1] = 0.001;
	lenghts[2] = 0.001;
	lenghts[3] = 0.001;
	lenghts[4] = 0.001;


	// Create the parameter synthesizer
	ParameterSynthesizer *PS = new ParameterSynthesizer(disc_ebola,model->getSpec());

	// Synthesize
	cout<<"\nStart synthesis...";
	clock_t tStart = clock();

	LinearSystemSet *result = PS->synthesizeSTL(base_vertex,lenghts,model->getInitParaSet());

	printf("\nTime taken: %.2fs\n", (double)(clock() - tStart)/CLOCKS_PER_SEC);

	if(result->isEmpty()){
		cout<<"Result: Empty parameter set.";
	}else{
		cout<<"Result: Parameter set composed by "<<result->size()<<" convex polytopes. ";
		cout<<"Print result? (y/n): ";
		char print_res;
		cin >> print_res;
		if(print_res == 'y'){
			result->print();
		}
	}

//	vector<double> pts;
//	ConvexHull *reach_set = new ConvexHull(3,pts);
//
//	// SIR model
//	SIRconv *sir = new SIRconv();
//	Model *model = sir;
//
//	lst vars, params, dyns;
//	vars = model->getVars();
//	params = model->getParams();
//	dyns = model->getDyns();
//
//	cout<<"Model name: "<<model->getName()<<"\n";
//	cout<<"Dynamics:\n";
//	for(int i=0; i<dyns.nops(); i++){
//		cout<<"\td"<<vars[i]<<" = "<<dyns[i]<<"\n";
//	}
//
//	// Create a new dynamic system with model's dynamics
//	DynamicalSystem *cont_SIR = new DynamicalSystem(vars,params,dyns);
//	DiscreteDynamicalSystem *disc_SIR = new DiscreteDynamicalSystem(vars,params,cont_SIR->eulerDisc(0.1));
//
//	cout<<"In main";
//
//	Reacher *R = new Reacher(disc_SIR, reach_set);
////	R->reach(1);
//	cout<<"done";




}
