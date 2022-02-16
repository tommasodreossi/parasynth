%skeleton "lalr1.cc" // -*- C++ -*-
%require "3.5.1"
%defines

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
	#include <string>
	#include "AbsSynIO.h"
	
	#include "Expr.h"
	
	#include "STL.h"
	#include "Atom.h"
	#include "Conjunction.h"
	#include "Disjunction.h"
	#include "Negation.h"
	#include "Always.h"
	#include "Eventually.h"
	#include "Until.h"
	
	#include "types.h"
	#include "Variable.h"
	#include "Parameter.h"
	#include "Constant.h"
	#include "Definition.h"
	#include "Direction.h"
	#include "InputData.h"

	class driver;
}

// The parsing context.
%param { driver& drv }

%locations
%define api.location.file "../include/locations.h"

%define parse.trace
%define parse.error verbose

%code {
#include "driver.h"
}

%define api.token.prefix {TOK_}
%token
	END 0
	PROB 
	REACH
	SYNTH
	VARMODE
	PARAMMODE
	BOX
	PARAL
	POLY
	VAR
	PARAM
	CONST
	DEFINE
	IN
	DYN
	SPEC
	ASSUME
	ITER
	PSPLITS
	PRESPLITS
	MAX_MAGNITUDE
	DIR
	TEMPL
	PDIR
	OPT
	TRANS
	AFO
	OFO
	DECOMP
	ALPHA
	AND					"&&"
	OR					"||"
	NOT					"!"
	LE					"<="
	GE					">="
	EQ					"="
	ALWAYS			"G"
	EVENTUALLY	"F"
	UNTIL				"U"
	MINUS				"-"
	PLUS				"+"
	STAR				"*"
	POW					"^"
	DIV					"/"
	LPAREN			"("
	RPAREN			")"
	LSQUARE			"["
	RSQUARE			"]"
	LBRACE			"{"
	RBRACE			"}"
	LANGLE			"<"
	RANGLE			">"
	COLON				":"
	SEMICOLON		";"
	COMMA				","
;

%token <std::string> IDENT
%token <std::string> ON
%token <std::string> OFF
%token <int> INTEGER
%token <double> DOUBLE

%left "&&"
%left "||"
%left "!"
%left "F"
%left "G"
%left "U"

%left "+" "-"
%left "*" "/"
%right "^"
%left UMINUS

%nterm <AbsSyn::problemType> problemType
%nterm <AbsSyn::modeType> modeType
%nterm <std::pair<double, double>> doubleInterval
%nterm <std::pair<int, int>> intInterval
%nterm <double> number
%nterm <std::vector<std::string>> identList
%nterm <SymbolicAlgebra::Expression<>> expr
%nterm <std::vector<int>> matrixRow
%nterm <std::vector<std::vector<int>>> rowList
%nterm <std::shared_ptr<STL> > formula
%nterm <AbsSyn::transType> transType
%nterm <AbsSyn::Direction *> direction
%nterm <AbsSyn::Direction::Type> directionType

%printer { yyo << $$; } <*>;

%start s

%%

s		: statement
		| s statement
		| END { yy::parser::error(@1, "Empty file"); YYERROR; }

statement		: header {}
						| symbol {}
						| matrices {}
						| option {}

header			: PROB ":" problemType ";"
						{
							if (drv.data.isProblemDefined())
							{
								yy::parser::error(@4, "Problem has already been defined");
								YYERROR;
							}
							drv.data.setProblem($3);
						}
						| PARAMMODE ":" modeType ";"
						{
							yy::parser::error(@$, "Parameter modality is deprecated and will be ignored");
						}
						| VARMODE ":" modeType ";"
						{
							yy::parser::error(@$, "Variable modality is deprecated and will be ignored");
						}
						| ITER ":" INTEGER ";"
						{
							if (drv.data.isIterationSet())
							{
								yy::parser::error(@4, "Iteration number already defined");
								YYERROR;
							}
							
							drv.data.setIterations($3);
						}
						| PSPLITS ":" INTEGER ";"
						{
							if (drv.data.getMaxParameterSplits() > 0)
							{
								yy::parser::error(@4, "The maximum number of parameter splits has been already defined");
								YYERROR;
							}

							drv.data.setMaxParameterSplits($3);
						}
						| PRESPLITS ":" ON ";"
						{
							drv.data.setPreSplits(true);
						}
						| PRESPLITS ":" OFF ";"
						{
							drv.data.setPreSplits(false);
						}
						| MAX_MAGNITUDE ":" DOUBLE ";"
						{
							drv.data.setMaxVersorMagnitude($3);
						}

