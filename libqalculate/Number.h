/*
    Qalculate    

    Copyright (C) 2004  Niklas Knutsson (nq@altern.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef NUMBER_H
#define NUMBER_H

#include <libqalculate/includes.h>

#include <cln/cln.h>

#define EQUALS_PRECISION_DEFAULT 	-1
#define EQUALS_PRECISION_LOWEST		-2
#define EQUALS_PRECISION_HIGHEST	-3

/** 
* A number.
*
* Can be rational, floating, point, complex or infinite. Implimented using CLN numbers.
 */
class Number {
	
	private:

	protected:
	
		void removeFloatZeroPart();
		void testApproximate();
		void testInteger();
		void setPrecisionAndApproximateFrom(const Number &o);

		cln::cl_N value;
		bool b_inf, b_pinf, b_minf;
		bool b_approx;
		int i_precision;

	public:
	
		/**
		* Constructs a number initialized as zero.
 		*/
		Number();
		/**
		* Constructs a number parsing a text string.
		*
		* @param number Text string to read number from.
		* @param po Options for parsing the text string.
 		*/
		Number(string number, const ParseOptions &po = default_parse_options);
		/**
		* Constructs a rational number.
		*
		* @param numerator
		* @param denominator
		* @param exp_10
 		*/
		Number(int numerator, int denominator = 1, int exp_10 = 0);
		/**
		* Constructs a copy of a number.
 		*/
		Number(const Number &o);
		virtual ~Number();
		
		void set(string number, const ParseOptions &po = default_parse_options);
		void set(int numerator, int denominator = 1, int exp_10 = 0);
		void setInfinity();
		void setPlusInfinity();
		void setMinusInfinity();
		void setFloat(double d_value);

		void setInternal(const cln::cl_N &cln_value);

		void setImaginaryPart(const Number &o);
		void setImaginaryPart(int numerator, int denominator = 1, int exp_10 = 0);
		void set(const Number &o);
		void clear();

		const cln::cl_N &internalNumber() const;
		
		double floatValue() const;
		/**
		* Converts a number to an integer. If the number does not represent an integer it will rounded using round().
		*
		* @param overflow If overflow is non-null it will be set to true if the number was to large to fit in an int.
		* @return Resulting integer.
 		*/
		int intValue(bool *overflow = NULL) const;
		
		/** Returns true if the number is approximate.
		*
 		* @return true if the number is approximate.
 		*/
		bool isApproximate() const;
		/** Returns true if the number has an approximate representation/is of approximate type -- if it is a floating point number. Numbers of approximate type are always approximate, but the reversed relation is not always true.
		*
 		* @return true if the number has an approximate representation.
 		*/
		bool isApproximateType() const;
		/** Defines the number as approximate or exact. If a number of approximate type is set as exact, it will be converted to a rational number.
		*
 		* @param is_approximate If the number shall be regarded as approximate.
 		*/
		void setApproximate(bool is_approximate = true);
		
		/** Returns the.precision of the number.
		*
 		* @return Precision of the number or -1 if the number is exact or the precision has not been set.
 		*/
		int precision() const;
		void setPrecision(int prec);
		
		bool isUndefined() const;
		/** Returns true if the number is infinity, plus infinity or minus infinity.
		*
 		* @return true if the number is infinite.
 		*/
		bool isInfinite() const;
		/** Returns true if the number is infinity, if the number is plus or minus infinity (which is not known).
		*
 		* @return true if the number is infinity.
 		*/
		bool isInfinity() const;
		/** Returns true if the number is plus infinity.
		*
 		* @return true if the number is plus infinity.
 		*/
		bool isPlusInfinity() const;
		/** Returns true if the number is minus infinity.
		*
 		* @return true if the number is minus infinity.
 		*/
		bool isMinusInfinity() const;
		
		/** Returns the real part of the number if it is complex, or a copy if it is real.
		*
 		* @return true if the real part of a complex number.
 		*/
		Number realPart() const;
		/** Returns the imaginary part as real number of the number if it is complex, or zero if it is real.
		*
 		* @return true if the imaginary part of a complex number.
 		*/
		Number imaginaryPart() const;
		Number numerator() const;
		Number denominator() const;
		Number complexNumerator() const;
		Number complexDenominator() const;

		void operator = (const Number &o);
		void operator -- (int);
		void operator ++ (int);
		Number operator - () const;
		Number operator * (const Number &o) const;
		Number operator / (const Number &o) const;
		Number operator + (const Number &o) const;
		Number operator - (const Number &o) const;
		Number operator ^ (const Number &o) const;
		Number operator && (const Number &o) const;
		Number operator || (const Number &o) const;
		Number operator ! () const;
		
		void operator *= (const Number &o);
		void operator /= (const Number &o);
		void operator += (const Number &o);
		void operator -= (const Number &o);
		void operator ^= (const Number &o);
		
		bool operator == (const Number &o) const;
		bool operator != (const Number &o) const;
		
		bool bitAnd(const Number &o);
		bool bitOr(const Number &o);
		bool bitXor(const Number &o);
		bool bitNot();
		bool bitEqv(const Number &o);
		bool shiftLeft(const Number &o);
		bool shiftRight(const Number &o);
		bool shift(const Number &o);
		
