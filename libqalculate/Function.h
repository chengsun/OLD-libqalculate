/*
    Qalculate    

    Copyright (C) 2003  Niklas Knutsson (nq@altern.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef FUNCTION_H
#define FUNCTION_H

#include <libqalculate/ExpressionItem.h>
#include <libqalculate/includes.h>

/** @file */

///Argument types
typedef enum {
	ARGUMENT_TYPE_FREE,
	ARGUMENT_TYPE_SYMBOLIC,
	ARGUMENT_TYPE_TEXT,
	ARGUMENT_TYPE_DATE,
	ARGUMENT_TYPE_FILE,
	ARGUMENT_TYPE_INTEGER,	
	ARGUMENT_TYPE_NUMBER,
	ARGUMENT_TYPE_VECTOR,	
	ARGUMENT_TYPE_MATRIX,
	ARGUMENT_TYPE_EXPRESSION_ITEM,
	ARGUMENT_TYPE_FUNCTION,	
	ARGUMENT_TYPE_UNIT,
	ARGUMENT_TYPE_BOOLEAN,
	ARGUMENT_TYPE_VARIABLE,
	ARGUMENT_TYPE_ANGLE,
	ARGUMENT_TYPE_SET,
	ARGUMENT_TYPE_DATA_OBJECT,
	ARGUMENT_TYPE_DATA_PROPERTY
} ArgumentType;

///Predefined max and min values for number and integer arguments.
typedef enum {
	ARGUMENT_MIN_MAX_NONE,
	ARGUMENT_MIN_MAX_POSITIVE,
	ARGUMENT_MIN_MAX_NONZERO,
	ARGUMENT_MIN_MAX_NONNEGATIVE,
	ARGUMENT_MIN_MAX_NEGATIVE	
} ArgumentMinMaxPreDefinition;

enum {
	SUBTYPE_FUNCTION,
	SUBTYPE_USER_FUNCTION,
	SUBTYPE_DATA_SET
};

/// Abstract base class for mathematical functions.
/**
*/
class MathFunction : public ExpressionItem {

  protected:

	int argc;
	int max_argc;
	vector<string> default_values;
	Sgi::hash_map<size_t, Argument*> argdefs;
	size_t last_argdef_index;		
	bool testArguments(MathStructure &vargs);
	virtual MathStructure createFunctionMathStructureFromVArgs(const MathStructure &vargs);
	virtual MathStructure createFunctionMathStructureFromSVArgs(vector<string> &svargs);	
	string scondition;
	
  public:
  
	MathFunction(string name_, int argc_, int max_argc_ = 0, string cat_ = "", string title_ = "", string descr_ = "", bool is_active = true);
	MathFunction(const MathFunction *function);
	MathFunction();
	virtual ~MathFunction();	

	virtual ExpressionItem *copy() const = 0;
	virtual void set(const ExpressionItem *item);
	virtual int type() const;
	virtual int subtype() const;