symbol			: VAR identList IN doubleInterval ";"
						{
							for (unsigned i = 0; i < $2.size(); i++)
							{
								if (drv.data.isSymbolDefined($2[i]))
								{
									yy::parser::error(@2, "Symbol '" + $2[i] + "' already defined");
									YYERROR;
								}
								
								SymbolicAlgebra::Symbol<> s($2[i]);
								drv.data.addVariable(new AbsSyn::Variable(s));
								drv.ctx.addVariable(s);
								
								AbsSyn::Direction *d = new AbsSyn::Direction(s, 0, AbsSyn::Direction::Type::INT, $4.first, $4.second, "default_" + $2[i]);
								drv.data.addDirectionConstraint(d, drv.ctx);
							}
						}
						| VAR identList ";"
						{
							for (unsigned i = 0; i < $2.size(); i++)
							{
								if (drv.data.isSymbolDefined($2[i]))
								{
									yy::parser::error(@2, "Symbol '" + $2[i] + "' already defined");
									YYERROR;
								}
								SymbolicAlgebra::Symbol<> s($2[i]);
								drv.data.addVariable(new AbsSyn::Variable(s));
								drv.ctx.addVariable(s);
							}
						}
						| PARAM identList IN doubleInterval ";"
						{
							for (unsigned i = 0; i < $2.size(); i++)
							{
								if (drv.data.isSymbolDefined($2[i]))
								{
									yy::parser::error(@2, "Symbol '" + $2[i] + "' already defined");
									YYERROR;
								}
								SymbolicAlgebra::Symbol<> s($2[i]);
								drv.data.addParameter(new AbsSyn::Parameter(s));
								drv.ctx.addParameter(s);
								
								AbsSyn::Direction *d = new AbsSyn::Direction(s, 0, AbsSyn::Direction::Type::INT, $4.first, $4.second, "default_" + $2[i]);
								drv.data.addParamDirectionConstraint(d, drv.ctx);
							}
						}
						| PARAM identList ";"
						{
							for (unsigned i = 0; i < $2.size(); i++)
							{
								if (drv.data.isSymbolDefined($2[i]))
								{
									yy::parser::error(@2, "Symbol '" + $2[i] + "' already defined");
									YYERROR;
								}
								SymbolicAlgebra::Symbol<> s($2[i]);
								drv.data.addParameter(new AbsSyn::Parameter(s));
								drv.ctx.addParameter(s);
							}
						}
						| CONST IDENT "=" expr ";"
						{
							if (!isNumeric($4, drv.ctx))
							{
								yy::parser::error(@3, "Expression defining constant must be numeric");
								YYERROR;
							}
							if (drv.data.isSymbolDefined($2))
							{
								yy::parser::error(@2, "Symbol '" + $2 + "' already defined");
								YYERROR;
							}
							SymbolicAlgebra::Symbol<> s($2);
							drv.data.addConstant(new AbsSyn::Constant(s, evaluate($4, drv.ctx)));
							drv.ctx.addConstant(s, evaluate($4, drv.ctx));
						}
						| DEFINE IDENT "=" expr ";"
						{
							if (drv.data.isSymbolDefined($2))
							{
								yy::parser::error(@2, "Symbol '" + $2 + "' already defined");
								YYERROR;
							}
							
							SymbolicAlgebra::Symbol<> s($2);
							drv.data.addDefinition(new AbsSyn::Definition(s, $4));
							drv.ctx.addDefinition(s, $4);
						}
						| DYN "(" IDENT ")" "=" expr ";"
						{
							if (!drv.data.isSymbolDefined($3))
							{
								yy::parser::error(@3, "Unknown symbol name '" + $3 + "'");
								YYERROR;
							}
							
							if (!drv.data.isVarDefined($3))
							{
								yy::parser::error(@3, "'" + $3 + "' is not a variable");
								YYERROR;
							}
							
							AbsSyn::Variable *v = drv.data.getVar($3);
							if (v->isDynamicDefined())
							{
								yy::parser::error(@$, "Redefinition of dynamic for variable '" + $3 + "'");
								YYERROR;
							}
							v->setDynamic($6);
						}
						| SPEC ":" formula ";"
						{
							if (!$3->simplify())
							{
								yy::parser::error(@3, "Negations of UNTIL are not allowed");
								YYERROR;
							}
							
							drv.data.addSpec($3);
						}
						| ASSUME direction ";"
						{
							if (drv.data.getProblem() == AbsSyn::problemType::SYNTH) {
								yy::parser::error(@1, "Assumptions are not supported for synthesis yet");
								YYERROR;
							}
							
							if (hasParams($2->getLHS(), drv.ctx) || hasParams($2->getRHS(), drv.ctx)) {
								yy::parser::error(@2, "Expressions in assumptions cannot contain parameters");
								YYERROR;
							}
							if (getVarDegree($2->getLHS(), drv.ctx) > 1 || getVarDegree($2->getRHS(), drv.ctx) > 1) {
								yy::parser::error(@2, "Assumption must be linear");
								YYERROR;
							}
							if ($2->getType() == AbsSyn::Direction::Type::EQ) {
								yy::parser::error(@2, "Directions with \"=\" are not supported yet in assumptions");
								YYERROR;
							}
							if ($2->getType() == AbsSyn::Direction::Type::INT) {
								yy::parser::error(@2, "Directions with intervals are not supported yet in assumptions");
								YYERROR;
							}
							
							drv.data.addAssumption($2);
						}