		bool hasRealPart() const;
		bool hasImaginaryPart() const;
		bool isComplex() const;
		bool isInteger() const;
		Number integer() const;
		bool isRational() const;
		bool isReal() const;
		bool isFraction() const;
		bool isZero() const;
		bool isOne() const;
		bool isTwo() const;
		bool isI() const;
		bool isMinusI() const;
		bool isMinusOne() const;
		bool isNegative() const;
		bool isNonNegative() const;
		bool isPositive() const;
		bool isNonPositive() const;
		bool realPartIsNegative() const;
		bool realPartIsPositive() const;
		bool imaginaryPartIsNegative() const;
		bool imaginaryPartIsPositive() const;
		bool hasNegativeSign() const;
		bool hasPositiveSign() const;
		bool equalsZero() const;
		bool equals(const Number &o) const;
		bool equalsApproximately(const Number &o, int prec) const;
		ComparisonResult compare(const Number &o) const;
		ComparisonResult compareApproximately(const Number &o, int prec = EQUALS_PRECISION_LOWEST) const;
		ComparisonResult compareImaginaryParts(const Number &o) const;
		ComparisonResult compareRealParts(const Number &o) const;
		bool isGreaterThan(const Number &o) const;
		bool isLessThan(const Number &o) const;
		bool isGreaterThanOrEqualTo(const Number &o) const;
		bool isLessThanOrEqualTo(const Number &o) const;
		bool isEven() const;
		bool denominatorIsEven() const;
		bool denominatorIsTwo() const;
		bool numeratorIsEven() const;
		bool numeratorIsOne() const;
		bool numeratorIsMinusOne() const;
		bool isOdd() const;
		
		int integerLength() const;

		/** Add to the number (x+o).
		*
		* @param o Number to add.
 		* @return true if the operation was successful.
 		*/
		bool add(const Number &o);
		/** Subtracts from to the number (x-o).
		*
		* @param o Number to subtract.
 		* @return true if the operation was successful.
 		*/
		bool subtract(const Number &o);
		/** Multiply the number (x*o).
		*
		* @param o Number to multiply with.
 		* @return true if the operation was successful.
 		*/
		bool multiply(const Number &o);
		/** Divide the number (x/o).
		*
		* @param o Number to divide by.
 		* @return true if the operation was successful.
 		*/
		bool divide(const Number &o);
		/** Invert the number (1/x).
		*
 		* @return true if the operation was successful.
 		*/
		bool recip();
		/** Raise the number (x^o).
		*
		* @param o Number to raise to.
		* @param try_exact If an exact solution should be tried first (might be slow).
 		* @return true if the operation was successful.
 		*/
		bool raise(const Number &o, bool try_exact = true);
		/** Multiply the number with a power of ten (x*10^o).
		*
		* @param o Number to raise 10 by.
 		* @return true if the operation was successful.
 		*/
		bool exp10(const Number &o);
		/** Multiply the number with a power of two (x*2^o).
		*
		* @param o Number to raise 2 by.
 		* @return true if the operation was successful.
 		*/
		bool exp2(const Number &o);
		/** Set the number to ten raised by the number (10^x).
		*
 		* @return true if the operation was successful.
 		*/
		bool exp10();
		/** Set the number to two raised by the number (2^x).
		*
 		* @return true if the operation was successful.
 		*/
		bool exp2();
		/** Raise the number by two (x^2).
		*
 		* @return true if the operation was successful.
 		*/
		bool square();
		
		/** Negate the number (-x).
		*
 		* @return true if the operation was successful.
 		*/
		bool negate();
		void setNegative(bool is_negative);
		bool abs();
		bool signum();
		bool round(const Number &o);
		bool floor(const Number &o);
		bool ceil(const Number &o);
		bool trunc(const Number &o);
		bool mod(const Number &o);
		bool isqrt();
		bool round();
		bool floor();
		bool ceil();
		bool trunc();	
		bool frac();		
		bool rem(const Number &o);
		
		bool smod(const Number &o);
		bool irem(const Number &o);
		bool irem(const Number &o, Number &q);
		bool iquo(const Number &o);
		bool iquo(const Number &o, Number &r);

		int getBoolean() const;
		void toBoolean();
		void setTrue(bool is_true = true);
		void setFalse();
		void setLogicalNot();
		
		/** Set the number to e, the base of natural logarithm, calculated with the current default precision.
 		*/
		void e();
		/** Set the number to pi, Archimede's constant, calculated with the current default precision.
 		*/
		void pi();
		/** Set the number to Catalan's constant, calculated with the current default precision.
 		*/
		void catalan();
		/** Set the number to Euler's constant, calculated with the current default precision.
 		*/
		void euler();
		/** Set the number to Riemann's zeta with the number as integral point. The number must be an integer greater than one.
		*
 		* @return true if the calculation was successful.
 		*/
		bool zeta();			
		
		bool sin();
		bool asin();
		bool sinh();
		bool asinh();
		bool cos();
		bool acos();
		bool cosh();
		bool acosh();
		bool tan();
		bool atan();
		bool tanh();
		bool atanh();
		bool ln();	
		bool log(const Number &o);
		bool exp();
		bool gcd(const Number &o);
		bool lcm(const Number &o);
		
		bool factorial();
		bool multiFactorial(const Number &o);
		bool doubleFactorial();
		bool binomial(const Number &m, const Number &k);
	
		bool add(const Number &o, MathOperation op); 

		string printNumerator(int base = 10, bool display_sign = true, bool display_base_indicator = true, bool lower_case = false) const;
		string printDenominator(int base = 10, bool display_sign = true, bool display_base_indicator = true, bool lower_case = false) const;
		string printImaginaryNumerator(int base = 10, bool display_sign = true, bool display_base_indicator = true, bool lower_case = false) const;
		string printImaginaryDenominator(int base = 10, bool display_sign = true, bool display_base_indicator = true, bool lower_case = false) const;

		string print(const PrintOptions &po = default_print_options, const InternalPrintStruct &ips = top_ips) const;
	
};

#endif