	bool testArgumentCount(int itmp);
	virtual MathStructure calculate(const string &eq, const EvaluationOptions &eo = default_evaluation_options);
	virtual MathStructure parse(const string &eq, const ParseOptions &po = default_parse_options);
	virtual int parse(MathStructure &mstruct, const string &eq, const ParseOptions &po = default_parse_options);
	virtual MathStructure calculate(MathStructure &vargs, const EvaluationOptions &eo = default_evaluation_options);	
	virtual int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);
	/** Returns the functions condition expression.
	*
	* @returns The function's condition expression 
	*/
	string condition() const;
	/** Print the function's condition expression with argument names.
	*
	* @returns The printed condition 
	*/
	string printCondition();
	/** Sets the functions condition expression.
	*
	* @param expression The function's new condition expression 
	*/
	void setCondition(string expression);
	/** Test if arguments fulfil the function's condition expression.
	*
	* @param vargs Vector with arguments.
	* @returns true if the arguments fulfil the function's condition expression 
	*/
	bool testCondition(const MathStructure &vargs);
	/** Returns the maximum number of arguments that the function accepts or -1 if the number of arguments is unlimited.
	*/
	int args() const;
	/** Returns the minimum number of arguments for the function.
	*/
	int minargs() const;
	/** Returns the maximum number of arguments that the function accepts or -1 if the number of arguments is unlimited.
	*/
	int maxargs() const;
	/** Parses arguments from a text string and places them in a vector. The text string should be a comma separated list of arguments.
	*	
	* @param str The argument string to parse.
	* @param vargs Vector to store parsed arguments in.
	* @param po Parse options.
	* @returns The number of parsed arguments.
	*/
	int args(const string &str, MathStructure &vargs, const ParseOptions &po = default_parse_options);
	/** Returns the index of the last argument definition.
	*
	* @returns The index of the last argument definition 
	*/
	size_t lastArgumentDefinitionIndex() const;
	/** Returns the argument definition for an argument index.
	*
	* @param index Argument index.
	* @returns The argument definition for the index or NULL if no the argument was not defined for the index 
	*/
	Argument *getArgumentDefinition(size_t index);
	/** Removes all argument definitions for the function.
	*/
	void clearArgumentDefinitions();
	/** Set the argument definition for an argument index.
	*
	* @param index Argument index.
	* @param argdef A newly allocated argument definition 
	*/
	void setArgumentDefinition(size_t index, Argument *argdef);
	int stringArgs(const string &str, vector<string> &svargs);
	void setDefaultValue(size_t arg_, string value_);
	const string &getDefaultValue(size_t arg_) const;	
	MathStructure produceVector(const MathStructure &vargs, int begin = -1, int end = -1);
	MathStructure produceArgumentsVector(const MathStructure &vargs, int begin = -1, int end = -1);
	
	virtual bool representsPositive(const MathStructure&, bool = false) const;
	virtual bool representsNegative(const MathStructure&, bool = false) const;
	virtual bool representsNonNegative(const MathStructure&, bool = false) const;
	virtual bool representsNonPositive(const MathStructure&, bool = false) const;
	virtual bool representsInteger(const MathStructure&, bool = false) const;
	virtual bool representsNumber(const MathStructure&, bool = false) const;
	virtual bool representsRational(const MathStructure&, bool = false) const;
	virtual bool representsReal(const MathStructure&, bool = false) const;
	virtual bool representsComplex(const MathStructure&, bool = false) const;
	virtual bool representsNonZero(const MathStructure&, bool = false) const;
	virtual bool representsEven(const MathStructure&, bool = false) const;
	virtual bool representsOdd(const MathStructure&, bool = false) const;
	virtual bool representsUndefined(const MathStructure&) const;
	virtual bool representsBoolean(const MathStructure&) const;
	virtual bool representsNonMatrix(const MathStructure&) const;
	
};

/// A user defined mathematical function.
/** User functions are functions defined using an expression.
*/
class UserFunction : public MathFunction {
  protected:
  
	string eq, eq_calc;
	vector<string> v_subs;
	vector<bool> v_precalculate;
	
  public:
  
	UserFunction(string cat_, string name_, string eq_, bool is_local = true, int argc_ = -1, string title_ = "", string descr_ = "", int max_argc_ = 0, bool is_active = true);
	UserFunction(const UserFunction *function);
	void set(const ExpressionItem *item);
	ExpressionItem *copy() const;
	string equation() const;
	string internalEquation() const;
	int calculate(MathStructure &mstruct, const MathStructure &vargs, const EvaluationOptions &eo);	
	void setEquation(string new_eq, int argc_ = -1, int max_argc_ = 0);
	void addSubfunction(string subfunction, bool precalculate = true);
	void setSubfunction(size_t index, string subfunction);
	void delSubfunction(size_t index);
	void clearSubfunctions();
	size_t countSubfunctions() const;
	void setSubfunctionPrecalculated(size_t index, bool precalculate);
	const string &getSubfunction(size_t index) const;
	bool subfunctionPrecalculated(size_t index) const;
	int subtype() const;
};

/// A mathematical function argument definition with free value and base class for all argument definitions.
/** Free arguments accepts any value.
*/
class Argument {

  protected:
  
  	string sname, scondition;
	bool b_zero, b_test, b_matrix, b_text, b_error, b_rational;
	/** This function is called from Argument::test() and performs validation specific to the argument definition type.
	* Should be reimplemented by all subclasses.
	*
	* @param value Value to test.
	* @param eo Evaluation options to use if the value needs to be evaluated.
	* @returns true if the value is valid for the argument definition.
	*/
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;
	/** This function is called from Argument::printlong() and returns description specific the argument definition type.
	* Should be reimplemented by all subclasses. For example IntegerArgument::subprintlong() might return "an integer"
	* and Argument::printlong() might append " that fulfills the condition: even(\x)".
	*
	* @returns Long description.
	*/
	virtual string subprintlong() const;
	
  public:

	/** Creates a new argument definition.
	*
	* @param name Name/title of the argument definition.
	* @param does_test If argument values will be tested.
	* @param does_error If an error will issued if the value tests false.
	*/
	Argument(string name_ = "", bool does_test = true, bool does_error = true);
	/** Creates a copy of an argument definition.
	*
	* @param arg Argument to copy.
	*/
	Argument(const Argument *arg);
	/** Destructor */
	virtual ~Argument();

	/** Sets the argument to a copy of an argument definition.
	*
	* @param arg Argument to copy.
	*/
	virtual void set(const Argument *arg);
	/** Returns a copy of the argument definition.
	*
	* @returns A copy.
	*/
	virtual Argument *copy() const;