matrices		: var_direction {}
						| template {}
						| param_direction {}

direction	: expr directionType expr
						{
							if (getVarDegree($1, drv.ctx) > 1) {
								yy::parser::error(@1, "Expression in directions must be at most linear");
								YYERROR;
							}
							if (getVarDegree($3, drv.ctx) > 1) {
								yy::parser::error(@3, "Expression in directions must be at most linear");
								YYERROR;
							}
							if (getParamDegree($1, drv.ctx) > 0) {
								yy::parser::error(@1, "Expression in directions cannot contain parameters");
							}
							if (getParamDegree($3, drv.ctx) > 0) {
								yy::parser::error(@3, "Expression in directions cannot contain parameters");
							}
							$$ = new AbsSyn::Direction($1, $3, $2);
						}
						| expr IN doubleInterval
						{
							if (getVarDegree($1, drv.ctx) > 1) {
								yy::parser::error(@1, "Expression in directions must be at most linear");
								YYERROR;
							}
							if (getParamDegree($1, drv.ctx) > 0) {
								yy::parser::error(@1, "Expression in directions cannot contain parameters");
								YYERROR;
							}
							$$ = new AbsSyn::Direction($1, 0, AbsSyn::Direction::Type::INT, $3.first, $3.second);
						}

directionType		: "<"
							{
								$$ = AbsSyn::Direction::Type::LT;
							}
							| "<="
							{
								$$ = AbsSyn::Direction::Type::LE;
							}
							| ">"
							{
								$$ = AbsSyn::Direction::Type::GT;
							}
							| ">="
							{
								$$ = AbsSyn::Direction::Type::GE;
							}
							| "="
							{
								$$ = AbsSyn::Direction::Type::EQ;
							}

var_direction	: DIR direction ";"
							{
								if ($2->hasParams(drv.ctx)) {
									yy::parser::error(@2, "Variable directions cannot contain parameters");
									YYERROR;
								}
								drv.data.addDirectionConstraint($2, drv.ctx);
							}
							| DIR IDENT ":" direction ";"
							{
								if ($4->hasParams(drv.ctx)) {
									yy::parser::error(@2, "Variable directions cannot contain parameters");
									YYERROR;
								}
								if (drv.data.isSymbolDefined($2))
								{
									yy::parser::error(@2, "Symbol '" + $2 + "' already defined");
									YYERROR;
								}
								$4->setName($2);
								drv.data.addDirectionConstraint($4, drv.ctx);
						}

