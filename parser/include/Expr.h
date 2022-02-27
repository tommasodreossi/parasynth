#ifndef __EXPR_H__
#define __EXPR_H__

#include <vector>
#include <algorithm>
#include <set>

#include "SymbolicAlgebra.h"

using namespace SymbolicAlgebra;
using namespace std;

namespace AbsSyn
{

// checks if the expression contains at least one of the symbols
inline bool contains(const Expression<> &e, const vector<Symbol<>> &symbols)
{
	std::set<Symbol<>> ids = e.get_symbols();
	
	for (auto it = ids.begin(); it != ids.end(); it++) {
		for (auto sym = symbols.begin(); sym != symbols.end(); sym++) {
			if (it->get_symbol_name(it->get_id()) == sym->get_symbol_name(sym->get_id())) {
				return true;
			}
		}
	}
	return false;
}

// returns the degree of the expression, considering only symbols as variables
inline unsigned getDegree(const Expression<> &e, std::vector<Symbol<>> symbols)
{
	// find an unused name
	std::set<Symbol<>> ids = e.get_symbols();
	
	string s = "a", name = s;
	unsigned n = 0;
	bool found = true;
	
	while (found) {
		n++;
		name = s;
		name += n;
		found = false;
		for (auto it = ids.begin(); it != ids.end(); it++) {
			if (Symbol<>::get_symbol_name(it->get_id()) == name) {
				found = true;
				break;
			}
		}
	}
	
	Symbol<> new_symbol(name);
	
	Expression<>::replacement_type rep {};
	
	for (auto it = symbols.begin(); it != symbols.end(); it++) {
		rep[*it] = new_symbol;
	}
	
	Expression<> temp(e);
	temp.replace(rep);
	
	return temp.degree(new_symbol);
}

// checks if the expression is numeric (no vars or parameters)
inline bool isNumeric(const Expression<> &e)
{
	return e.get_symbols().size() == 0;
}

/*
 * returns the coefficient of the symbol "name"
 * Applies only to linear expressions w.r.t the symbol specified
 */
inline double getCoefficient(const Expression<> &e, const Symbol<> &s)
{
	// avoid -0
	double val = e.get_coeff(s, 1).evaluate<double>();
	return val == 0 ? 0 : val;
}

/*
 * returns the numerical term
 */
inline double getOffset(const Expression<> &e)
{
	Expression<> temp(e);
	
	std::set<Symbol<>> ids = e.get_symbols();
	Expression<>::replacement_type rep{};
	
	for (auto it = ids.begin(); it != ids.end(); it++) {
		rep[*it] = 0;
	}
	
	temp.replace(rep);
	
	// avoid -0
	double val = temp.evaluate<double>();
	return val == 0 ? 0 : val;
}

// simplify an expanded expression
inline Expression<> simplify(Expression<> e)
{
	std::set<Symbol<>> ids = e.get_symbols();
	
	if (ids.size() == 0) {
		return e.evaluate<double>();
	} else {
		// symbol we operate on
		Symbol<> s = *ids.begin();
		
		// accumulator for result
		Expression<> res = 0;
		
		// coefficients for symbol s
		std::map<int, Expression<>> coeffs = e.get_coeffs(s);

		for (auto it = coeffs.begin(); it != coeffs.end(); it++) {
			// compute the corresponding power of s
			Expression<> power = 1;
			for (int i = 0; i < it->first; i++) {
				power *= s;
			}
			res += power * simplify(it->second);
		}
		
		res.expand();
		
		return res;
	}
}


}

#endif