	/** Resturns a short description of the argument definition.
	* Ex. "number" for NumberArgument.
	*
	* @returns Short description.
	*/
	virtual string print() const;
	/** Resturns a long description of the argument definition.
	* Ex. "A real number > 2".
	*
	* @returns Long description.
	*/
	string printlong() const;

	/** Tests if a value fulfils the requirements of the argument definition.
	* The value might change if it has not been fully evaluated.
	*
	* @param value Value to test.
	* @param f Mathematical function that the value is an argument for.
	* @param eo Evaluation options to use if the value needs to be evaluated.
	* @returns true if the value is valid for the argument definition.
	*/
	bool test(MathStructure &value, int index, MathFunction *f, const EvaluationOptions &eo = default_evaluation_options) const;
	/** Parses an expression for an argument value.
	* The default behavior is to use Calculator::parse() directly.
	*
	* @param str Expression.
	* @param po Parse options.
	* @returns A new mathematical structure with the parsed expression.
	*/
	virtual MathStructure parse(const string &str, const ParseOptions &po = default_parse_options) const;
	/** Parses an expression for an argument value.
	* The default behavior is to use Calculator::parse() directly.
	*
	* @param mstruct Mathematical structure to set with the parsed expression.
	* @param str Expression.
	* @param po Parse options.
	*/
	virtual void parse(MathStructure *mstruct, const string &str, const ParseOptions &po = default_parse_options) const;

	/** Returns the name/title of the argument definition.
	*
	* @returns Name/title.
	*/
	string name() const;
	/** Sets the name/title of the argument definition.
	*
	* @param name_ New name/title.
	*/
	void setName(string name_);

	/** Sets a custom condition for argument values.
	* '\x' is replaced by the argument value in the expression.
	*
	* @param condition Condition expression.
	*/
	void setCustomCondition(string condition);
	/** Returns the custom condition expression set for argument values.
	*
	* @returns Custom condition for argument values.
	*/
	string getCustomCondition() const;

	/** If the value for the argument will be tested. If not, the argument only works as an suggestion and any value is allowed.
	*
	* @returns true if the argument value will be tested.
	*/
	bool tests() const;
	void setTests(bool does_error);

	/** If an error message will be presented to the user if the value for the argument is not allowed.
	*
	* @returns true if error messages will be shown.
	*/
	bool alerts() const;
	void setAlerts(bool does_error);

	/** If an argument value of zero is forbidden.
	*
	* @returns true if zero argument value is forbidden.
	*/
	bool zeroForbidden() const;
	/** Sets if a value of zero is forbidden for the argument value.
	*
	* @param forbid_zero If zero shall be forbidden.
	*/	
	void setZeroForbidden(bool forbid_zero);
	
	bool matrixAllowed() const;
	void setMatrixAllowed(bool allow_matrix);

	/** If only rational polynomials are allowed as argument value.
	*
	* @see MathStructure::isRationalPolynomial()
	* @returns true if only rational polynomials is allowed.
	*/
	bool rationalPolynomial() const;
	void setRationalPolynomial(bool rational_polynomial);
	
	virtual bool suggestsQuotes() const;
	
	virtual int type() const;

};

/// A definition for numerical arguments.
/** These arguments allows numerical values. The value can be restricted to real or rational numbers (defaults to allow all numbers, including complex), and a max and/or min value.
*/
class NumberArgument : public Argument {

  protected:
  
	Number *fmin, *fmax;
	bool b_incl_min, b_incl_max;
	bool b_complex, b_rational_number;

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	NumberArgument(string name_ = "", ArgumentMinMaxPreDefinition minmax = ARGUMENT_MIN_MAX_NONE, bool does_test = true, bool does_error = true);
	NumberArgument(const NumberArgument *arg);
	virtual ~NumberArgument();
	
	virtual void set(const Argument *arg);
	virtual Argument *copy() const;

	virtual string print() const;	
	
	void setMin(const Number *nmin);	
	void setIncludeEqualsMin(bool include_equals);
	bool includeEqualsMin() const;	
	const Number *min() const;
	void setMax(const Number *nmax);	
	void setIncludeEqualsMax(bool include_equals);
	bool includeEqualsMax() const;	
	const Number *max() const;	
	
	bool complexAllowed() const;
	void setComplexAllowed(bool allow_complex);
	bool rationalNumber() const;
	void setRationalNumber(bool rational_number);

	virtual int type() const;

};

/// A definition for integer arguments.
/** These arguments allows numerical integer values. The value can be restricted to a max and/or min value.
*/
class IntegerArgument : public Argument {

  protected:
  
	Number *imin, *imax;

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;
	virtual string subprintlong() const;

  public:
  