template		: TEMPL "=" "{" rowList "}"
						{
							if ($4[0].size() != drv.data.getVarNum())
							{
								yy::parser::error(@3, "template matrix must have as many columns as the number of variables");
								YYERROR;
							}
							
							if (drv.data.templateRows() != 0)
							{
								yy::parser::error(@$, "Redefinition of template matrix");
								YYERROR;
							}
							
							drv.data.setTemplate($4);
						}

rowList			: "{" matrixRow "}" { $$ = std::vector<std::vector<int>>{$2}; }
						| rowList "," "{" matrixRow "}" { $1.push_back($4); $$ = $1; }

matrixRow	: matrixRow "," IDENT
					{
						if (!drv.data.isDirectionDefined($3)) {
							yy::parser::error(@3, "Direction " + $3 + " is not defined");
							YYERROR;
						}
						
						$1.push_back(drv.data.findDirectionPos($3));
						$$ = $1;
					}
					| IDENT
					{
						if (!drv.data.isDirectionDefined($1)) {
							yy::parser::error(@1, "Direction " + $1 + " is not defined");
							YYERROR;
						}
						
						$$ = {drv.data.findDirectionPos($1)};
					}
					| matrixRow "," INTEGER
					{
						if ($3 < 0 || (unsigned int)$3 >= drv.data.getDirectionsNum())
						{
							yy::parser::error(@3, "Unknown direction " + std::to_string($3));
							YYERROR;
						}
						
						$1.push_back($3);
						$$ = $1;
					}
					| INTEGER
					{
						if ($1 < 0 || (unsigned int)$1 >= drv.data.getDirectionsNum())
						{
							yy::parser::error(@1, "Unknown direction " + std::to_string($1));
							YYERROR;
						}
						
						$$ = {$1};
					}

param_direction	: PDIR direction ";"
								{
									if ($2->hasVars(drv.ctx)) {
										yy::parser::error(@2, "Parameter directions cannot contain variables");
										YYERROR;
									}
									
									drv.data.addParamDirectionConstraint($2, drv.ctx);
								}
								| PDIR IDENT ":" direction ";"
								{
									if ($4->hasVars(drv.ctx)) {
										yy::parser::error(@2, "Parameter directions cannot contain variables");
										YYERROR;
									}
									if (drv.data.isSymbolDefined($2))
									{
										yy::parser::error(@2, "Symbol '" + $2 + "' already defined");
										YYERROR;
									}
									$4->setName($2);
									drv.data.addParamDirectionConstraint($4, drv.ctx);
								}

problemType	: REACH { $$ = AbsSyn::problemType::REACH; }
						| SYNTH { $$ = AbsSyn::problemType::SYNTH; }

modeType	: BOX { $$ = AbsSyn::modeType::BOX; }
					| PARAL { $$ = AbsSyn::modeType::PARAL; }
					| POLY { $$ = AbsSyn::modeType::POLY; }

identList	: IDENT { $$ = std::vector<std::string>{$1}; }
					| identList "," IDENT { $1.push_back($3); $$ = $1; }

intInterval			: "[" expr "," expr "]"
								{
									if (!isNumeric($2, drv.ctx))
									{
										yy::parser::error(@2, "Intervals require numeric expressions");
										YYERROR;
									}
									
									if (!isNumeric($4, drv.ctx))
									{
										yy::parser::error(@4, "Intervals require numeric expressions");
										YYERROR;
									}
									
									int x1 = (int) evaluate($2, drv.ctx);
									int x2 = (int) evaluate($4, drv.ctx);
									
									if (x2 < x1)
									{
										yy::parser::error(@$, "Right endpoint must be greater than or equal to the left one");
										YYERROR;
									}
									
									$$ = std::pair<int, int>(x1, x2);
								}

doubleInterval	: "[" expr "," expr "]"
								{
									if (!isNumeric($2, drv.ctx))
									{
										yy::parser::error(@2, "Intervals require numeric expressions");
										YYERROR;
									}
									
									if (!isNumeric($4, drv.ctx))
									{
										yy::parser::error(@4, "Intervals require numeric expressions");
										YYERROR;
									}

									double x1 = evaluate($2, drv.ctx);
									double x2 = evaluate($4, drv.ctx);
									
									if (x2 < x1)
									{
										yy::parser::error(@$, "Right endpoint must be greater than or equal to the left one");
										YYERROR;
									}
									
									$$ = std::pair<double, double>(x1, x2);
								}

number		: DOUBLE { $$ = $1; }
					| INTEGER { $$ = (double) $1; }

expr		: number	{ $$ = $1; }
				| "+" expr { $$ = $2; }
				| IDENT	
				{
					if (!drv.data.isSymbolDefined($1))
					{
						yy::parser::error(@1, "Symbol " + $1 + " is undefined");
						std::cout << drv.data << std::endl;
						YYERROR;
					}
					
					$$ = drv.ctx.getSymbol($1);
				}
				| expr "*" expr { $$ = $1 * $3; }
				| expr "^" expr
				{
					if (!isNumeric($3, drv.ctx))
					{
						yy::parser::error(@3, "Exponent must be numeric");
						YYERROR;
					}
					
					double val = $3.evaluate<double>();
					if (val != (int) val) {
						yy::parser::error(@3, "Exponent must be integer");
						YYERROR;
					}
					
					int exp = (int) val;
					if (exp < 0) {
						yy::parser::error(@3, "Exponent must be non-negative");
						YYERROR;
					}
					
					if (exp == 0) {
						$$ = 1;
					} else {
						SymbolicAlgebra::Expression<> res = $1;
						for (int i = 1; i < exp; i++) {
							res *= $1;
						}
						$$ = res;
					}
				}
				| expr "/" expr
				{
					if (!isNumeric($3, drv.ctx))
					{
						yy::parser::error(@3, "cannot divide by non--numeric expression");
						YYERROR;
					}
					$$ = $1 / $3;
				}
				| expr "+" expr { $$ = $1 + $3; }
				| expr "-" expr { $$ = $1 - $3; }
				| "-" expr %prec UMINUS { $$ = -$2; }
				| "(" expr ")" { $$ = $2;}

formula	: expr ">" expr { $$ = std::make_shared<Atom>($3 - $1); }
				| expr ">=" expr { $$ = std::make_shared<Atom>($3 - $1); }
				| expr "<" expr { $$ = std::make_shared<Atom>($1 - $3); }
				| expr "<=" expr { $$ = std::make_shared<Atom>($1 - $3); }
				| expr "=" expr
				{
					std::shared_ptr<STL> f1 = std::make_shared<Atom>($1 - $3);
					std::shared_ptr<STL> f2 = std::make_shared<Atom>($3 - $1);
					$$ = std::make_shared<Conjunction>(f1, f2);
				}
				| formula AND formula		{ $$ = std::make_shared<Conjunction>($1, $3); }
				| formula OR formula		{ $$ = std::make_shared<Disjunction>($1, $3); }
				| NOT formula									{ $$ = std::make_shared<Negation>($2); }
				| "(" formula ")" { $$ = $2; }
				| "G" intInterval formula %prec "G"	{ $$ = std::make_shared<Always>($2.first, $2.second, $3); }
				| "F" intInterval formula %prec "F"	{ $$ = std::make_shared<Eventually>($2.first, $2.second, $3); }
				| formula "U" intInterval formula %prec "U"	{ $$ = std::make_shared<Until>($1, $3.first, $3.second, $4); }

option	: OPT TRANS transType ";"
				{
					if (drv.data.isTransModeDefined())
					{
						yy::parser::error(@$, "Transformation type already defined");
						YYERROR;
					}
					drv.data.setTransMode($3);
				}
				| OPT DECOMP
				{
					if (drv.data.isDecompositionDefined())
					{
						yy::parser::error(@$, "Decomposition option already defined");
						YYERROR;
					}
					
					drv.data.setDecomposition();
				}
				| OPT ALPHA DOUBLE ";"
				{
					if (drv.data.isAlphaDefined())
					{
						yy::parser::error(@$, "Alpha already defined");
						YYERROR;
					}
					
					if ($3 > 1)
					{
						yy::parser::error(@3, "Alpha must be between 0 and 1");
						YYERROR;
					}
					
					drv.data.setAlpha($3);
				}

transType : AFO { $$ = AbsSyn::transType::AFO; }
					| OFO { $$ = AbsSyn::transType::OFO; }

%%

void
yy::parser::error (const location_type& l, const std::string& m)
{
	std::cerr << "Error at line " << l.end.line << ", column " << l.end.column << ": " << m << '\n';
}