  	IntegerArgument(string name_ = "", ArgumentMinMaxPreDefinition minmax = ARGUMENT_MIN_MAX_NONE, bool does_test = true, bool does_error = true);
	IntegerArgument(const IntegerArgument *arg);
	virtual ~IntegerArgument();

	virtual void set(const Argument *arg);
	virtual Argument *copy() const;

	virtual string print() const;	

	void setMin(const Number *nmin);	
	const Number *min() const;
	void setMax(const Number *nmax);	
	const Number *max() const;
	
	virtual int type() const;

};

/// A symbolic argument.
/** Accepts variables and symbolic structures.
*/
class SymbolicArgument : public Argument {

  protected:
  
  	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:

  	SymbolicArgument(string name_ = "", bool does_test = true, bool does_error = true);
	SymbolicArgument(const SymbolicArgument *arg);
	virtual ~SymbolicArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};

/// A text argument.
/** Accepts text (symbolic) structures. Argument values are parsed as text, unless surrounded by back slashes (which are then removed). Surrounding Parentheses and first quotation marks are removed.
*/
class TextArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	TextArgument(string name_ = "", bool does_test = true, bool does_error = true);
	TextArgument(const TextArgument *arg);
	virtual ~TextArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
	virtual bool suggestsQuotes() const;
};

/// A date argument.
/** A text argument representing a date.
*/
class DateArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	DateArgument(string name_ = "", bool does_test = true, bool does_error = true);
	DateArgument(const DateArgument *arg);
	virtual ~DateArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};

/// A vector argument.
/**
*/
class VectorArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;
	vector<Argument*> subargs;
	bool b_argloop;

  public:
  
  	VectorArgument(string name_ = "", bool does_test = true, bool allow_matrix = false, bool does_error = true);
	VectorArgument(const VectorArgument *arg);
	virtual ~VectorArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
	bool reoccuringArguments() const;
	void setReoccuringArguments(bool reocc);
	void addArgument(Argument *arg);
	void delArgument(size_t index);
	size_t countArguments() const;
	Argument *getArgument(size_t index) const;
};

/// A matrix argument.
/**
*/
class MatrixArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;
	bool b_square;

  public:
  
  	MatrixArgument(string name_ = "", bool does_test = true, bool does_error = true);
	MatrixArgument(const MatrixArgument *arg);
	virtual bool squareDemanded() const;
	virtual void setSquareDemanded(bool square);
	virtual ~MatrixArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};

/// Argument for functions, variables and units.
/** Text string representing a function, variable or unit name.
*/
class ExpressionItemArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	ExpressionItemArgument(string name_ = "", bool does_test = true, bool does_error = true);
	ExpressionItemArgument(const ExpressionItemArgument *arg);
	virtual ~ExpressionItemArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};
/// A function argument.
/**
*/
class FunctionArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	FunctionArgument(string name_ = "", bool does_test = true, bool does_error = true);
	FunctionArgument(const FunctionArgument *arg);
	virtual ~FunctionArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};

/// A boolean argument.
/** Accepts zero or one.
*/
class BooleanArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	BooleanArgument(string name_ = "", bool does_test = true, bool does_error = true);
	BooleanArgument(const BooleanArgument *arg);
	virtual ~BooleanArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};

class UnitArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	UnitArgument(string name_ = "", bool does_test = true, bool does_error = true);
	UnitArgument(const UnitArgument *arg);
	virtual ~UnitArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};
class AngleArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	AngleArgument(string name_ = "", bool does_test = true, bool does_error = true);
	AngleArgument(const AngleArgument *arg);
	virtual ~AngleArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
	virtual void parse(MathStructure *mstruct, const string &str, const ParseOptions &po = default_parse_options) const;
};
class VariableArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	VariableArgument(string name_ = "", bool does_test = true, bool does_error = true);
	VariableArgument(const VariableArgument *arg);
	virtual ~VariableArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};
class FileArgument : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;

  public:
  
  	FileArgument(string name_ = "", bool does_test = true, bool does_error = true);
	FileArgument(const FileArgument *arg);
	virtual ~FileArgument();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
};

/// A set of accepted arguments.
/** This is used when several different type of argments shall be accepted by a function.
*/
class ArgumentSet : public Argument {

  protected:
  
	virtual bool subtest(MathStructure &value, const EvaluationOptions &eo) const;  
	virtual string subprintlong() const;
	vector<Argument*> subargs;

  public:
  
  	ArgumentSet(string name_ = "", bool does_test = true, bool does_error = true);
	ArgumentSet(const ArgumentSet *arg);
	virtual ~ArgumentSet();
	virtual int type() const;
	virtual Argument *copy() const;
	virtual string print() const;
	void addArgument(Argument *arg);
	void delArgument(size_t index);
	size_t countArguments() const;
	Argument *getArgument(size_t index) const;
	
};

#endif
