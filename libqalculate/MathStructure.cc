/*
    Qalculate    

    Copyright (C) 2004-2007  Niklas Knutsson (nq@altern.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "support.h"

#include "MathStructure.h"
#include "Calculator.h"
#include "Number.h"
#include "Function.h"
#include "Variable.h"
#include "Unit.h"
#include "Prefix.h"
#include <map>

#define SWAP_CHILDREN(i1, i2)		MathStructure *swap_mstruct = v_subs[v_order[i1]]; v_subs[v_order[i1]] = v_subs[v_order[i2]]; v_subs[v_order[i2]] = swap_mstruct;
#define CHILD_TO_FRONT(i)		v_order.insert(v_order.begin(), v_order[i]); v_order.erase(v_order.begin() + (i + 1));
#define SET_CHILD_MAP(i)		setToChild(i + 1, true);
#define SET_MAP(o)			set(o, true);
#define SET_MAP_NOCOPY(o)		set_nocopy(o, true);
#define MERGE_APPROX_AND_PREC(o)	if(!b_approx && o.isApproximate()) b_approx = true; if(o.precision() > 0 && (i_precision < 1 || o.precision() < i_precision)) i_precision = o.precision();
#define CHILD_UPDATED(i)		if(!b_approx && CHILD(i).isApproximate()) b_approx = true; if(CHILD(i).precision() > 0 && (i_precision < 1 || CHILD(i).precision() < i_precision)) i_precision = CHILD(i).precision();
#define CHILDREN_UPDATED		for(size_t child_i = 0; child_i < SIZE; child_i++) {if(!b_approx && CHILD(child_i).isApproximate()) b_approx = true; if(CHILD(child_i).precision() > 0 && (i_precision < 1 || CHILD(child_i).precision() < i_precision)) i_precision = CHILD(child_i).precision();}

#define APPEND(o)		v_order.push_back(v_subs.size()); v_subs.push_back(new MathStructure(o)); if(!b_approx && o.isApproximate()) b_approx = true; if(o.precision() > 0 && (i_precision < 1 || o.precision() < i_precision)) i_precision = o.precision();
#define APPEND_NEW(o)		{v_order.push_back(v_subs.size()); MathStructure *m_append_new = new MathStructure(o); v_subs.push_back(m_append_new); if(!b_approx && m_append_new->isApproximate()) b_approx = true; if(m_append_new->precision() > 0 && (i_precision < 1 || m_append_new->precision() < i_precision)) i_precision = m_append_new->precision();}
#define APPEND_COPY(o)		v_order.push_back(v_subs.size()); v_subs.push_back(new MathStructure(*(o))); if(!b_approx && (o)->isApproximate()) b_approx = true; if((o)->precision() > 0 && (i_precision < 1 || (o)->precision() < i_precision)) i_precision = (o)->precision();
#define APPEND_POINTER(o)	v_order.push_back(v_subs.size()); v_subs.push_back(o); if(!b_approx && (o)->isApproximate()) b_approx = true; if((o)->precision() > 0 && (i_precision < 1 || (o)->precision() < i_precision)) i_precision = (o)->precision();
#define APPEND_REF(o)		v_order.push_back(v_subs.size()); v_subs.push_back(o); (o)->ref(); if(!b_approx && (o)->isApproximate()) b_approx = true; if((o)->precision() > 0 && (i_precision < 1 || (o)->precision() < i_precision)) i_precision = (o)->precision();
#define PREPEND(o)		v_order.insert(v_order.begin(), v_subs.size()); v_subs.push_back(new MathStructure(o)); if(!b_approx && o.isApproximate()) b_approx = true; if(o.precision() > 0 && (i_precision < 1 || o.precision() < i_precision)) i_precision = o.precision();
#define PREPEND_REF(o)		v_order.insert(v_order.begin(), v_subs.size()); v_subs.push_back(o); (o)->ref(); if(!b_approx && (o)->isApproximate()) b_approx = true; if((o)->precision() > 0 && (i_precision < 1 || (o)->precision() < i_precision)) i_precision = (o)->precision();
#define INSERT_REF(o, i)	v_order.insert(v_order.begin() + i, v_subs.size()); v_subs.push_back(o); (o)->ref(); if(!b_approx && (o)->isApproximate()) b_approx = true; if((o)->precision() > 0 && (i_precision < 1 || (o)->precision() < i_precision)) i_precision = (o)->precision();
#define CLEAR			v_order.clear(); for(size_t i = 0; i < v_subs.size(); i++) {v_subs[i]->unref();} v_subs.clear();
#define REDUCE(v_size)          {vector<size_t> v_tmp; v_tmp.resize(SIZE, 0); for(size_t v_index = v_size; v_index < v_order.size(); v_index++) {v_subs[v_order[v_index]]->unref(); v_subs[v_order[v_index]] = NULL; v_tmp[v_order[v_index]] = 1;} v_order.resize(v_size); for(vector<MathStructure*>::iterator v_it = v_subs.begin(); v_it != v_subs.end();) {if(*v_it == NULL) v_it = v_subs.erase(v_it); else ++v_it;} size_t i_change = 0; for(size_t v_index = 0; v_index < v_tmp.size(); v_index++) {if(v_tmp[v_index] == 1) i_change++; v_tmp[v_index] = i_change;} for(size_t v_index = 0; v_index < v_order.size(); v_index++) v_order[v_index] -= v_tmp[v_index];}
//#define CHILD(v_index)		(*v_subs[v_order[v_index]])
#define SIZE			v_order.size()
#define LAST			(*v_subs[v_order[v_order.size() - 1]])
#define ERASE(v_index)		v_subs[v_order[v_index]]->unref(); v_subs.erase(v_subs.begin() + v_order[v_index]); for(size_t v_index2 = 0; v_index2 < v_order.size(); v_index2++) {if(v_order[v_index2] > v_order[v_index]) v_order[v_index2]--;} v_order.erase(v_order.begin() + (v_index));

#define IS_REAL(o)		(o.isNumber() && o.number().isReal())
#define IS_RATIONAL(o)		(o.isNumber() && o.number().isRational())
#define IS_N_INF_NUMBER(o)	(o.isNumber() && !o.number().isInfinite())

#define IS_A_SYMBOL(o)		(o.isSymbolic() || o.isVariable() || o.isFunction())

#define POWER_CLEAN(o)		if(o[1].isOne()) {o.setToChild(1);} else if(o[1].isZero()) {o.set(1, 1);}

/*void printRecursive(const MathStructure &mstruct) {
	std::cout << "RECURSIVE " << mstruct.print() << std::endl;
	for(size_t i = 0; i < mstruct.size(); i++) {
		std::cout << i << ": " << mstruct[i].print() << std::endl;
		for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
			std::cout << i << "-" << i2 << ": " << mstruct[i][i2].print() << std::endl;
			for(size_t i3 = 0; i3 < mstruct[i][i2].size(); i3++) {
				std::cout << i << "-" << i2 << "-" << i3 << ": " << mstruct[i][i2][i3].print() << std::endl;
				for(size_t i4 = 0; i4 < mstruct[i][i2][i3].size(); i4++) {
					std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << ": " << mstruct[i][i2][i3][i4].print() << std::endl;
					for(size_t i5 = 0; i5 < mstruct[i][i2][i3][i4].size(); i5++) {
						std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << ": " << mstruct[i][i2][i3][i4][i5].print() << std::endl;
						for(size_t i6 = 0; i6 < mstruct[i][i2][i3][i4][i5].size(); i6++) {
							std::cout << i << "-" << i2 << "-" << i3 << "-" << i4 << "-" << i5 << "-" << i6 << ": " << mstruct[i][i2][i3][i4][i5][i6].print() << std::endl;
						}
					}
				}
			}
		}
	}
}*/

bool warn_about_denominators_assumed_nonzero(const MathStructure &mstruct, const EvaluationOptions &eo) {
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	mnonzero.eval(eo2);
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	PrintOptions po;
	po.spell_out_logical_operators = true;
	mnonzero.format(po);
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), mnonzero.print(po).c_str(), NULL);
	return true;
}
bool warn_about_denominators_assumed_nonzero_or_positive(const MathStructure &mstruct, const MathStructure &mstruct2, const EvaluationOptions &eo) {
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	MathStructure *mpos = new MathStructure(mstruct2);
	mpos->add(m_zero, OPERATION_EQUALS_GREATER);
	mnonzero.add_nocopy(mpos, OPERATION_LOGICAL_OR);
	mnonzero.eval(eo2);
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	PrintOptions po;
	po.spell_out_logical_operators = true;
	mnonzero.format(po);
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), mnonzero.print(po).c_str(), NULL);
	return true;
}
bool warn_about_denominators_assumed_nonzero_llgg(const MathStructure &mstruct, const MathStructure &mstruct2, const MathStructure &mstruct3, const EvaluationOptions &eo) {
	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = false;
	eo2.test_comparisons = true;
	eo2.isolate_x = true;
	eo2.expand = true;
	eo2.approximation = APPROXIMATION_APPROXIMATE;
	MathStructure mnonzero(mstruct);
	mnonzero.add(m_zero, OPERATION_NOT_EQUALS);
	MathStructure *mpos = new MathStructure(mstruct2);
	mpos->add(m_zero, OPERATION_EQUALS_GREATER);
	MathStructure *mpos2 = new MathStructure(mstruct3);
	mpos2->add(m_zero, OPERATION_EQUALS_GREATER);
	mpos->add_nocopy(mpos2, OPERATION_LOGICAL_AND);
	mnonzero.add_nocopy(mpos, OPERATION_LOGICAL_OR);
	MathStructure *mneg = new MathStructure(mstruct2);
	mneg->add(m_zero, OPERATION_LESS);
	MathStructure *mneg2 = new MathStructure(mstruct3);
	mneg2->add(m_zero, OPERATION_LESS);
	mneg->add_nocopy(mneg2, OPERATION_LOGICAL_AND);
	mnonzero.add_nocopy(mneg, OPERATION_LOGICAL_OR);
	mnonzero.eval(eo2);
	if(mnonzero.isZero()) return false;
	if(mnonzero.isOne()) return true;
	PrintOptions po;
	po.spell_out_logical_operators = true;
	mnonzero.format(po);
	CALCULATOR->error(false, _("To avoid division by zero, the following must be true: %s."), mnonzero.print(po).c_str(), NULL);
	return true;
}

void unnegate_sign(MathStructure &mstruct) {
	if(mstruct.isNumber()) {
		mstruct.number().negate();
	} else if(mstruct.isMultiplication()) {
		if(mstruct[0].isMinusOne()) {
			if(mstruct.size() > 2) {
				mstruct.delChild(1);
			} else if(mstruct.size() == 2) {
				mstruct.setToChild(2);
			} else {
				mstruct.set(1, 1);
			}
		} else {
			unnegate_sign(mstruct[0]);
		}
	}
}

void MathStructure::mergePrecision(const MathStructure &o) {MERGE_APPROX_AND_PREC(o)}

void idm1(const MathStructure &mnum, bool &bfrac, bool &bint);
void idm2(const MathStructure &mnum, bool &bfrac, bool &bint, Number &nr);
void idm3(MathStructure &mnum, Number &nr, bool expand);

void MathStructure::ref() {
	i_ref++;
}
void MathStructure::unref() {
	i_ref--;
	if(i_ref == 0) {
		delete this;
	}
}
size_t MathStructure::refcount() const {
	return i_ref;
}


inline void MathStructure::init() {
	m_type = STRUCT_NUMBER;
	b_approx = false;
	i_precision = -1;
	i_ref = 1;
	function_value = NULL;
	b_protected = false;
	o_uncertainty = NULL;
}

MathStructure::MathStructure() {
	init();
}
MathStructure::MathStructure(const MathStructure &o) {
	init();
	set(o);
}
MathStructure::MathStructure(int num, int den, int exp10) {
	init();
	set(num, den, exp10);
}
MathStructure::MathStructure(string sym) {
	init();
    if (sym == "undefined") {
        setUndefined(true);
        return;
    }
	set(sym);
}
MathStructure::MathStructure(double float_value) {
	init();
	set(float_value);
}
MathStructure::MathStructure(const MathStructure *o, ...) {
	init();
	clear();
	va_list ap;
	va_start(ap, o); 
	if(o) {
		APPEND_COPY(o)
		while(true) {
			o = va_arg(ap, const MathStructure*);
			if(!o) break;
			APPEND_COPY(o)
		}
	}
	va_end(ap);	
	m_type = STRUCT_VECTOR;
}
MathStructure::MathStructure(MathFunction *o, ...) {
	init();
	clear();
	va_list ap;
	va_start(ap, o); 
	o_function = o;
	while(true) {
		const MathStructure *mstruct = va_arg(ap, const MathStructure*);
		if(!mstruct) break;
		APPEND_COPY(mstruct)
	}
	va_end(ap);	
	m_type = STRUCT_FUNCTION;
}
MathStructure::MathStructure(Unit *u, Prefix *p) {
	init();
	set(u, p);
}
MathStructure::MathStructure(Variable *o) {
	init();
	set(o);
}
MathStructure::MathStructure(const Number &o) {
	init();
	set(o);
}
MathStructure::~MathStructure() {
	clear();
}

void MathStructure::set(const MathStructure &o, bool merge_precision) {
	clear(merge_precision);
	switch(o.type()) {
		case STRUCT_NUMBER: {
			o_number.set(o.number());
			break;
		}
		case STRUCT_SYMBOLIC: {
			s_sym = o.symbol();
			break;
		}
		case STRUCT_FUNCTION: {
			o_function = o.function();
			if(o.functionValue()) function_value = new MathStructure(*o.functionValue());
			break;
		}
		case STRUCT_VARIABLE: {
			o_variable = o.variable();
			break;
		}
		case STRUCT_UNIT: {
			o_unit = o.unit();
			o_prefix = o.prefix();
			b_plural = o.isPlural();
			break;
		}
		case STRUCT_COMPARISON: {
			ct_comp = o.comparisonType();
			break;
		}
		default: {}
	}
	b_protected = o.isProtected();
	for(size_t i = 0; i < o.size(); i++) {
		APPEND_COPY((&o[i]))
	}
	if(merge_precision) {
		MERGE_APPROX_AND_PREC(o);
	} else {
		b_approx = o.isApproximate();
		i_precision = o.precision();
	}
	if(o.uncertainty()) {
		o_uncertainty = new MathStructure(*o.uncertainty());
	}
	m_type = o.type();
}
void MathStructure::set_nocopy(MathStructure &o, bool merge_precision) {
	o.ref();
	clear(merge_precision);
	switch(o.type()) {
		case STRUCT_NUMBER: {
			o_number.set(o.number());
			break;
		}
		case STRUCT_SYMBOLIC: {
			s_sym = o.symbol();
			break;
		}
		case STRUCT_FUNCTION: {
			o_function = o.function();
			if(o.functionValue()) {
				function_value = (MathStructure*) o.functionValue();
				function_value->ref();
			}
			break;
		}
		case STRUCT_VARIABLE: {
			o_variable = o.variable();
			break;
		}
		case STRUCT_UNIT: {
			o_unit = o.unit();
			o_prefix = o.prefix();
			b_plural = o.isPlural();
			break;
		}
		case STRUCT_COMPARISON: {
			ct_comp = o.comparisonType();
			break;
		}
		default: {}
	}
	b_protected = o.isProtected();
	for(size_t i = 0; i < o.size(); i++) {
		APPEND_REF((&o[i]))
	}
	if(merge_precision) {
		MERGE_APPROX_AND_PREC(o);
	} else {
		b_approx = o.isApproximate();
		i_precision = o.precision();
	}
	if(o.uncertainty()) {
		o_uncertainty = (MathStructure*) o.uncertainty();
		o_uncertainty->ref();
	}
	m_type = o.type();
	o.unref();
}
void MathStructure::setToChild(size_t index, bool preserve_precision, MathStructure *mparent, size_t index_this) {
	if(index > 0 && index <= SIZE) {
		if(mparent) {
			CHILD(index - 1).ref();
			mparent->setChild_nocopy(&CHILD(index - 1), index_this);
		} else {
			set_nocopy(CHILD(index - 1), preserve_precision);
		}
	}
}
void MathStructure::set(int num, int den, int exp10, bool preserve_precision) {
	clear(preserve_precision);
	o_number.set(num, den, exp10);
	if(preserve_precision) {
		MERGE_APPROX_AND_PREC(o_number);
	} else {
		b_approx = o_number.isApproximate();
		i_precision = o_number.precision();
	}
	m_type = STRUCT_NUMBER;
}
void MathStructure::set(double float_value, bool preserve_precision) {
	clear(preserve_precision);
	o_number.setFloat(float_value);
	if(preserve_precision) {
		MERGE_APPROX_AND_PREC(o_number);
	} else {
		b_approx = o_number.isApproximate();
		i_precision = o_number.precision();
	}
	m_type = STRUCT_NUMBER;
}
void MathStructure::set(string sym, bool preserve_precision) {
	clear(preserve_precision);
	s_sym = sym;
	m_type = STRUCT_SYMBOLIC;
}
void MathStructure::setVector(const MathStructure *o, ...) {
	clear();
	va_list ap;
	va_start(ap, o); 
	if(o) {
		APPEND_COPY(o)
		while(true) {
			o = va_arg(ap, const MathStructure*);
			if(!o) break;
			APPEND_COPY(o)
		}
	}
	va_end(ap);	
	m_type = STRUCT_VECTOR;
}
void MathStructure::set(MathFunction *o, ...) {
	clear();
	va_list ap;
	va_start(ap, o); 
	o_function = o;
	while(true) {
		const MathStructure *mstruct = va_arg(ap, const MathStructure*);
		if(!mstruct) break;
		APPEND_COPY(mstruct)
	}
	va_end(ap);	
	m_type = STRUCT_FUNCTION;
}
void MathStructure::set(Unit *u, Prefix *p, bool preserve_precision) {
	clear(preserve_precision);
	o_unit = u;
	o_prefix = p;
	m_type = STRUCT_UNIT;
}
void MathStructure::set(Variable *o, bool preserve_precision) {
	clear(preserve_precision);
	o_variable = o;
	m_type = STRUCT_VARIABLE;
}
void MathStructure::set(const Number &o, bool preserve_precision) {
	clear(preserve_precision);
	o_number.set(o);
	if(preserve_precision) {
		MERGE_APPROX_AND_PREC(o_number);
	} else {
		b_approx = o_number.isApproximate();
		i_precision = o_number.precision();
	}
	m_type = STRUCT_NUMBER;
}
void MathStructure::setInfinity(bool preserve_precision) {
	clear(preserve_precision);
	o_number.setInfinity();
	m_type = STRUCT_NUMBER;
}
void MathStructure::setUndefined(bool preserve_precision) {
	clear(preserve_precision);
	m_type = STRUCT_UNDEFINED;
}

void MathStructure::setProtected(bool do_protect) {b_protected = do_protect;}
bool MathStructure::isProtected() const {return b_protected;}

void MathStructure::operator = (const MathStructure &o) {set(o);}
void MathStructure::operator = (const Number &o) {set(o);}
void MathStructure::operator = (int i) {set(i, 1);}
void MathStructure::operator = (Unit *u) {set(u);}
void MathStructure::operator = (Variable *v) {set(v);}
void MathStructure::operator = (string sym) {set(sym);}
MathStructure MathStructure::operator - () const {
	MathStructure o2(*this);
	o2.negate();
	return o2;
}
MathStructure MathStructure::operator * (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.multiply(o);
	return o2;
}
MathStructure MathStructure::operator / (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.divide(o);
	return o2;
}
MathStructure MathStructure::operator + (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.add(o);
	return o;
}
MathStructure MathStructure::operator - (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.subtract(o);
	return o2;
}
MathStructure MathStructure::operator ^ (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.raise(o);
	return o2;
}
MathStructure MathStructure::operator && (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.add(o, OPERATION_LOGICAL_AND);
	return o2;
}
MathStructure MathStructure::operator || (const MathStructure &o) const {
	MathStructure o2(*this);
	o2.add(o, OPERATION_LOGICAL_OR);
	return o2;
}
MathStructure MathStructure::operator ! () const {
	MathStructure o2(*this);
	o2.setLogicalNot();
	return o2;
}

void MathStructure::operator *= (const MathStructure &o) {multiply(o);}
void MathStructure::operator /= (const MathStructure &o) {divide(o);}
void MathStructure::operator += (const MathStructure &o) {add(o);}
void MathStructure::operator -= (const MathStructure &o) {subtract(o);}
void MathStructure::operator ^= (const MathStructure &o) {raise(o);}

void MathStructure::operator *= (const Number &o) {multiply(o);}
void MathStructure::operator /= (const Number &o) {divide(o);}
void MathStructure::operator += (const Number &o) {add(o);}
void MathStructure::operator -= (const Number &o) {subtract(o);}
void MathStructure::operator ^= (const Number &o) {raise(o);}
		
void MathStructure::operator *= (int i) {multiply(i);}
void MathStructure::operator /= (int i) {divide(i);}
void MathStructure::operator += (int i) {add(i);}
void MathStructure::operator -= (int i) {subtract(i);}
void MathStructure::operator ^= (int i) {raise(i);}
		
void MathStructure::operator *= (Unit *u) {multiply(u);}
void MathStructure::operator /= (Unit *u) {divide(u);}
void MathStructure::operator += (Unit *u) {add(u);}
void MathStructure::operator -= (Unit *u) {subtract(u);}
void MathStructure::operator ^= (Unit *u) {raise(u);}
		
void MathStructure::operator *= (Variable *v) {multiply(v);}
void MathStructure::operator /= (Variable *v) {divide(v);}
void MathStructure::operator += (Variable *v) {add(v);}
void MathStructure::operator -= (Variable *v) {subtract(v);}
void MathStructure::operator ^= (Variable *v) {raise(v);}
		
void MathStructure::operator *= (string sym) {multiply(sym);}
void MathStructure::operator /= (string sym) {divide(sym);}
void MathStructure::operator += (string sym) {add(sym);}
void MathStructure::operator -= (string sym) {subtract(sym);}
void MathStructure::operator ^= (string sym) {raise(sym);}

bool MathStructure::operator == (const MathStructure &o) const {return equals(o);}
bool MathStructure::operator == (const Number &o) const {return equals(o);}
bool MathStructure::operator == (int i) const {return equals(i);}
bool MathStructure::operator == (Unit *u) const {return equals(u);}
bool MathStructure::operator == (Variable *v) const {return equals(v);}
bool MathStructure::operator == (string sym) const {return equals(sym);}

bool MathStructure::operator != (const MathStructure &o) const {return !equals(o);}

MathStructure& MathStructure::CHILD(size_t v_index) const
{
    if(v_index < v_order.size() && v_order[v_index] < v_subs.size())
        return *v_subs[v_order[v_index]];
    
    MathStructure* m = new MathStructure;//(new UnknownVariable("x","x"));
    m->setUndefined(true);
    return *m;
}

const MathStructure &MathStructure::operator [] (size_t index) const {return CHILD(index);}
MathStructure &MathStructure::operator [] (size_t index) {return CHILD(index);}

void MathStructure::clear(bool preserve_precision) {
	m_type = STRUCT_NUMBER;
	o_number.clear();
	if(function_value) {
		function_value->unref();
		function_value = NULL;
	}
	if(o_uncertainty) {
		o_uncertainty->unref();
		o_uncertainty = NULL;
	}
	o_function = NULL;
	o_variable = NULL;
	o_unit = NULL;
	o_prefix = NULL;
	b_plural = false;
	b_protected = false;
	CLEAR;
	if(!preserve_precision) {
		i_precision = -1;
		b_approx = false;
	}
}
void MathStructure::clearVector(bool preserve_precision) {
	clear(preserve_precision);
	m_type = STRUCT_VECTOR;
}
void MathStructure::clearMatrix(bool preserve_precision) {
	clearVector(preserve_precision);
	MathStructure *mstruct = new MathStructure();
	mstruct->clearVector();
	APPEND_POINTER(mstruct);
}

const MathStructure *MathStructure::functionValue() const {
	return function_value;
}

const Number &MathStructure::number() const {
	return o_number;
}
Number &MathStructure::number() {
	return o_number;
}
void MathStructure::numberUpdated() {
	if(m_type != STRUCT_NUMBER) return;
	MERGE_APPROX_AND_PREC(o_number)
}
void MathStructure::childUpdated(size_t index, bool recursive) {
	if(index > SIZE || index < 1) return;
	if(recursive) CHILD(index - 1).childrenUpdated(true);
	MERGE_APPROX_AND_PREC(CHILD(index - 1))
}
void MathStructure::childrenUpdated(bool recursive) {
	for(size_t i = 0; i < SIZE; i++) {
		if(recursive) CHILD(i).childrenUpdated(true);
		MERGE_APPROX_AND_PREC(CHILD(i))
	}
}
const string &MathStructure::symbol() const {
	return s_sym;
}
ComparisonType MathStructure::comparisonType() const {
	return ct_comp;
}
void MathStructure::setComparisonType(ComparisonType comparison_type) {
	ct_comp = comparison_type;
}
void MathStructure::setType(StructureType mtype) {
	m_type = mtype;
}
Unit *MathStructure::unit() const {
	return o_unit;
}
Prefix *MathStructure::prefix() const {
	return o_prefix;
}
void MathStructure::setPrefix(Prefix *p) {
	if(isUnit()) o_prefix = p;
}
bool MathStructure::isPlural() const {
	return b_plural;
}
void MathStructure::setPlural(bool is_plural) {
	if(isUnit()) b_plural = is_plural;
}
void MathStructure::setFunction(MathFunction *f) {
	o_function = f;
}
void MathStructure::setUnit(Unit *u) {
	o_unit = u;
}
void MathStructure::setVariable(Variable *v) {
	o_variable = v;
}
MathFunction *MathStructure::function() const {
	return o_function;
}
Variable *MathStructure::variable() const {
	return o_variable;
}
void MathStructure::setUncertainty(const MathStructure &o) {
	if(o_uncertainty) o_uncertainty->set(o);
	else o_uncertainty = new MathStructure(o);
}
const MathStructure *MathStructure::uncertainty() const {
	return o_uncertainty;
}

bool MathStructure::isAddition() const {return m_type == STRUCT_ADDITION;}
bool MathStructure::isMultiplication() const {return m_type == STRUCT_MULTIPLICATION;}
bool MathStructure::isPower() const {return m_type == STRUCT_POWER;}
bool MathStructure::isSymbolic() const {return m_type == STRUCT_SYMBOLIC;}
bool MathStructure::isEmptySymbol() const {return m_type == STRUCT_SYMBOLIC && s_sym.empty();}
bool MathStructure::isVector() const {return m_type == STRUCT_VECTOR;}
bool MathStructure::isMatrix() const {
	if(m_type != STRUCT_VECTOR || SIZE < 1) return false;
	for(size_t i = 0; i < SIZE; i++) {
		if(!CHILD(i).isVector() || (i > 0 && CHILD(i).size() != CHILD(i - 1).size())) return false;
	}
	return true;
}
bool MathStructure::isFunction() const {return m_type == STRUCT_FUNCTION;}
bool MathStructure::isUnit() const {return m_type == STRUCT_UNIT;}
bool MathStructure::isUnit_exp() const {return m_type == STRUCT_UNIT || (m_type == STRUCT_POWER && CHILD(0).isUnit());}
bool MathStructure::isNumber_exp() const {return m_type == STRUCT_NUMBER || (m_type == STRUCT_POWER && CHILD(0).isNumber());}
bool MathStructure::isVariable() const {return m_type == STRUCT_VARIABLE;}
bool MathStructure::isComparison() const {return m_type == STRUCT_COMPARISON;}
bool MathStructure::isLogicalAnd() const {return m_type == STRUCT_LOGICAL_AND;}
bool MathStructure::isLogicalOr() const {return m_type == STRUCT_LOGICAL_OR;}
bool MathStructure::isLogicalXor() const {return m_type == STRUCT_LOGICAL_XOR;}
bool MathStructure::isLogicalNot() const {return m_type == STRUCT_LOGICAL_NOT;}
bool MathStructure::isBitwiseAnd() const {return m_type == STRUCT_BITWISE_AND;}
bool MathStructure::isBitwiseOr() const {return m_type == STRUCT_BITWISE_OR;}
bool MathStructure::isBitwiseXor() const {return m_type == STRUCT_BITWISE_XOR;}
bool MathStructure::isBitwiseNot() const {return m_type == STRUCT_BITWISE_NOT;}
bool MathStructure::isInverse() const {return m_type == STRUCT_INVERSE;}
bool MathStructure::isDivision() const {return m_type == STRUCT_DIVISION;}
bool MathStructure::isNegate() const {return m_type == STRUCT_NEGATE;}
bool MathStructure::isInfinity() const {return m_type == STRUCT_NUMBER && o_number.isInfinite();}
bool MathStructure::isUndefined() const {return m_type == STRUCT_UNDEFINED || (m_type == STRUCT_NUMBER && o_number.isUndefined());}
bool MathStructure::isInteger() const {return m_type == STRUCT_NUMBER && o_number.isInteger();}
bool MathStructure::isNumber() const {return m_type == STRUCT_NUMBER;}
bool MathStructure::isZero() const {return m_type == STRUCT_NUMBER && o_number.isZero();}
bool MathStructure::isOne() const {return m_type == STRUCT_NUMBER && o_number.isOne();}
bool MathStructure::isMinusOne() const {return m_type == STRUCT_NUMBER && o_number.isMinusOne();}

bool MathStructure::hasNegativeSign() const {
	return (m_type == STRUCT_NUMBER && o_number.hasNegativeSign()) || m_type == STRUCT_NEGATE || (m_type == STRUCT_MULTIPLICATION && SIZE > 0 && CHILD(0).hasNegativeSign());
}

bool MathStructure::representsBoolean() const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isOne() || o_number.isZero();}
		case STRUCT_VARIABLE: {return o_variable->representsBoolean();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsBoolean()) || o_function->representsBoolean(*this);}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsBoolean()) return false;
			}
			return true;
		}
		case STRUCT_LOGICAL_NOT: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_COMPARISON: {return true;}
		default: {return false;}
	}
}

bool MathStructure::representsNumber(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return true;}
		case STRUCT_VARIABLE: {return o_variable->representsNumber(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNumber();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNumber(allow_units)) || o_function->representsNumber(*this, allow_units);}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_ADDITION: {}
		case STRUCT_POWER: {}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNumber(allow_units)) return false;
			}
			return true;
		}
		default: {return false;}
	}
}
bool MathStructure::representsInteger(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isInteger();}
		case STRUCT_VARIABLE: {return o_variable->representsInteger(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isInteger();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsInteger(allow_units)) || o_function->representsInteger(*this, allow_units);}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsInteger(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(0).representsInteger(allow_units) && CHILD(1).representsInteger(false) && CHILD(1).representsPositive(false);
		}
		default: {return false;}
	}
}
bool MathStructure::representsPositive(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isPositive();}
		case STRUCT_VARIABLE: {return o_variable->representsPositive(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isPositive();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsPositive(allow_units)) || o_function->representsPositive(*this, allow_units);}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsPositive(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = true;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsNegative(allow_units)) {
					b = !b;
				} else if(!CHILD(i).representsPositive(allow_units)) {
					return false;
				}
			}
			return b;
		}
		case STRUCT_POWER: {
			return (CHILD(0).representsPositive(allow_units) && CHILD(1).representsReal(false))
			|| (CHILD(0).representsNonZero(allow_units) && CHILD(0).representsReal(allow_units) && ((CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().numeratorIsEven()) || (CHILD(1).representsEven(false) && CHILD(1).representsInteger(false))));
		}
		default: {return false;}
	}
}
bool MathStructure::representsNegative(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isNegative();}
		case STRUCT_VARIABLE: {return o_variable->representsNegative(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNegative();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNegative(allow_units)) || o_function->representsNegative(*this, allow_units);}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNegative(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsNegative(allow_units)) {
					b = !b;
				} else if(!CHILD(i).representsPositive(allow_units)) {
					return false;
				}
			}
			return b;
		}
		case STRUCT_POWER: {
			return CHILD(1).representsInteger(false) && CHILD(1).representsOdd(false) && CHILD(0).representsNegative(allow_units);
		}
		default: {return false;}
	}
}
bool MathStructure::representsNonNegative(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isNonNegative();}
		case STRUCT_VARIABLE: {return o_variable->representsNonNegative(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonNegative();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNonNegative(allow_units)) || o_function->representsNonNegative(*this, allow_units);}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonNegative(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = true;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsNegative(allow_units)) {
					b = !b;
				} else if(!CHILD(i).representsNonNegative(allow_units)) {
					return false;
				}
			}
			return b;
		}
		case STRUCT_POWER: {
			return (CHILD(0).isZero() && CHILD(1).representsNonNegative(false))
			|| (CHILD(0).representsNonNegative(allow_units) && CHILD(1).representsReal(false))
			|| (CHILD(0).representsReal(allow_units) && ((CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().numeratorIsEven()) || (CHILD(1).representsEven(false) && CHILD(1).representsInteger(false))));
		}
		default: {return false;}
	}
}
bool MathStructure::representsNonPositive(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isNonPositive();}
		case STRUCT_VARIABLE: {return o_variable->representsNonPositive(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonPositive();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNonPositive(allow_units)) || o_function->representsNonPositive(*this, allow_units);}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonPositive(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsNegative(allow_units)) {
					b = !b;
				} else if(!CHILD(i).representsPositive(allow_units)) {
					return false;
				}
			}
			return b;
		}
		case STRUCT_POWER: {
			return (CHILD(0).isZero() && CHILD(1).representsPositive(false)) || representsNegative(allow_units);
		}
		default: {return false;}
	}
}
bool MathStructure::representsRational(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isRational();}
		case STRUCT_VARIABLE: {return o_variable->representsRational(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isRational();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsRational(allow_units)) || o_function->representsRational(*this, allow_units);}
		case STRUCT_UNIT: {return false;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsRational(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsRational(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(1).representsInteger(false) && CHILD(0).representsRational(allow_units) && (CHILD(0).representsPositive(allow_units) || (CHILD(0).representsNegative(allow_units) && CHILD(1).representsEven(false) && CHILD(1).representsPositive(false)));
		}
		default: {return false;}
	}
}
bool MathStructure::representsReal(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isReal();}
		case STRUCT_VARIABLE: {return o_variable->representsReal(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isReal();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsReal(allow_units)) || o_function->representsReal(*this, allow_units);}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsReal(allow_units)) return false;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsReal(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return (CHILD(0).representsPositive(allow_units) && CHILD(1).representsReal(false)) 
			|| (CHILD(0).representsReal(allow_units) && ((CHILD(1).isNumber() && CHILD(1).number().isRational() && !CHILD(1).number().denominatorIsEven()) || (CHILD(1).representsEven(false) && CHILD(1).representsInteger(false))) && (CHILD(1).representsPositive(false) || CHILD(0).representsNonZero(allow_units)));
		}
		default: {return false;}
	}
}
bool MathStructure::representsComplex(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isComplex();}
		case STRUCT_VARIABLE: {return o_variable->representsComplex(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isComplex();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsComplex(allow_units)) || o_function->representsComplex(*this, allow_units);}
		case STRUCT_ADDITION: {
			bool c = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsComplex(allow_units)) {
					if(c) return false;
					c = true;
				} else if(!CHILD(i).representsReal(allow_units) || !CHILD(i).representsNonZero(allow_units)) {
					return false;
				}
			}
			return c;
		}
		case STRUCT_MULTIPLICATION: {
			bool c = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsComplex(allow_units)) {
					if(c) return false;
					c = true;
				} else if(!CHILD(i).representsReal(allow_units)) {
					return false;
				}
			}
			return c;
		}
		case STRUCT_UNIT: {return false;}
		case STRUCT_POWER: {return CHILD(1).isNumber() && CHILD(1).number().denominatorIsEven() && CHILD(0).representsNegative();}
		default: {return false;}
	}
}
bool MathStructure::representsNonZero(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return !o_number.isZero();}
		case STRUCT_VARIABLE: {return o_variable->representsNonZero(allow_units);}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonZero();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNonZero(allow_units)) || o_function->representsNonZero(*this, allow_units);}
		case STRUCT_UNIT: {return allow_units;}
		case STRUCT_ADDITION: {
			bool neg = false, started = false;
			for(size_t i = 0; i < SIZE; i++) {
				if((!started || neg) && CHILD(i).representsNegative(allow_units)) {
					neg = true;
				} else if(neg || !CHILD(i).representsPositive(allow_units)) {
					return false;
				}
				started = true;
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonZero(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(0).representsNonZero(allow_units) || (!CHILD(0).isZero() && CHILD(1).representsNonPositive());
		}
		default: {return false;}
	}
}
bool MathStructure::representsZero(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isZero();}
		case STRUCT_VARIABLE: {return o_variable->isKnown() && !o_variable->representsNonZero(allow_units) && ((KnownVariable*) o_variable)->get().representsZero();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsZero(allow_units));}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsZero(allow_units)) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).representsZero(allow_units)) {
					return true;
				}
			}
			return false;
		}
		case STRUCT_POWER: {
			return CHILD(0).representsZero(allow_units) && CHILD(1).representsPositive(allow_units);
		}
		default: {return false;}
	}
}
bool MathStructure::representsEven(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isEven();}
		case STRUCT_VARIABLE: {return o_variable->representsEven(allow_units);}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsEven(allow_units)) || o_function->representsEven(*this, allow_units);}
		default: {return false;}
	}
}
bool MathStructure::representsOdd(bool allow_units) const {
	switch(m_type) {
		case STRUCT_NUMBER: {return o_number.isOdd();}
		case STRUCT_VARIABLE: {return o_variable->representsOdd(allow_units);}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsOdd(allow_units)) || o_function->representsOdd(*this, allow_units);}
		default: {return false;}
	}
}
bool MathStructure::representsUndefined(bool include_childs, bool include_infinite, bool be_strict) const {
	switch(m_type) {
		case STRUCT_NUMBER: {
			if(include_infinite) {
				return o_number.isInfinite();
			}
			return false;
		}
		case STRUCT_UNDEFINED: {return true;}
		case STRUCT_VARIABLE: {return o_variable->representsUndefined(include_childs, include_infinite, be_strict);}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsUndefined(include_childs, include_infinite, be_strict)) || o_function->representsUndefined(*this);}
		case STRUCT_POWER: {
			if(be_strict) {
				if((!CHILD(0).representsNonZero(true) && !CHILD(1).representsNonNegative(false)) || (CHILD(0).isInfinity() && !CHILD(1).representsNonZero(true))) {
					return true;
				}
			} else {
				if((CHILD(0).representsZero(true) && CHILD(1).representsNegative(false)) || (CHILD(0).isInfinity() && CHILD(1).representsZero(true))) {
					return true;
				}
			}
		}
		default: {
			if(include_childs) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).representsUndefined(include_childs, include_infinite, be_strict)) return true;
				}
			}
			return false;
		}
	}
}
bool MathStructure::representsNonMatrix() const {
	switch(m_type) {
		case STRUCT_VECTOR: {return !isMatrix();}
		case STRUCT_POWER: {return CHILD(0).representsNonMatrix();}
		case STRUCT_VARIABLE: {return o_variable->representsNonMatrix();}
		case STRUCT_SYMBOLIC: {return CALCULATOR->defaultAssumptions()->isNonMatrix();}
		case STRUCT_FUNCTION: {return (function_value && function_value->representsNonMatrix()) || o_function->representsNonMatrix(*this);}
		case STRUCT_INVERSE: {}
		case STRUCT_NEGATE: {}
		case STRUCT_DIVISION: {}
		case STRUCT_MULTIPLICATION: {}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).representsNonMatrix()) {
					return false;
				}
			}
			return true;
		}
		default: {}
	}
	return true;
}

void MathStructure::setApproximate(bool is_approx) {
	b_approx = is_approx;
	if(b_approx) {
		if(i_precision < 1) i_precision = PRECISION;
	} else {
		i_precision = -1;
	}
}
bool MathStructure::isApproximate() const {
	return b_approx;
}

int MathStructure::precision() const {
	return i_precision;
}
void MathStructure::setPrecision(int prec) {
	i_precision = prec;
	if(i_precision > 0) b_approx = true;
}

void MathStructure::transform(StructureType mtype, const MathStructure &o) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND(o);
}
void MathStructure::transform(StructureType mtype, const Number &o) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(o);
}
void MathStructure::transform(StructureType mtype, int i) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_POINTER(new MathStructure(i, 1));
}
void MathStructure::transform(StructureType mtype, Unit *u) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(u);
}
void MathStructure::transform(StructureType mtype, Variable *v) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(v);
}
void MathStructure::transform(StructureType mtype, string sym) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_NEW(sym);
}
void MathStructure::transform_nocopy(StructureType mtype, MathStructure *o) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
	APPEND_POINTER(o);
}
void MathStructure::transform(StructureType mtype) {
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = mtype;
	APPEND_POINTER(struct_this);
}
void MathStructure::add(const MathStructure &o, MathOperation op, bool append) {
	switch(op) {
		case OPERATION_ADD: {
			add(o, append);
			break;
		}
		case OPERATION_SUBTRACT: {
			subtract(o, append);
			break;
		}
		case OPERATION_MULTIPLY: {
			multiply(o, append);
			break;
		}
		case OPERATION_DIVIDE: {
			divide(o, append);
			break;
		}
		case OPERATION_RAISE: {
			raise(o);
			break;
		}
		case OPERATION_EXP10: {
			MathStructure *mstruct = new MathStructure(10, 1);
			mstruct->raise(o);
			multiply_nocopy(mstruct, append);
			break;
		}
		case OPERATION_LOGICAL_AND: {
			if(m_type == STRUCT_LOGICAL_AND && append) {
				APPEND(o);
			} else {
				transform(STRUCT_LOGICAL_AND, o);
			}
			break;
		}
		case OPERATION_LOGICAL_OR: {
			if(m_type == STRUCT_LOGICAL_OR && append) {
				APPEND(o);
			} else {
				transform(STRUCT_LOGICAL_OR, o);
			}
			break;
		}
		case OPERATION_LOGICAL_XOR: {
			transform(STRUCT_LOGICAL_XOR, o);
			break;
		}
		case OPERATION_BITWISE_AND: {
			if(m_type == STRUCT_BITWISE_AND && append) {
				APPEND(o);
			} else {
				transform(STRUCT_BITWISE_AND, o);
			}
			break;
		}
		case OPERATION_BITWISE_OR: {
			if(m_type == STRUCT_BITWISE_OR && append) {
				APPEND(o);
			} else {
				transform(STRUCT_BITWISE_OR, o);
			}
			break;
		}
		case OPERATION_BITWISE_XOR: {
			transform(STRUCT_BITWISE_XOR, o);
			break;
		}
		case OPERATION_EQUALS: {}
		case OPERATION_NOT_EQUALS: {}
		case OPERATION_GREATER: {}
		case OPERATION_LESS: {}
		case OPERATION_EQUALS_GREATER: {}
		case OPERATION_EQUALS_LESS: {
			if(append && m_type == STRUCT_COMPARISON && append) {
				MathStructure *o2 = new MathStructure(CHILD(1));
				o2->add(o, op);
				transform_nocopy(STRUCT_LOGICAL_AND, o2);
			} else if(append && m_type == STRUCT_LOGICAL_AND && LAST.type() == STRUCT_COMPARISON) {
				MathStructure *o2 = new MathStructure(LAST[1]);
				o2->add(o, op);
				APPEND_POINTER(o2);
			} else {
				transform(STRUCT_COMPARISON, o);
				switch(op) {
					case OPERATION_EQUALS: {ct_comp = COMPARISON_EQUALS; break;}
					case OPERATION_NOT_EQUALS: {ct_comp = COMPARISON_NOT_EQUALS; break;}
					case OPERATION_GREATER: {ct_comp = COMPARISON_GREATER; break;}
					case OPERATION_LESS: {ct_comp = COMPARISON_LESS; break;}
					case OPERATION_EQUALS_GREATER: {ct_comp = COMPARISON_EQUALS_GREATER; break;}
					case OPERATION_EQUALS_LESS: {ct_comp = COMPARISON_EQUALS_LESS; break;}
					default: {}
				}
			}
			break;
		}
		default: {
		}
	}
}
void MathStructure::add(const MathStructure &o, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND(o);
	} else {
		transform(STRUCT_ADDITION, o);
	}
}
void MathStructure::subtract(const MathStructure &o, bool append) {
	MathStructure *o2 = new MathStructure(o);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(const MathStructure &o, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND(o);
	} else {
		transform(STRUCT_MULTIPLICATION, o);
	}
}
void MathStructure::divide(const MathStructure &o, bool append) {
//	transform(STRUCT_DIVISION, o);
	MathStructure *o2 = new MathStructure(o);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(const MathStructure &o) {
	transform(STRUCT_POWER, o);
}
void MathStructure::add(const Number &o, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_NEW(o);
	} else {
		transform(STRUCT_ADDITION, o);
	}
}
void MathStructure::subtract(const Number &o, bool append) {
	MathStructure *o2 = new MathStructure(o);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(const Number &o, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_NEW(o);
	} else {
		transform(STRUCT_MULTIPLICATION, o);
	}
}
void MathStructure::divide(const Number &o, bool append) {
	MathStructure *o2 = new MathStructure(o);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(const Number &o) {
	transform(STRUCT_POWER, o);
}
void MathStructure::add(int i, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_POINTER(new MathStructure(i, 1));
	} else {
		transform(STRUCT_ADDITION, i);
	}
}
void MathStructure::subtract(int i, bool append) {
	MathStructure *o2 = new MathStructure(i, 1);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(int i, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_POINTER(new MathStructure(i, 1));
	} else {
		transform(STRUCT_MULTIPLICATION, i);
	}
}
void MathStructure::divide(int i, bool append) {
	MathStructure *o2 = new MathStructure(i, 1);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(int i) {
	transform(STRUCT_POWER, i);
}
void MathStructure::add(Variable *v, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_NEW(v);
	} else {
		transform(STRUCT_ADDITION, v);
	}
}
void MathStructure::subtract(Variable *v, bool append) {
	MathStructure *o2 = new MathStructure(v);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(Variable *v, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_NEW(v);
	} else {
		transform(STRUCT_MULTIPLICATION, v);
	}
}
void MathStructure::divide(Variable *v, bool append) {
	MathStructure *o2 = new MathStructure(v);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(Variable *v) {
	transform(STRUCT_POWER, v);
}
void MathStructure::add(Unit *u, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_NEW(u);
	} else {
		transform(STRUCT_ADDITION, u);
	}
}
void MathStructure::subtract(Unit *u, bool append) {
	MathStructure *o2 = new MathStructure(u);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(Unit *u, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_NEW(u);
	} else {
		transform(STRUCT_MULTIPLICATION, u);
	}
}
void MathStructure::divide(Unit *u, bool append) {
	MathStructure *o2 = new MathStructure(u);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(Unit *u) {
	transform(STRUCT_POWER, u);
}
void MathStructure::add(string sym, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_NEW(sym);
	} else {
		transform(STRUCT_ADDITION, sym);
	}
}
void MathStructure::subtract(string sym, bool append) {
	MathStructure *o2 = new MathStructure(sym);
	o2->negate();
	add_nocopy(o2, append);
}
void MathStructure::multiply(string sym, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_NEW(sym);
	} else {
		transform(STRUCT_MULTIPLICATION, sym);
	}
}
void MathStructure::divide(string sym, bool append) {
	MathStructure *o2 = new MathStructure(sym);
	o2->inverse();
	multiply_nocopy(o2, append);
}
void MathStructure::raise(string sym) {
	transform(STRUCT_POWER, sym);
}
void MathStructure::add_nocopy(MathStructure *o, MathOperation op, bool append) {
	switch(op) {
		case OPERATION_ADD: {
			add_nocopy(o, append);
			break;
		}
		case OPERATION_SUBTRACT: {
			subtract_nocopy(o, append);
			break;
		}
		case OPERATION_MULTIPLY: {
			multiply_nocopy(o, append);
			break;
		}
		case OPERATION_DIVIDE: {
			divide_nocopy(o, append);
			break;
		}
		case OPERATION_RAISE: {
			raise_nocopy(o);
			break;
		}
		case OPERATION_EXP10: {
			MathStructure *mstruct = new MathStructure(10, 1);
			mstruct->raise_nocopy(o);
			multiply_nocopy(mstruct, append);
			break;
		}
		case OPERATION_LOGICAL_AND: {
			if(m_type == STRUCT_LOGICAL_AND && append) {
				APPEND_POINTER(o);
			} else {
				transform_nocopy(STRUCT_LOGICAL_AND, o);
			}
			break;
		}
		case OPERATION_LOGICAL_OR: {
			if(m_type == STRUCT_LOGICAL_OR && append) {
				APPEND_POINTER(o);
			} else {
				transform_nocopy(STRUCT_LOGICAL_OR, o);
			}
			break;
		}
		case OPERATION_LOGICAL_XOR: {
			transform_nocopy(STRUCT_LOGICAL_XOR, o);
			break;
		}
		case OPERATION_BITWISE_AND: {
			if(m_type == STRUCT_BITWISE_AND && append) {
				APPEND_POINTER(o);
			} else {
				transform_nocopy(STRUCT_BITWISE_AND, o);
			}
			break;
		}
		case OPERATION_BITWISE_OR: {
			if(m_type == STRUCT_BITWISE_OR && append) {
				APPEND_POINTER(o);
			} else {
				transform_nocopy(STRUCT_BITWISE_OR, o);
			}
			break;
		}
		case OPERATION_BITWISE_XOR: {
			transform_nocopy(STRUCT_BITWISE_XOR, o);
			break;
		}
		case OPERATION_EQUALS: {}
		case OPERATION_NOT_EQUALS: {}
		case OPERATION_GREATER: {}
		case OPERATION_LESS: {}
		case OPERATION_EQUALS_GREATER: {}
		case OPERATION_EQUALS_LESS: {
			if(append && m_type == STRUCT_COMPARISON && append) {
				MathStructure *o2 = new MathStructure(CHILD(1));
				o2->add_nocopy(o, op);
				transform_nocopy(STRUCT_LOGICAL_AND, o2);
			} else if(append && m_type == STRUCT_LOGICAL_AND && LAST.type() == STRUCT_COMPARISON) {
				MathStructure *o2 = new MathStructure(LAST[1]);
				o2->add_nocopy(o, op);
				APPEND_POINTER(o2);
			} else {
				transform_nocopy(STRUCT_COMPARISON, o);
				switch(op) {
					case OPERATION_EQUALS: {ct_comp = COMPARISON_EQUALS; break;}
					case OPERATION_NOT_EQUALS: {ct_comp = COMPARISON_NOT_EQUALS; break;}
					case OPERATION_GREATER: {ct_comp = COMPARISON_GREATER; break;}
					case OPERATION_LESS: {ct_comp = COMPARISON_LESS; break;}
					case OPERATION_EQUALS_GREATER: {ct_comp = COMPARISON_EQUALS_GREATER; break;}
					case OPERATION_EQUALS_LESS: {ct_comp = COMPARISON_EQUALS_LESS; break;}
					default: {}
				}
			}
			break;
		}
		default: {
		}
	}
}
void MathStructure::add_nocopy(MathStructure *o, bool append) {
	if(m_type == STRUCT_ADDITION && append) {
		APPEND_POINTER(o);
	} else {
		transform_nocopy(STRUCT_ADDITION, o);
	}
}
void MathStructure::subtract_nocopy(MathStructure *o, bool append) {
	o->negate();
	add_nocopy(o, append);
}
void MathStructure::multiply_nocopy(MathStructure *o, bool append) {
	if(m_type == STRUCT_MULTIPLICATION && append) {
		APPEND_POINTER(o);
	} else {
		transform_nocopy(STRUCT_MULTIPLICATION, o);
	}
}
void MathStructure::divide_nocopy(MathStructure *o, bool append) {
	o->inverse();
	multiply_nocopy(o, append);
}
void MathStructure::raise_nocopy(MathStructure *o) {
	transform_nocopy(STRUCT_POWER, o);
}
void MathStructure::negate() {
	//transform(STRUCT_NEGATE);
	MathStructure *struct_this = new MathStructure();
	struct_this->set_nocopy(*this);
	clear(true);
	m_type = STRUCT_MULTIPLICATION;
	APPEND(m_minus_one);
	APPEND_POINTER(struct_this);
}
void MathStructure::inverse() {
	//transform(STRUCT_INVERSE);
	raise(m_minus_one);
}
void MathStructure::setLogicalNot() {
	transform(STRUCT_LOGICAL_NOT);
}		
void MathStructure::setBitwiseNot() {
	transform(STRUCT_BITWISE_NOT);
}		

bool MathStructure::equals(const MathStructure &o) const {
	if(m_type != o.type()) return false;
	if(SIZE != o.size()) return false;
	switch(m_type) {
		case STRUCT_UNDEFINED: {return true;}
		case STRUCT_SYMBOLIC: {return s_sym == o.symbol();}
		case STRUCT_NUMBER: {return o_number.equals(o.number());}
		case STRUCT_VARIABLE: {return o_variable == o.variable();}
		case STRUCT_UNIT: {return o_unit == o.unit() && o_prefix == o.prefix();}
		case STRUCT_COMPARISON: {if(ct_comp != o.comparisonType()) return false; break;}
		case STRUCT_FUNCTION: {if(o_function != o.function()) return false; break;}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_LOGICAL_AND: {
			if(SIZE < 1) return false;
			if(SIZE == 2) {
				return (CHILD(0) == o[0] && CHILD(1) == o[1]) || (CHILD(0) == o[1] && CHILD(1) == o[0]);
			}
			vector<size_t> i2taken;
			for(size_t i = 0; i < SIZE; i++) {
				bool b = false, b2 = false;
				for(size_t i2 = 0; i2 < o.size(); i2++) {
					if(CHILD(i).equals(o[i2])) {
						b2 = true;
						for(size_t i3 = 0; i3 < i2taken.size(); i3++) {
							if(i2taken[i3] == i2) {
								b2 = false;
							}
						}
						if(b2) {
							b = true;
							i2taken.push_back(i2);
							break;
						}
					}
				}
				if(!b) return false;
			}
			return true;
		}
		default: {}
	}
	if((o_uncertainty == NULL) !=(o.uncertainty() == NULL)) return false;
	if(o_uncertainty && !o_uncertainty->equals(*o.uncertainty())) return false;
	if(SIZE < 1) return false;
	for(size_t i = 0; i < SIZE; i++) {
		if(!CHILD(i).equals(o[i])) return false;
	}
	return true;
}
bool MathStructure::equals(const Number &o) const {
	if(m_type != STRUCT_NUMBER) return false;
	return o_number.equals(o);
}
bool MathStructure::equals(int i) const {
	if(m_type != STRUCT_NUMBER) return false;
	return o_number.equals(Number(i, 1));
}
bool MathStructure::equals(Unit *u) const {
	if(m_type != STRUCT_UNIT) return false;
	return o_unit == u;
}
bool MathStructure::equals(Variable *v) const {
	if(m_type != STRUCT_VARIABLE) return false;
	return o_variable == v;
}
bool MathStructure::equals(string sym) const {
	if(m_type != STRUCT_SYMBOLIC) return false;
	return s_sym == sym;
}
ComparisonResult MathStructure::compare(const MathStructure &o) const {
	if(isNumber() && o.isNumber()) {
		return o_number.compare(o.number());
	}
	if(equals(o)) return COMPARISON_RESULT_EQUAL;
	if(o.representsReal(true) && representsComplex(true)) return COMPARISON_RESULT_NOT_EQUAL;
	if(representsReal(true) && o.representsComplex(true)) return COMPARISON_RESULT_NOT_EQUAL;
	MathStructure mtest(*this);
	mtest -= o;
	EvaluationOptions eo = default_evaluation_options;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	mtest.calculatesub(eo, eo);
	bool incomp = false;
	if(mtest.isAddition()) {
		for(size_t i = 1; i < mtest.size(); i++) {
			if(mtest[i - 1].isUnitCompatible(mtest[i]) == 0) {
				incomp = true;
				break;
			}
		}
	}
	if(mtest.isZero()) return COMPARISON_RESULT_EQUAL;
	else if(mtest.representsPositive(true)) {if(incomp) return COMPARISON_RESULT_NOT_EQUAL; return COMPARISON_RESULT_LESS;}
	else if(mtest.representsNegative(true)) {if(incomp) return COMPARISON_RESULT_NOT_EQUAL; return COMPARISON_RESULT_GREATER;}
	else if(mtest.representsNonZero(true)) return COMPARISON_RESULT_NOT_EQUAL;
	else if(mtest.representsNonPositive(true)) {if(incomp) return COMPARISON_RESULT_NOT_EQUAL; return COMPARISON_RESULT_EQUAL_OR_LESS;}
	else if(mtest.representsNonNegative(true)) {if(incomp) return COMPARISON_RESULT_NOT_EQUAL; return COMPARISON_RESULT_EQUAL_OR_GREATER;}
	return COMPARISON_RESULT_UNKNOWN;
}
ComparisonResult MathStructure::compareApproximately(const MathStructure &o) const {
	if(isNumber() && o.isNumber()) {
		return o_number.compareApproximately(o.number());
	}
	if(equals(o)) return COMPARISON_RESULT_EQUAL;
	if(o.representsReal(true) && representsComplex(true)) return COMPARISON_RESULT_NOT_EQUAL;
	if(representsReal(true) && o.representsComplex(true)) return COMPARISON_RESULT_NOT_EQUAL;
	MathStructure mtest(*this);
	mtest -= o;
	EvaluationOptions eo = default_evaluation_options;
	eo.approximation = APPROXIMATION_APPROXIMATE;
	mtest.calculatesub(eo, eo);
	bool incomp = false;
	if(mtest.isAddition()) {
		for(size_t i = 1; i < mtest.size(); i++) {
			if(mtest[i - 1].isUnitCompatible(mtest[i]) == 0) {
				incomp = true;
				break;
			}
		}
	}
	if(mtest.isZero()) return COMPARISON_RESULT_EQUAL;
	else if(mtest.representsPositive(true)) {if(incomp) return COMPARISON_RESULT_NOT_EQUAL; return COMPARISON_RESULT_LESS;}
	else if(mtest.representsNegative(true)) {if(incomp) return COMPARISON_RESULT_NOT_EQUAL; return COMPARISON_RESULT_GREATER;}
	else if(mtest.representsNonZero(true)) return COMPARISON_RESULT_NOT_EQUAL;
	else if(mtest.representsNonPositive(true)) {if(incomp) return COMPARISON_RESULT_NOT_EQUAL; return COMPARISON_RESULT_EQUAL_OR_LESS;}
	else if(mtest.representsNonNegative(true)) {if(incomp) return COMPARISON_RESULT_NOT_EQUAL; return COMPARISON_RESULT_EQUAL_OR_GREATER;}
	return COMPARISON_RESULT_UNKNOWN;
}

int MathStructure::merge_addition(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool reversed) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.add(mstruct.number()) && (eo.approximation == APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	if(isZero()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(mstruct.isZero()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite()) {
		if(mstruct.representsNumber(false)) {
			MERGE_APPROX_AND_PREC(mstruct)
			return 2;
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite()) {
		if(representsNumber(false)) {			
			if(mparent) {
				mparent->swapChildren(index_this + 1, index_mstruct + 1);
			} else {
				clear(true);
				o_number = mstruct.number();
				MERGE_APPROX_AND_PREC(mstruct)
			}
			return 3;
		}
	}
	if(representsUndefined() || mstruct.representsUndefined()) return -1;
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE == mstruct.size()) {
						for(size_t i = 0; i < SIZE; i++) {
							CHILD(i).calculateAdd(mstruct[i], eo, this, i);
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_ADDITION: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					return -1;
				}
				case STRUCT_ADDITION: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(reversed) {
							INSERT_REF(&mstruct[i], i)
							calculateAddIndex(i, eo, false);
						} else {
							APPEND_REF(&mstruct[i]);
							calculateAddLast(eo, false);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}
					return 1;
				}
				default: {
					MERGE_APPROX_AND_PREC(mstruct)
					if(reversed) {
						PREPEND_REF(&mstruct);
						calculateAddIndex(0, eo, true, mparent, index_this);
					} else {
						APPEND_REF(&mstruct);
						calculateAddLast(eo, true, mparent, index_this);
					}
					return 1;
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {return -1;}
				case STRUCT_ADDITION: {
					return 0;
				}
				case STRUCT_MULTIPLICATION: {
					size_t i1 = 0, i2 = 0;
					bool b = true;
					if(CHILD(0).isNumber()) i1 = 1;
					if(mstruct[0].isNumber()) i2 = 1;
					if(SIZE - i1 == mstruct.size() - i2) {
						for(size_t i = i1; i < SIZE; i++) {
							if(CHILD(i) != mstruct[i + i2 - i1]) {
								b = false;
								break;
							}
						}
						if(b) {
							if(i1 == 0) {
								PREPEND(m_one);
							}
							if(i2 == 0) {
								CHILD(0).number()++;
							} else {
								CHILD(0).number() += mstruct[0].number();
							}
							MERGE_APPROX_AND_PREC(mstruct)
							calculateMultiplyIndex(0, eo, true, mparent, index_this);
							return 1;
						}
					}
					b = true; size_t divs = 0;
					for(; b && i1 < SIZE; i1++) {
						if(CHILD(i1).isPower() && CHILD(i1)[1].hasNegativeSign()) {
							divs++;
							b = false;
							for(; i2 < mstruct.size(); i2++) {								
								if(mstruct[i2].isPower() && mstruct[i2][1].hasNegativeSign()) {
									if(mstruct[i2] == CHILD(i1)) {
										b = true;
									}
									i2++;
									break;
								}
							}
						}
					}
					if(b && divs > 0) {
						for(; i2 < mstruct.size(); i2++) {
							if(mstruct[i2].isPower() && mstruct[i2][1].hasNegativeSign()) {
								b = false;
								break;
							}
						}
					}
					if(b && divs > 0) {
						if(SIZE - divs == 0) {
							if(mstruct.size() - divs == 0) {
								calculateMultiply(MathStructure(2, 1), eo);
							} else if(mstruct.size() - divs == 1) {
								PREPEND(m_one);
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										CHILD(0).add_nocopy(&mstruct[i], true);
										CHILD(0).calculateAddLast(eo, true, this, 0);
										break;
									}
								}
								calculateMultiplyIndex(0, eo, true, mparent, index_this);
							} else {								
								for(size_t i = 0; i < mstruct.size();) {
									if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
										mstruct.delChild(i + 1);
									} else {
										i++;
									}
								}
								PREPEND(m_one);
								mstruct.ref();
								CHILD(0).add_nocopy(&mstruct, true);
								CHILD(0).calculateAddLast(eo, true, this, 0);
								calculateMultiplyIndex(0, eo, true, mparent, index_this);
							}
						} else if(SIZE - divs == 1) {
							size_t index = 0;
							for(; index < SIZE; index++) {
								if(!CHILD(index).isPower() || !CHILD(index)[1].hasNegativeSign()) {
									break;
								}
							}
							if(mstruct.size() - divs == 0) {
								if(IS_REAL(CHILD(index))) {
									CHILD(index).number()++;
								} else {
									CHILD(index).calculateAdd(m_one, eo, this, index);
								}
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							} else if(mstruct.size() - divs == 1) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										CHILD(index).add_nocopy(&mstruct[i], true);
										CHILD(index).calculateAddLast(eo, true, this, index);
										break;
									}
								}
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							} else {
								for(size_t i = 0; i < mstruct.size();) {
									if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
										mstruct.delChild(i + 1);
									} else {
										i++;
									}
								}
								mstruct.ref();
								CHILD(index).add_nocopy(&mstruct, true);
								CHILD(index).calculateAddLast(eo, true, this, index);
								calculateMultiplyIndex(index, eo, true, mparent, index_this);
							}
						} else {
							for(size_t i = 0; i < SIZE;) {
								if(CHILD(i).isPower() && CHILD(i)[1].hasNegativeSign()) {
									ERASE(i);
								} else {
									i++;
								}
							}
							if(mstruct.size() - divs == 0) {
								calculateAdd(m_one, eo);
							} else if(mstruct.size() - divs == 1) {
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										add_nocopy(&mstruct[i], true);
										calculateAddLast(eo);
										break;
									}
								}
							} else {
								MathStructure *mstruct2 = new MathStructure();
								mstruct2->setType(STRUCT_MULTIPLICATION);
								for(size_t i = 0; i < mstruct.size(); i++) {
									if(!mstruct[i].isPower() || !mstruct[i][1].hasNegativeSign()) {
										mstruct[i].ref();
										mstruct2->addChild_nocopy(&mstruct[i]);
									}
								}
								add_nocopy(mstruct2, true);
								calculateAddLast(eo, true, mparent, index_this);
							}
							for(size_t i = 0; i < mstruct.size(); i++) {
								if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
									mstruct[i].ref();
									multiply_nocopy(&mstruct[i], true);
									calculateMultiplyLast(eo);
								}
							}
						}
						return 1;
					}
					break;
				}
				case STRUCT_POWER: {
					if(mstruct[1].hasNegativeSign()) {
						bool b = false;
						size_t index = 0;
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isPower() && CHILD(i)[1].hasNegativeSign()) {
								if(b) {
									b = false;
									break;
								}
								if(mstruct == CHILD(i)) {
									index = i;
									b = true;
								}
								if(!b) break;
							}
						}
						if(b) {
							if(SIZE == 2) {
								if(index == 0) setToChild(2, true);
								else setToChild(1, true);
							} else {
								ERASE(index);
							}
							calculateAdd(m_one, eo);
							mstruct.ref();
							multiply_nocopy(&mstruct, false);
							calculateMultiplyLast(eo, true, mparent, index_this);
							return 1;
						}
					}
				}
				default: {
					if(SIZE == 2 && CHILD(0).isNumber() && CHILD(1) == mstruct) {
						CHILD(0).number()++;
						MERGE_APPROX_AND_PREC(mstruct)
						calculateMultiplyIndex(0, eo, true, mparent, index_this);						
						return 1;
					}
				}					
			}
			break;
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {return -1;}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {
					return 0;
				}
				default: {
					if(equals(mstruct)) {
						multiply_nocopy(new MathStructure(2, 1), true);
						MERGE_APPROX_AND_PREC(mstruct)
						calculateMultiplyLast(eo, true, mparent, index_this);						
						return 1;
					}
				}
			}	
		}		
	}
	return -1;
}

bool reducable(const MathStructure &mnum, const MathStructure &mden, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {}
		case STRUCT_ADDITION: {
			break;
		}
		default: {
			if(mnum.representsNonZero(true)) {
				bool reduce = true;
				for(size_t i = 0; i < mden.size() && reduce; i++) {
					switch(mden[i].type()) {
						case STRUCT_MULTIPLICATION: {
							reduce = false;
							for(size_t i2 = 0; i2 < mden[i].size(); i2++) {
								if(mnum == mden[i][i2]) {
									reduce = true;
									if(!nr.isOne() && !nr.isFraction()) nr.set(1, 1);
									break;
								} else if(mden[i][i2].isPower() && mden[i][i2][1].isNumber() && mden[i][i2][1].number().isReal() && mnum == mden[i][i2][0]) {
									if(!mden[i][i2][1].number().isPositive()) {
										break;
									}
									if(mden[i][i2][1].number().isLessThan(nr)) nr = mden[i][i2][1].number();
									reduce = true;
									break;
								}
							}
							break;
						}
						case STRUCT_POWER: {
							if(mden[i][1].isNumber() && mden[i][1].number().isReal() && mnum == mden[i][0]) {
								if(!mden[i][1].number().isPositive()) {
									reduce = false;
									break;
								}
								if(mden[i][1].number().isLessThan(nr)) nr = mden[i][1].number();
								break;
							}
						}
						default: {
							if(mnum != mden[i]) {
								reduce = false;
								break;
							}
							if(!nr.isOne() && !nr.isFraction()) nr.set(1, 1);
						}
					}
				}
				return reduce;
			}
		}
	}
	return false;
}
void reduce(const MathStructure &mnum, MathStructure &mden, Number &nr, const EvaluationOptions &eo) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {}
		case STRUCT_ADDITION: {
			break;
		}
		default: {
			for(size_t i = 0; i < mden.size(); i++) {
				switch(mden[i].type()) {
					case STRUCT_MULTIPLICATION: {
						for(size_t i2 = 0; i2 < mden[i].size(); i2++) {
							if(mden[i][i2] == mnum) {
								if(!nr.isOne()) {
									MathStructure *mexp = new MathStructure(1, 1);
									mexp->number() -= nr;
									mden[i][i2].raise_nocopy(mexp);
									mden[i][i2].calculateRaiseExponent(eo, &mden[i], i2);
								} else {
									if(mden[i].size() == 1) {
										mden[i].set(m_one);
									} else {
										mden[i].delChild(i2 + 1);
										if(mden[i].size() == 1) {
											mden[i].setToChild(1, true, &mden, i + 1);
										}
									}
								}
								break;
							} else if(mden[i][i2].isPower() && mden[i][i2][1].isNumber() && mden[i][i2][1].number().isReal() && mnum.equals(mden[i][i2][0])) {
								mden[i][i2][1].number() -= nr;
								if(mden[i][i2][1].number().isOne()) {
									mden[i][i2].setToChild(1, true, &mden[i], i2 + 1);
								} else {
									mden[i][i2].calculateRaiseExponent(eo, &mden[i], i2);
								}
								break;
							}
						}
						break;
					}
					case STRUCT_POWER: {
						if(mden[i][1].isNumber() && mden[i][1].number().isReal() && mnum.equals(mden[i][0])) {
							mden[i][1].number() -= nr;
							if(mden[i][1].number().isOne()) {
								mden[i].setToChild(1, true, &mden, i + 1);
							} else {
								mden[i].calculateRaiseExponent(eo, &mden, i);
							}
							break;
						}
					}
					default: {
						if(!nr.isOne()) {
							MathStructure *mexp = new MathStructure(1, 1);
							mexp->number() -= nr;
							mden[i].raise_nocopy(mexp);
							mden[i].calculateRaiseExponent(eo, &mden, 1);
						} else {
							mden[i].set(m_one);
						}
					}
				}
			}
			mden.calculatesub(eo, eo, false);
		}
	}
}

bool addablePower(const MathStructure &mstruct, const EvaluationOptions &eo) {
	if(mstruct[0].representsNonNegative(true)) return true;
	if(mstruct[1].representsInteger()) return true;
	return eo.allow_complex && mstruct[0].representsNegative(true) && mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().denominatorIsEven();
}

int MathStructure::merge_multiplication(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool reversed, bool do_append) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.multiply(mstruct.number()) && (eo.approximation == APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.isInfinite() || o_number.isInfinite() || mstruct.number().isInfinite())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	if(mstruct.isOne()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	} else if(isOne()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite()) {
		if(o_number.isMinusInfinity()) {
			if(mstruct.representsPositive(false)) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			} else if(mstruct.representsNegative(false)) {
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(o_number.isPlusInfinity()) {
			if(mstruct.representsPositive(false)) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			} else if(mstruct.representsNegative(false)) {
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
		if(mstruct.representsReal(false) && mstruct.representsNonZero(false)) {
			o_number.setInfinity();
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite()) {
		if(mstruct.number().isMinusInfinity()) {
			if(representsPositive(false)) {
				clear(true);
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(representsNegative(false)) {
				clear(true);
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(mstruct.number().isPlusInfinity()) {
			if(representsPositive(false)) {
				clear(true);
				o_number.setPlusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			} else if(representsNegative(false)) {
				clear(true);
				o_number.setMinusInfinity();
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		}
		if(representsReal(false) && representsNonZero(false)) {
			clear(true);
			o_number.setInfinity();
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		}
	}

	if(representsUndefined() || mstruct.representsUndefined()) return -1;
	const MathStructure *mnum = NULL, *mden = NULL;
	bool b_nonzero = false;
	if(eo.reduce_divisions) {
		if(!isNumber() && mstruct.isPower() && mstruct[0].isAddition() && mstruct[0].size() > 1 && mstruct[1].isNumber() && mstruct[1].number().isMinusOne()) {
			if((!isPower() || !CHILD(1).hasNegativeSign()) && representsNumber() && mstruct[0].representsNumber()) {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mstruct[0].representsZero(true)) || mstruct[0].representsNonZero(true)) {
					b_nonzero = true;
				}
				if(b_nonzero || (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mstruct[0].representsZero(true))) {
					mnum = this;
					mden = &mstruct[0];
				}
			}
		} else if(!mstruct.isNumber() && isPower() && CHILD(0).isAddition() && CHILD(0).size() > 1 && CHILD(1).isNumber() && CHILD(1).number().isMinusOne()) {
			if((!mstruct.isPower() || !mstruct[1].hasNegativeSign()) && mstruct.representsNumber() && CHILD(0).representsNumber()) {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true)) || CHILD(0).representsNonZero(true)) {
					b_nonzero = true;
				}
				if(b_nonzero || (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true))) {
					mnum = &mstruct;
					mden = &CHILD(0);
				}
			}
		}
	}
	if(mnum && mden && eo.reduce_divisions) {
		switch(mnum->type()) {
			case STRUCT_ADDITION: {
				break;
			}
			case STRUCT_MULTIPLICATION: {
				Number nr;
				vector<Number> nrs;
				vector<size_t> reducables;
				for(size_t i = 0; i < mnum->size(); i++) {
					switch((*mnum)[i].type()) {
						case STRUCT_ADDITION: {break;}
						case STRUCT_POWER: {
							if((*mnum)[i][1].isNumber() && (*mnum)[i][1].number().isReal()) {
								if((*mnum)[i][1].number().isPositive()) {
									nr.set((*mnum)[i][1].number());
									if(reducable((*mnum)[i][0], *mden, nr)) {
										nrs.push_back(nr);
										reducables.push_back(i);
									}
								}
								break;
							}
						}
						default: {
							nr.set(1, 1);
							if(reducable((*mnum)[i], *mden, nr)) {
								nrs.push_back(nr);
								reducables.push_back(i);
							}
						}
					}
				}
				if(reducables.size() > 0) {
					if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
					if(mnum == this) {
						mstruct.ref();
						transform_nocopy(STRUCT_MULTIPLICATION, &mstruct);
					} else {
						transform(STRUCT_MULTIPLICATION);
						PREPEND_REF(&mstruct);
					}
					size_t i_erased = 0;
					for(size_t i = 0; i < reducables.size(); i++) {
						switch(CHILD(0)[reducables[i] - i_erased].type()) {
							case STRUCT_POWER: {
								if(CHILD(0)[reducables[i] - i_erased][1].isNumber() && CHILD(0)[reducables[i] - i_erased][1].number().isReal()) {
									reduce(CHILD(0)[reducables[i] - i_erased][0], CHILD(1)[0], nrs[i], eo);
									if(nrs[i] == CHILD(0)[reducables[i] - i_erased][1].number()) {
										CHILD(0).delChild(reducables[i] - i_erased + 1);
										i_erased++;
									} else {
										CHILD(0)[reducables[i] - i_erased][1].number() -= nrs[i];
										if(CHILD(0)[reducables[i] - i_erased][1].number().isOne()) {
											CHILD(0)[reducables[i] - i_erased].setToChild(1, true, &CHILD(0), reducables[i] - i_erased + 1);
										} else {
											CHILD(0)[reducables[i] - i_erased].calculateRaiseExponent(eo, &CHILD(0), reducables[i] - i_erased);
										}
										CHILD(0).calculateMultiplyIndex(reducables[i] - i_erased, eo, true, this, 0);
									}
									break;
								}
							}
							default: {
								reduce(CHILD(0)[reducables[i] - i_erased], CHILD(1)[0], nrs[i], eo);
								if(nrs[i].isOne()) {
									CHILD(0).delChild(reducables[i] - i_erased + 1);
									i_erased++;
								} else {
									MathStructure mexp(1, 1);
									mexp.number() -= nrs[i];
									CHILD(0)[reducables[i] - i_erased].calculateRaise(mexp, eo, &CHILD(0), reducables[i] - i_erased);
									CHILD(0).calculateMultiplyIndex(reducables[i] - i_erased, eo, true, this, 0);
								}
							}
						}
					}
					if(CHILD(0).size() == 0) {
						setToChild(2, true, mparent, index_this + 1);
					} else if(CHILD(0).size() == 1) {
						CHILD(0).setToChild(1, true, this, 1);
						calculateMultiplyIndex(0, eo, true, mparent, index_this);
					} else {
						calculateMultiplyIndex(0, eo, true, mparent, index_this);
					}
					return 1;
				}
				break;
			}
			case STRUCT_POWER: {
				if((*mnum)[1].isNumber() && (*mnum)[1].number().isReal()) {
					if((*mnum)[1].number().isPositive()) {
						Number nr((*mnum)[1].number());
						if(reducable((*mnum)[0], *mden, nr)) {
							if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
							if(nr != (*mnum)[1].number()) {
								MathStructure mnum2((*mnum)[0]);
								if(mnum == this) {
									CHILD(1).number() -= nr;
									if(CHILD(1).number().isOne()) {
										set(mnum2);
									} else {
										calculateRaiseExponent(eo);
									}
									mstruct.ref();
									transform_nocopy(STRUCT_MULTIPLICATION, &mstruct);
									calculateMultiplyLast(eo);
								} else {
									transform(STRUCT_MULTIPLICATION);
									PREPEND(mstruct);
									CHILD(0)[1].number() -= nr;
									if(CHILD(0)[1].number().isOne()) {
										CHILD(0) = mnum2;
									} else {
										CHILD(0).calculateRaiseExponent(eo);
									}
									calculateMultiplyIndex(0, eo);
								}
								reduce(mnum2, CHILD(1)[0], nr, eo);
							} else {
								if(mnum == this) {
									MathStructure mnum2((*mnum)[0]);
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
										reduce(mnum2, (*mparent)[index_this][0], nr, eo);
									} else {
										set_nocopy(mstruct, true);
										reduce(mnum2, CHILD(0), nr, eo);
									}
								} else {
									reduce((*mnum)[0], CHILD(0), nr, eo);
								}
							}
							return 1;
						}
					}
					break;
				}
			}
			default: {
				Number nr(1, 1);
				if(reducable(*mnum, *mden, nr)) {
					if(!b_nonzero && eo.warn_about_denominators_assumed_nonzero && !warn_about_denominators_assumed_nonzero(*mden, eo)) break;
					if(mnum == this) {
						MathStructure mnum2(*mnum);
						if(mparent) {
							mparent->swapChildren(index_this + 1, index_mstruct + 1);
							reduce(mnum2, (*mparent)[index_this][0], nr, eo);
							(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
						} else {
							set_nocopy(mstruct, true);
							reduce(mnum2, CHILD(0), nr, eo);
							calculateRaiseExponent(eo, mparent, index_this);
						}						
					} else {
						reduce(*mnum, CHILD(0), nr, eo);
						calculateRaiseExponent(eo, mparent, index_this);
					}
					return 1;
				}
			}
		}
	}

	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(isMatrix() && mstruct.isMatrix()) {
						if(CHILD(0).size() != mstruct.size()) {
							CALCULATOR->error(true, _("The second matrix must have as many rows (was %s) as the first has columns (was %s) for matrix multiplication."), i2s(mstruct.size()).c_str(), i2s(CHILD(0).size()).c_str(), NULL);
							return -1;
						}
						MathStructure msave(*this);
						size_t rows = size();
						clearMatrix(true);
						resizeMatrix(rows, mstruct[0].size(), m_zero);
						MathStructure mtmp;
						for(size_t index_r = 0; index_r < SIZE; index_r++) {
							for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
								for(size_t index = 0; index < msave[0].size(); index++) {
									mtmp = msave[index_r][index];
									mtmp.calculateMultiply(mstruct[index][index_c], eo);
									CHILD(index_r)[index_c].calculateAdd(mtmp, eo, &CHILD(index_r), index_c);
								}			
							}		
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else {
						if(SIZE == mstruct.size()) {
							for(size_t i = 0; i < SIZE; i++) {
								mstruct[i].ref();
								CHILD(i).multiply_nocopy(&mstruct[i], true);
								CHILD(i).calculateMultiplyLast(eo, true, this, i);
							}
							m_type = STRUCT_ADDITION;
							MERGE_APPROX_AND_PREC(mstruct)
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						}
					}
					return -1;
				}
				default: {
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).calculateMultiply(mstruct, eo, this, i);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);
					return 1;
				}
			}
		}
		case STRUCT_ADDITION: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					return 0;
				}
				case STRUCT_ADDITION: {					
					if(eo.expand) {
						MathStructure msave(*this);
						CLEAR;
						for(size_t i = 0; i < mstruct.size(); i++) {
							APPEND(msave);
							mstruct[i].ref();
							LAST.multiply_nocopy(&mstruct[i], true);							
							if(reversed) {
								LAST.swapChildren(1, LAST.size());
								LAST.calculateMultiplyIndex(0, eo, true, this, SIZE - 1);
							} else {
								LAST.calculateMultiplyLast(eo, true, this, SIZE - 1);
							}
						}
						MERGE_APPROX_AND_PREC(mstruct)
						calculatesub(eo, eo, false, mparent, index_this);
						return 1;
					} else {
						if(equals(mstruct)) {
							raise_nocopy(new MathStructure(2, 1));
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					}
					break;
				}
				case STRUCT_POWER: {
					if(mstruct[1].isNumber() && *this == mstruct[0] && addablePower(mstruct, eo)) {
						if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true)) 
						|| (mstruct[1].isNumber() && mstruct[1].number().isReal() && !mstruct[1].number().isMinusOne())
						|| representsNonZero(true) 
						|| mstruct[1].representsPositive()
						|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(*this, mstruct[1], eo))) {
							if(mparent) {
								mparent->swapChildren(index_this + 1, index_mstruct + 1);
								(*mparent)[index_this][1].number()++;
								(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
							} else {
								set_nocopy(mstruct, true);
								CHILD(1).number()++;
								calculateRaiseExponent(eo, mparent, index_this);
							}
							return 1;
						}
					}
					if(!eo.expand && mstruct[0].isAddition()) return -1;
					if(mstruct[1].hasNegativeSign()) {
						int ret;
						vector<bool> merged;
						merged.resize(SIZE, false);
						size_t merges = 0;
						MathStructure *mstruct2 = new MathStructure(mstruct);						
						for(size_t i = 0; i < SIZE; i++) {
							if(CHILD(i).isOne()) ret = -1;
							else ret = CHILD(i).merge_multiplication(*mstruct2, eo, NULL, 0, 0, false, false);
							if(ret == 0) {
								ret = mstruct2->merge_multiplication(CHILD(i), eo, NULL, 0, 0, true, false);
								if(ret >= 1) {
									mstruct2->ref();
									setChild_nocopy(mstruct2, i + 1);
								}
							}
							if(ret >= 1) {
								mstruct2->unref();
								if(i + 1 != SIZE) mstruct2 = new MathStructure(mstruct);
								merged[i] = true;
								merges++;
							} else {
								if(i + 1 == SIZE) mstruct2->unref();
								merged[i] = false;
							}
						}
						if(merges == 0) {
							return -1;
						} else if(merges == SIZE) {
							calculatesub(eo, eo, false, mparent, index_this);
							return 1;
						} else if(merges == SIZE - 1) {
							for(size_t i = 0; i < SIZE; i++) {
								if(!merged[i]) {
									mstruct.ref();
									CHILD(i).multiply_nocopy(&mstruct, true);
									break;
								}
							}
							calculatesub(eo, eo, false, mparent, index_this);
						} else {
							MathStructure *mdiv = new MathStructure();
							merges = 0;
							for(size_t i = 0; i - merges < SIZE; i++) {
								if(!merged[i]) {
									CHILD(i - merges).ref();
									if(merges > 0) {
										(*mdiv)[0].add_nocopy(&CHILD(i - merges), merges > 1);
									} else {
										mdiv->multiply(mstruct);
										mdiv->setChild_nocopy(&CHILD(i - merges), 1);
									}
									ERASE(i - merges);
									merges++;
								}
							}
							add_nocopy(mdiv, true);
							calculatesub(eo, eo, false);
						}
						return 1;
					}
					if(!eo.expand) return -1;
				}
				case STRUCT_MULTIPLICATION: {
					if(!eo.expand && do_append) {
						transform(STRUCT_MULTIPLICATION);
						for(size_t i = 0; i < mstruct.size(); i++) {
							APPEND_REF(&mstruct[i]);
						}
						return 1;
					}
				}
				default: {
					if(!eo.expand) return -1;
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).multiply(mstruct, true);						
						if(reversed) {
							CHILD(i).swapChildren(1, CHILD(i).size());
							CHILD(i).calculateMultiplyIndex(0, eo, true, this, i);
						} else {
							CHILD(i).calculateMultiplyLast(eo, true, this, i);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);
					return 1;
				}
			}
			return -1;
		}
		case STRUCT_MULTIPLICATION: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {
					if(!eo.expand) {
						if(!do_append) return -1;
						APPEND_REF(&mstruct);
						return 1;
					}
					return 0;
				}
				case STRUCT_MULTIPLICATION: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(reversed) {
							PREPEND_REF(&mstruct[i]);
							calculateMultiplyIndex(0, eo, false);
						} else {
							APPEND_REF(&mstruct[i]);
							calculateMultiplyLast(eo, false);
						}
					}
					MERGE_APPROX_AND_PREC(mstruct)
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}					
					return 1;
				}
				case STRUCT_POWER: {
					if(mstruct[1].isNumber()) {
						if(*this == mstruct[0] && addablePower(mstruct, eo)) {
							if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true)) 
							|| (mstruct[1].isNumber() && mstruct[1].number().isReal() && !mstruct[1].number().isMinusOne()) 
							|| representsNonZero(true) 
							|| mstruct[1].representsPositive()
							|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(*this, mstruct[1], eo))) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
									(*mparent)[index_this][1].number()++;
									(*mparent)[index_this].calculateRaiseExponent(eo, mparent, index_this);
								} else {
									set_nocopy(mstruct, true);
									CHILD(1).number()++;
									calculateRaiseExponent(eo, mparent, index_this);
								}							
								return 1;
							}
						} else {
							for(size_t i = 0; i < SIZE; i++) {
								int ret = CHILD(i).merge_multiplication(mstruct, eo, NULL, 0, 0, false, false);
								if(ret == 0) {
									ret = mstruct.merge_multiplication(CHILD(i), eo, NULL, 0, 0, true, false);
									if(ret >= 1) {
										if(ret == 2) ret = 3;
										else if(ret == 3) ret = 2;
										mstruct.ref();
										setChild_nocopy(&mstruct, i + 1);
									}
								}
								if(ret >= 1) {
									if(ret != 2) calculateMultiplyIndex(i, eo, true, mparent, index_this);
									return 1;
								}	
							}
						}
					}
				}
				default: {
					if(do_append) {
						MERGE_APPROX_AND_PREC(mstruct)
						if(reversed) {
							PREPEND_REF(&mstruct);
							calculateMultiplyIndex(0, eo, true, mparent, index_this);
						} else {
							APPEND_REF(&mstruct);
							calculateMultiplyLast(eo, true, mparent, index_this);
						}
						return 1;
					}
				}
			}
			return -1;
		}
		case STRUCT_POWER: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {
					return 0;
				}
				case STRUCT_POWER: {
					if(mstruct[0] == CHILD(0)) {
						bool b = CHILD(0).representsNonNegative(true), b2 = true, b_warn = false;
						if(!b) {
							b = CHILD(1).representsInteger() && mstruct[1].representsInteger();
						}
						if(!b) {
							b = eo.allow_complex && CHILD(0).representsNegative();
							if(b) {
								b = (CHILD(1).representsInteger() 
								|| (CHILD(1).isNumber() && CHILD(1).number().isRational() && CHILD(1).number().denominatorIsEven()));
							}
							if(b) {
								b = (mstruct[1].representsInteger() 
								|| (mstruct[1].isNumber() && mstruct[1].number().isRational() && mstruct[1].number().denominatorIsEven()));
							}
						}
						if(b) {
							b = false;
							bool b2test = false;
							if(IS_REAL(mstruct[1]) && IS_REAL(CHILD(1))) {
								if(mstruct[1].number().isPositive() == CHILD(1).number().isPositive()) {
									b2 = true;
									b = true;
								} else if(!mstruct[1].number().isMinusOne() && !CHILD(1).number().isMinusOne()) {
									b2 = (mstruct[1].number() + CHILD(1).number()).isNegative();
									b = true;
									if(!b2) b2test = true;
								}								
							}
							if(!b || b2test) {
								b = (!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true)) 
								|| CHILD(0).representsNonZero(true) 
								|| (CHILD(1).representsPositive() && mstruct[1].representsPositive()) 
								|| (CHILD(1).representsNegative() && mstruct[1].representsNegative());
								if(!b && eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true)) {
									b = true;
									b_warn = true;
								}
								if(b2test) {
									b2 = b;
									b = true;
								}
							}
						}
						if(b) {
							if(IS_REAL(CHILD(1)) && IS_REAL(mstruct[1])) {
								if(!b2 && !do_append) return -1;
								if(b_warn && !warn_about_denominators_assumed_nonzero(CHILD(0), eo)) return -1;
								if(b2) {
									CHILD(1).number() += mstruct[1].number();
									calculateRaiseExponent(eo, mparent, index_this);
								} else {									
									if(CHILD(1).number().isNegative()) {
										CHILD(1).number()++;
										mstruct[1].number() += CHILD(1).number();
										CHILD(1).number().set(-1, 1);										
									} else {
										mstruct[1].number()++;
										CHILD(1).number() += mstruct[1].number();
										mstruct[1].number().set(-1, 1);										
									}									
									MERGE_APPROX_AND_PREC(mstruct)
									transform(STRUCT_MULTIPLICATION);
									CHILD(0).calculateRaiseExponent(eo, this, 0);
									if(reversed) {
										PREPEND_REF(&mstruct);
										CHILD(0).calculateRaiseExponent(eo, this, 0);
										calculateMultiplyIndex(0, eo, true, mparent, index_this);
									} else {
										APPEND_REF(&mstruct);
										CHILD(1).calculateRaiseExponent(eo, this, 1);
										calculateMultiplyLast(eo, true, mparent, index_this);
									}
								}						
								return 1;
							} else {
								MathStructure mstruct2(CHILD(1));
								mstruct2 += mstruct[1];
								if(mstruct2.calculatesub(eo, eo, false)) {
									if(b_warn && !warn_about_denominators_assumed_nonzero_llgg(CHILD(0), CHILD(1), mstruct[1], eo)) return -1;
									CHILD(1) = mstruct2;
									calculateRaiseExponent(eo, mparent, index_this);
									return 1;
								}
							}
						}
					} else if(mstruct[1] == CHILD(1) && (mstruct[1].representsInteger() || CHILD(0).representsPositive(true) || mstruct[0].representsPositive(true))) {
						MathStructure mstruct2(CHILD(0));
						if(mstruct2.calculateMultiply(mstruct[0], eo)) {
							CHILD(0) = mstruct2;
							MERGE_APPROX_AND_PREC(mstruct)
							calculateRaiseExponent(eo, mparent, index_this);							
							return 1;
						}
					}
					break;
				}
				default: {
					if(!mstruct.isNumber() && CHILD(1).isNumber() && CHILD(0) == mstruct && addablePower(*this, eo)) {
						if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true)) 
						|| (CHILD(1).isNumber() && CHILD(1).number().isReal() && !CHILD(1).number().isMinusOne())
						|| CHILD(0).representsNonZero(true) 
						|| CHILD(1).representsPositive()
						|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !CHILD(0).representsZero(true) && warn_about_denominators_assumed_nonzero_or_positive(CHILD(0), CHILD(1), eo))) {
							CHILD(1).number()++;
							MERGE_APPROX_AND_PREC(mstruct)
							calculateRaiseExponent(eo, mparent, index_this);
							return 1;
						}
					}
					if(mstruct.isZero() && (!eo.keep_zero_units || containsRepresentativeOfType(STRUCT_UNIT, true, true) == 0) && !representsUndefined(true, true, !eo.assume_denominators_nonzero)) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					break;
				}
			}
			return -1;
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {}
				case STRUCT_ADDITION: {}
				case STRUCT_MULTIPLICATION: {}
				case STRUCT_POWER: {
					return 0;
				}
				default: {
					if(mstruct.isZero() && (!eo.keep_zero_units || containsRepresentativeOfType(STRUCT_UNIT, true, true) == 0) && !representsUndefined(true, true, !eo.assume_denominators_nonzero) && representsNonMatrix()) {
						clear(true); 
						MERGE_APPROX_AND_PREC(mstruct)
						return 3;
					}
					if(isZero() && !mstruct.representsUndefined(true, true, !eo.assume_denominators_nonzero) && (!eo.keep_zero_units || !mstruct.containsRepresentativeOfType(STRUCT_UNIT, true, true)) && mstruct.representsNonMatrix()) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 2;
					}
					if(equals(mstruct)) {
						raise_nocopy(new MathStructure(2, 1));
						MERGE_APPROX_AND_PREC(mstruct)
						calculateRaiseExponent(eo, mparent, index_this);						
						return 1;
					}
					break;
				}
			}
			break;
		}			
	}
	return -1;
}

int MathStructure::merge_power(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.raise(mstruct.number(), eo.approximation != APPROXIMATION_APPROXIMATE) && (eo.approximation == APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.isInfinite() || o_number.isInfinite() || mstruct.number().isInfinite())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		if(mstruct.number().isRational()) {
			if(!mstruct.number().numeratorIsOne() && o_number.isPositive()) {
				nr = o_number;
				if(nr.raise(mstruct.number().numerator()) && (eo.approximation == APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.isInfinite() || o_number.isInfinite() || mstruct.number().isInfinite())) {
					o_number = nr;
					numberUpdated();
					nr.set(mstruct.number().denominator());
					nr.recip();
					calculateRaise(nr, eo, mparent, index_this);
					return 1;
				}
			} else if(eo.split_squares && o_number.isInteger() && mstruct.number().denominatorIsTwo()) {
				nr.set(1, 1);
				bool b = true, overflow;
				int val;
				while(b) {
					b = false;
					overflow = false;
					val = o_number.intValue(&overflow);
					if(overflow) {
						cln::cl_I cval = cln::numerator(cln::rational(cln::realpart(o_number.internalNumber())));
						for(size_t i = 0; i < NR_OF_PRIMES; i++) {
							if(cln::zerop(cln::rem(cval, SQUARE_PRIMES[i]))) {
								nr *= PRIMES[i];
								o_number /= SQUARE_PRIMES[i];
								b = true;
								break;
							}
						}
					} else {
						for(size_t i = 0; i < NR_OF_PRIMES; i++) {
							if(SQUARE_PRIMES[i] > val) {
								break;
							} else if(val % SQUARE_PRIMES[i] == 0) {
								nr *= PRIMES[i];
								o_number /= SQUARE_PRIMES[i];
								b = true;
								break;
							}
						}
					}
				}
				if(!nr.isOne()) {
					transform(STRUCT_MULTIPLICATION);
					CHILD(0).calculateRaise(mstruct, eo, this, 0);
					PREPEND(nr);
					calculateMultiplyIndex(0, eo, true, mparent, index_this);
					return 1;
				}
			}
		}
		return -1;
	}
	if(mstruct.isOne()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(m_type == STRUCT_NUMBER && o_number.isInfinite()) {
		if(mstruct.representsNegative(false)) {
			o_number.clear();
			MERGE_APPROX_AND_PREC(mstruct)
			return 1;
		} else if(mstruct.representsNonZero(false) && mstruct.representsNumber(false)) {
			if(o_number.isMinusInfinity()) {
				if(mstruct.representsEven(false)) {
					o_number.setPlusInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.representsOdd(false)) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				} else if(mstruct.representsInteger(false)) {
					o_number.setInfinity();
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
		}
	} else if(mstruct.isNumber() && mstruct.number().isInfinite()) {
		if(mstruct.number().isInfinity()) {
		} else if(mstruct.number().isMinusInfinity()) {
			if(representsReal(false) && representsNonZero(false)) {
				clear(true);
				MERGE_APPROX_AND_PREC(mstruct)
				return 1;
			}
		} else if(mstruct.number().isPlusInfinity()) {
			if(representsPositive(false)) {
				if(mparent) {
					mparent->swapChildren(index_this + 1, index_mstruct + 1);
				} else {
					set_nocopy(mstruct, true);
				}
				return 1;
			}
		}
	}
	if(representsUndefined() || mstruct.representsUndefined()) return -1;
	if(isZero() && mstruct.representsPositive()) {
		return 1;
	}
	if(mstruct.isZero() && !representsUndefined(true, true)) {
		set(m_one);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			if(mstruct.isNumber() && mstruct.number().isInteger()) {
				if(isMatrix()) {
					if(matrixIsSquare()) {
						Number nr(mstruct.number());
						bool b_neg = false;
						if(nr.isNegative()) {
							nr.setNegative(false);
							b_neg = true;
						}
						if(!nr.isOne()) {
							MathStructure msave(*this);
							nr--;
							while(nr.isPositive()) {
								calculateMultiply(msave, eo);
								nr--;
							}
						}
						if(b_neg) {
							invertMatrix(eo);
						}
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					}
					return -1;
				} else {
					if(mstruct.number().isMinusOne()) {
						return -1;
					}
					Number nr(mstruct.number());
					if(nr.isNegative()) {
						nr++;
					} else {
						nr--;
					}
					MathStructure msave(*this);
					calculateMultiply(msave, eo);
					calculateRaise(nr, eo);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				
			}
			goto default_power_merge;
		}
		case STRUCT_ADDITION: {
			if(mstruct.isNumber() && mstruct.number().isInteger()) {
				if(mstruct.number().isMinusOne()) {
					return -1;
					bool bnum = false;
					for(size_t i = 0; !bnum && i < SIZE; i++) {
						switch(CHILD(i).type()) {
							case STRUCT_NUMBER: {
								if(!CHILD(i).number().isZero() && CHILD(i).number().isReal()) {
									bnum = true;
								}
								break;
							}
							case STRUCT_MULTIPLICATION: {
								if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
									if(!CHILD(i)[0].number().isZero() && CHILD(i)[0].number().isReal()) {
										bnum = true;
									}
									break;
								}
							}
							default: {}
						}
					}
					if(bnum) {
						Number nr;
						size_t negs = 0;
						for(size_t i = 0; i < SIZE; i++) {
							switch(CHILD(i).type()) {
								case STRUCT_NUMBER: {
									if(!CHILD(i).number().isZero() && CHILD(i).number().isReal()) {
										if(CHILD(i).number().isNegative()) negs++;
										if(nr.isZero()) {
											nr = CHILD(i).number();
										} else {
											if(negs) {
												if(CHILD(i).number().isNegative()) {
													nr.setNegative(true);
													if(CHILD(i).number().isGreaterThan(nr)) {
														nr = CHILD(i).number();
													}
													break;
												} else {
													nr.setNegative(false);
												}
											}
											if(CHILD(i).number().isLessThan(nr)) {
												nr = CHILD(i).number();
											}
										}
									}
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(CHILD(i).size() > 0 && CHILD(i)[0].isNumber()) {
										if(!CHILD(i)[0].number().isZero() && CHILD(i)[0].number().isReal()) {
											if(CHILD(i)[0].number().isNegative()) negs++;
											if(nr.isZero()) {
												nr = CHILD(i)[0].number();
											} else {
												if(negs) {
													if(CHILD(i)[0].number().isNegative()) {
														nr.setNegative(true);
														if(CHILD(i)[0].number().isGreaterThan(nr)) {
															nr = CHILD(i)[0].number();
														}
														break;
													} else {
														nr.setNegative(false);
													}
												}
												if(CHILD(i)[0].number().isLessThan(nr)) {
													nr = CHILD(i)[0].number();
												}
											}
										}
										break;
									}
								}
								default: {
									if(nr.isZero() || !nr.isFraction()) {
										nr.set(1, 1);
									}
								}
							}
							
						}
						nr.setNegative(negs > SIZE - negs);
						if(bnum && !nr.isOne() && !nr.isZero()) {
							nr.recip();
							for(size_t i = 0; i < SIZE; i++) {
								switch(CHILD(i).type()) {
									case STRUCT_NUMBER: {
										if(IS_REAL(CHILD(i))) CHILD(i).number() *= nr;
										else CHILD(i).calculateMultiply(nr, eo);
										break;
									}
									case STRUCT_MULTIPLICATION: {
										if(IS_REAL(CHILD(i)[0])) {
											CHILD(i)[0].number() *= nr;
											CHILD(i).calculateMultiplyIndex(0, eo, true, this, i);
										} else {
											CHILD(i).calculateMultiply(nr, eo, this, i);
										}
										break;
									}
									default: {
										CHILD(i).calculateMultiply(nr, eo);
									}
								}
							}
							calculatesub(eo, eo, false);
							mstruct.ref();
							raise_nocopy(&mstruct);
							calculateRaiseExponent(eo);
							calculateMultiply(nr, eo, mparent, index_this);
							return 1;
						}
					}
				} else if(eo.expand && !mstruct.number().isZero()) {
					bool neg = mstruct.number().isNegative();
					Number m(mstruct.number());
					m.setNegative(false);
					if(!representsNonMatrix()) {
						MathStructure mthis(*this);
						while(!m.isOne()) {
							calculateMultiply(mthis, eo);
							m--;
						}
					} else {
						MathStructure mstruct1(CHILD(0));
						MathStructure mstruct2(CHILD(1));
						for(size_t i = 2; i < SIZE; i++) {
							mstruct2.add(CHILD(i), true);
						}					
						Number k(1);
						Number p1(m);
						Number p2(1);
						p1--;
						Number bn;
						CLEAR
						APPEND(mstruct1);
						CHILD(0).calculateRaise(m, eo);
						while(k.isLessThan(m)) {
							bn.binomial(m, k);
							APPEND_NEW(bn);
							LAST.multiply(mstruct1);
							if(!p1.isOne()) {
								LAST[1].raise_nocopy(new MathStructure(p1));
								LAST[1].calculateRaiseExponent(eo);
							}
							LAST.multiply(mstruct2, true);
							if(!p2.isOne()) {
								LAST[2].raise_nocopy(new MathStructure(p2));
								LAST[2].calculateRaiseExponent(eo);
							}
							LAST.calculatesub(eo, eo, false);
							k++;
							p1--;
							p2++;
						}
						APPEND(mstruct2);
						LAST.calculateRaise(m, eo);
						calculatesub(eo, eo, false);
					}
					if(neg) calculateInverse(eo);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			goto default_power_merge;
		}
		case STRUCT_MULTIPLICATION: {
			if(mstruct.representsInteger()) {
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculateRaise(mstruct, eo, this, i);
				}
				MERGE_APPROX_AND_PREC(mstruct)
				calculatesub(eo, eo, false, mparent, index_this);				
				return 1;
			} else {
				bool b = true;
				for(size_t i = 0; i < SIZE; i++) {
					if(!CHILD(i).representsNonNegative(true)) {
						b = false;
						break;
					}
				}
				if(b) {
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).calculateRaise(mstruct, eo, this, i);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					calculatesub(eo, eo, false, mparent, index_this);					
					return 1;
				}
			}
			goto default_power_merge;
		}
		case STRUCT_POWER: {
			if(mstruct.representsInteger() || (mstruct.isNumber() && mstruct.number().isRational() && !mstruct.number().denominatorIsEven()) || representsNonNegative(true)) {
				if(CHILD(1).isNumber() && CHILD(1).number().isRational() && (CHILD(1).isInteger() || CHILD(1).number().denominatorIsEven() || (!CHILD(1).number().numeratorIsEven() && mstruct.representsOdd()) || CHILD(0).representsNonNegative(true)) && (eo.allow_complex || !CHILD(1).number().denominatorIsEven() || CHILD(0).representsPositive(true) || CHILD(0).representsComplex())) {
					if(CHILD(1).number().numeratorIsEven() && !mstruct.representsInteger()) {
						if(CHILD(0).representsNegative(true)) {
							CHILD(0).calculateNegate(eo);				
						} else if(!CHILD(0).representsNonNegative(true)) {
							MathStructure mstruct_base(CHILD(0));
							CHILD(0).set(CALCULATOR->f_abs, &mstruct_base, NULL);							
						}
					}
					mstruct.ref();
					MERGE_APPROX_AND_PREC(mstruct)
					CHILD(1).multiply_nocopy(&mstruct, true);
					CHILD(1).calculateMultiplyLast(eo, true, this, 1);
					calculateRaiseExponent(eo, mparent, index_this);					
					return 1;
				} else if(CHILD(0).representsNonNegative(true) || CHILD(1).representsInteger()) {
					mstruct.ref();
					MERGE_APPROX_AND_PREC(mstruct)
					CHILD(1).multiply_nocopy(&mstruct, true);
					CHILD(1).calculateMultiplyLast(eo, true, this, 1);
					calculateRaiseExponent(eo, mparent, index_this);					
					return 1;
				}
			}
			
			goto default_power_merge;
		}
		case STRUCT_VARIABLE: {
			if(o_variable == CALCULATOR->v_e) {
				if(mstruct.isMultiplication() && mstruct.size() == 2 && mstruct[1].isVariable() && mstruct[1].variable() == CALCULATOR->v_pi) {
					if(mstruct[0].isNumber()) {
						if(mstruct[0].number().isI() || mstruct[0].number().isMinusI()) {
							set(m_minus_one, true);
							return 1;
						} else if(mstruct[0].number().isComplex() && !mstruct[0].number().hasRealPart()) {
							Number img(mstruct[0].number().imaginaryPart());
							if(img.isInteger()) {
								if(img.isEven()) {
									set(m_one, true);
								} else {
									set(m_minus_one, true);
								}
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if(img == Number(1, 2)) {
								clear(true);
								img.set(1, 1);
								o_number.setImaginaryPart(img);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if(img == Number(-1, 2)) {
								clear(true);
								img.set(-1, 1);
								o_number.setImaginaryPart(img);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
						}
					}
				} else if(mstruct.isFunction() && mstruct.function() == CALCULATOR->f_ln && mstruct.size() == 1) {
					if(mstruct[0].representsPositive() || mstruct[0].representsComplex()) {
						set_nocopy(mstruct[0], true);
						return 1;
					}
				}
			}
			goto default_power_merge;
		}
		default: {
			default_power_merge:
			if(mstruct.isAddition()) {
				bool b = representsNonNegative(true);
				if(!b) {
					b = true;
					bool bneg = representsNegative(true);
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(!mstruct[i].representsInteger() && (!bneg || !eo.allow_complex || !mstruct[i].isNumber() || !mstruct[i].number().isRational() || !mstruct[i].number().denominatorIsEven())) {
							b = false;
							break;
						}
					}
				}
				if(b) {
					MathStructure msave(*this);
					clear(true);
					m_type = STRUCT_MULTIPLICATION;
					MERGE_APPROX_AND_PREC(mstruct)
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND(msave);
						mstruct[i].ref();
						LAST.raise_nocopy(&mstruct[i]);
						LAST.calculateRaiseExponent(eo, this, SIZE - 1);
						calculateMultiplyLast(eo, false);
					}					
					if(SIZE == 1) {
						setToChild(1, false, mparent, index_this + 1);
					} else if(SIZE == 0) {
						clear(true);
					} else {
						evalSort();
					}				
					return 1;
				}
			} else if(mstruct.isMultiplication() && mstruct.size() > 1) {
				bool b = representsNonNegative(true);
				if(!b) {
					b = true;
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(!mstruct[i].representsInteger()) {
							b = false;
							break;
						}
					}
				}
				if(b) {
					MathStructure mthis(*this);
					for(size_t i = 0; i < mstruct.size(); i++) {
						if(isZero() && !mstruct[i].representsPositive(true)) continue;
						if(i == 0) mthis.raise(mstruct[i]);
						else mthis[1] = mstruct[i];
						if(mthis.calculateRaiseExponent(eo)) {
							set(mthis);
							if(mstruct.size() == 2) {
								if(i == 0) {
									mstruct[1].ref();
									raise_nocopy(&mstruct[1]);
								} else {
									mstruct[0].ref();
									raise_nocopy(&mstruct[0]);
								}
							} else {
								mstruct.ref();
								raise_nocopy(&mstruct);
								CHILD(1).delChild(i + 1);
							}
							calculateRaiseExponent(eo);
							MERGE_APPROX_AND_PREC(mstruct)
							return 1;
						}
					}
				}
			} else if(mstruct.isNumber() && mstruct.number().isRational() && !mstruct.number().isInteger() && !mstruct.number().numeratorIsOne() && !mstruct.number().numeratorIsMinusOne()) {
				if(representsNonNegative(true)) {
					if(isFunction() && function() == CALCULATOR->f_abs && size() == 1 && mstruct.number().numeratorIsEven()) {
						setToChild(1);
					}
					bool b;
					if(mstruct.number().isNegative()) {
						b = calculateRaise(-mstruct.number().numerator(), eo);
						if(!b) {
							setToChild(1);
							break;
						}
						raise(m_minus_one);
						CHILD(1).number() /= mstruct.number().denominator();
					} else {
						b = calculateRaise(mstruct.number().numerator(), eo);
						if(!b) {
							setToChild(1);
							break;
						}
						raise(m_one);
						CHILD(1).number() /= mstruct.number().denominator();
					}
					if(b) calculateRaiseExponent(eo);
					return 1;
				} else if(mstruct.number().denominatorIsEven() || !mstruct.number().numeratorIsEven()) {
					raise(m_one);
					CHILD(1).number() /= mstruct.number().denominator();
					if(calculateRaiseExponent(eo)) {
						calculateRaise(mstruct.number().numerator(), eo);
					} else {
						setToChild(1);
						break;
						raise(mstruct.number().numerator());
					}
					return 1;
				}				
			}
			break;
		}
	}
	return -1;
}

int MathStructure::merge_logical_and(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {

	if(equals(mstruct)) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(mstruct.representsPositive()) {
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(mstruct.representsNonPositive()) {
		if(isZero()) return 2;
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 3;
	}
	if(representsPositive()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(representsNonPositive()) {
		if(!isZero()) clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(isLogicalOr()) {
		MERGE_APPROX_AND_PREC(mstruct)
		if(mstruct.isLogicalOr()) {
			for(size_t i = 0; i < SIZE; ) {
				MathStructure msave(CHILD(i));
				for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
					if(i2 > 0) {
						insertChild(msave, i + 1);
					}
					CHILD(i).calculateLogicalAnd(mstruct[i2], eo, this, i);
					i++;
				}
			}
		} else {
			for(size_t i = 0; i < SIZE; i++) {
				CHILD(i).calculateLogicalAnd(mstruct, eo, this, i);
			}
		}
		calculatesub(eo, eo, false);
		return 1;
	} else if(mstruct.isLogicalOr()) {
		MathStructure mthis(*this);
		MERGE_APPROX_AND_PREC(mstruct)
		for(size_t i = 0; i < mstruct.size(); i++) {
			mstruct[i].ref();
			if(i == 0) {				
				add_nocopy(&mstruct[i], OPERATION_LOGICAL_AND, true);
				calculateLogicalAndLast(eo, true);
			} else {
				add(mthis, OPERATION_LOGICAL_OR, true);
				LAST.add_nocopy(&mstruct[i], OPERATION_LOGICAL_AND, true);
				LAST.calculateLogicalAndLast(eo, true, this, SIZE - 1);
			}
		}
		calculatesub(eo, eo, false);
		return 1;
	} else if(isComparison() && mstruct.isComparison()) {
		if(CHILD(0) == mstruct[0]) {
			ComparisonResult cr = mstruct[1].compare(CHILD(1));
			ComparisonType ct1 = ct_comp, ct2 = mstruct.comparisonType();
			switch(cr) {
				case COMPARISON_RESULT_UNKNOWN: {
					return -1;
				}
				case COMPARISON_RESULT_NOT_EQUAL: {
					if(ct_comp == COMPARISON_EQUALS && ct2 == COMPARISON_EQUALS) {
						clear(true);
						MERGE_APPROX_AND_PREC(mstruct)
						return 1;
					} else if(ct_comp == COMPARISON_EQUALS && ct2 == COMPARISON_NOT_EQUALS) {
						MERGE_APPROX_AND_PREC(mstruct)
						return 2;
					} else if(ct_comp == COMPARISON_NOT_EQUALS && ct2 == COMPARISON_EQUALS) {
						if(mparent) {
							mparent->swapChildren(index_this + 1, index_mstruct + 1);
						} else {
							set_nocopy(mstruct, true);
						}
						return 3;
					}
					return -1;
				}
				case COMPARISON_RESULT_EQUAL: {
					MERGE_APPROX_AND_PREC(mstruct)
					if(ct_comp == ct2) return 1;
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS) {
							ct_comp = COMPARISON_LESS;
							if(ct2 == COMPARISON_LESS) return 3;
							return 1;
						} else if(ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS_GREATER) {							
							ct_comp = COMPARISON_GREATER;
							if(ct2 == COMPARISON_GREATER) return 3;
							return 1;
						}
					} else if(ct2 == COMPARISON_NOT_EQUALS) {
						if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
							if(ct_comp == COMPARISON_LESS) return 2;
							ct_comp = COMPARISON_LESS;
							return 1;
						} else if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {
							if(ct_comp == COMPARISON_GREATER) return 2;
							ct_comp = COMPARISON_GREATER;
							return 1;
						}
					} else if((ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_EQUALS) && (ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS)) {
						if(ct_comp == COMPARISON_EQUALS) return 2;
						ct_comp = COMPARISON_EQUALS;
						if(ct2 == COMPARISON_EQUALS) return 3;
						return 1;
					} else if((ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) && (ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS)) {
						if(ct_comp == COMPARISON_LESS) return 2;
						ct_comp = COMPARISON_LESS;
						if(ct2 == COMPARISON_LESS) return 3;
						return 1;
					} else if((ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) && (ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS_GREATER)) {
						if(ct_comp == COMPARISON_GREATER) return 2;
						ct_comp = COMPARISON_GREATER;
						if(ct2 == COMPARISON_GREATER) return 3;
						return 1;
					}
					clear(true);
					return 1;
				}
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_EQUAL_OR_LESS: {
					switch(ct1) {
						case COMPARISON_LESS: {
							if(ct2 == COMPARISON_GREATER || ct2 == COMPARISON_EQUALS) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							} else if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_NOT_EQUALS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_GREATER: {
							if(ct2 == COMPARISON_GREATER) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						} 
						case COMPARISON_EQUALS_LESS: {
							if(ct2 == COMPARISON_EQUALS_LESS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						} 
						case COMPARISON_EQUALS_GREATER: {
							if(ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								clear(true);
								MERGE_APPROX_AND_PREC(mstruct)
								return 1;
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
					} 
					break;
				}
				case COMPARISON_RESULT_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_LESS: {
					switch(ct1) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) clear(true); return 1;}
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}								
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) clear(true); return 1;}
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {									 
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
					}
					break;
				}				
			}
		}
	} else if(isLogicalAnd()) {
		if(mstruct.isLogicalAnd()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				APPEND_REF(&mstruct[i]);
			}
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		} else {
			APPEND_REF(&mstruct);
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);			
		}
		return 1;
	} else if(mstruct.isLogicalAnd()) {
		transform(STRUCT_LOGICAL_AND);
		for(size_t i = 0; i < mstruct.size(); i++) {
			APPEND_REF(&mstruct[i]);
		}
		MERGE_APPROX_AND_PREC(mstruct)
		calculatesub(eo, eo, false);
		return 1;
	}
	return -1;

}

int MathStructure::merge_logical_or(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this, size_t index_mstruct, bool) {

	if(mstruct.representsPositive()) {
		if(isOne()) {
			MERGE_APPROX_AND_PREC(mstruct)
			return 2;
		}
		set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 3;
	}
	if(mstruct.representsNonPositive()) {
		if(representsNonPositive() && !isZero()) clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;			
	}
	if(representsPositive()) {
		if(!isOne()) set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 2;
	}
	if(representsNonPositive()) {
		if(mparent) {
			mparent->swapChildren(index_this + 1, index_mstruct + 1);
		} else {
			set_nocopy(mstruct, true);
		}
		return 3;
	}
	if(equals(mstruct)) {
		return 2;
	}

	if(isLogicalAnd()) {
		if(mstruct.isLogicalAnd()) {
			if(SIZE < mstruct.size()) {
				bool b = true;
				for(size_t i = 0; i < SIZE; i++) {
					b = false;
					for(size_t i2 = 0; i2 < mstruct.size(); i2++) {
						if(CHILD(i) == mstruct[i2]) {
							b = true;
							break;
						}
					}
					if(!b) break;
				}
				if(b) {
					MERGE_APPROX_AND_PREC(mstruct)
					return 2;
				}
			} else if(SIZE > mstruct.size()) {
				bool b = true;
				for(size_t i = 0; i < mstruct.size(); i++) {
					b = false;
					for(size_t i2 = 0; i2 < SIZE; i2++) {
						if(mstruct[i] == CHILD(i2)) {
							b = true;
							break;
						}
					}
					if(!b) break;
				}
				if(b) {
					if(mparent) {
						mparent->swapChildren(index_this + 1, index_mstruct + 1);
					} else {
						set_nocopy(mstruct, true);
					}
					return 3;
				}
			}
		} else {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i) == mstruct) {
					if(mparent) {
						mparent->swapChildren(index_this + 1, index_mstruct + 1);
					} else {
						set_nocopy(mstruct, true);
					}
					return 3;
				}
			}
		}
	} else if(mstruct.isLogicalAnd()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(equals(mstruct[i])) {
				MERGE_APPROX_AND_PREC(mstruct)
				return 2;
			}
		}
	}

	if(isComparison() && mstruct.isComparison()) {
		if(CHILD(0) == mstruct[0]) {
			ComparisonResult cr = mstruct[1].compare(CHILD(1));
			ComparisonType ct1 = ct_comp, ct2 = mstruct.comparisonType();
			switch(cr) {
				case COMPARISON_RESULT_UNKNOWN: {
					return -1;
				}
				case COMPARISON_RESULT_NOT_EQUAL: {
					return -1;
				}
				case COMPARISON_RESULT_EQUAL: {
					if(ct_comp == ct2) return 1;
					switch(ct_comp) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_EQUALS_GREATER: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_LESS: {ct_comp = COMPARISON_EQUALS_LESS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_EQUALS_GREATER; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_EQUALS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}								
								case COMPARISON_LESS: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_GREATER: {}
								case COMPARISON_EQUALS_GREATER: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_LESS: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_EQUALS_GREATER: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {ct_comp = COMPARISON_EQUALS_LESS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_NOT_EQUALS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_LESS: {}
								case COMPARISON_EQUALS_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								default: {}
							}
							break;
						}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {ct_comp = ct2; MERGE_APPROX_AND_PREC(mstruct) return 3;}
								case COMPARISON_EQUALS_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {ct_comp = COMPARISON_EQUALS_GREATER; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_LESS: {ct_comp = COMPARISON_NOT_EQUALS; MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
					}
					break;
				}
				case COMPARISON_RESULT_EQUAL_OR_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_EQUAL_OR_LESS: {
					switch(ct1) {
						case COMPARISON_LESS: {
							if(ct2 == COMPARISON_LESS || ct2 == COMPARISON_EQUALS_LESS || ct2 == COMPARISON_NOT_EQUALS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						}
						case COMPARISON_GREATER: {
							if(ct2 == COMPARISON_GREATER) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						} 
						case COMPARISON_EQUALS_LESS: {
							if(ct2 == COMPARISON_EQUALS_LESS) {
								if(mparent) {
									mparent->swapChildren(index_this + 1, index_mstruct + 1);
								} else {
									set_nocopy(mstruct, true);
								}
								return 3;
							}
							break;
						} 
						case COMPARISON_EQUALS_GREATER: {
							if(ct2 == COMPARISON_EQUALS_GREATER || ct2 == COMPARISON_EQUALS) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							if(ct2 == COMPARISON_GREATER) {
								MERGE_APPROX_AND_PREC(mstruct)
								return 2;
							}
							break;
						}
						default: {}
					} 
					break;
				}
				case COMPARISON_RESULT_GREATER: {
					switch(ct1) {
						case COMPARISON_GREATER: {ct1 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct1 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct1 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct1 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
					switch(ct2) {
						case COMPARISON_GREATER: {ct2 = COMPARISON_LESS; break;}
						case COMPARISON_EQUALS_GREATER: {ct2 = COMPARISON_EQUALS_LESS; break;}
						case COMPARISON_LESS: {ct2 = COMPARISON_GREATER; break;}
						case COMPARISON_EQUALS_LESS: {ct2 = COMPARISON_EQUALS_GREATER; break;}
						default: {}
					}
				}
				case COMPARISON_RESULT_LESS: {
					switch(ct1) {
						case COMPARISON_EQUALS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_NOT_EQUALS: {
							switch(ct2) {
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_LESS: {}
						case COMPARISON_LESS: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {
									if(mparent) {
										mparent->swapChildren(index_this + 1, index_mstruct + 1);
									} else {
										set_nocopy(mstruct, true);
									}
									return 3;
								}
								default: {}
							}
							break;
						}
						case COMPARISON_EQUALS_GREATER: {}
						case COMPARISON_GREATER: {
							switch(ct2) {
								case COMPARISON_NOT_EQUALS: {}
								case COMPARISON_EQUALS_LESS: {}
								case COMPARISON_LESS: {set(1, 1, 0, true); MERGE_APPROX_AND_PREC(mstruct) return 1;}
								case COMPARISON_EQUALS: {}
								case COMPARISON_EQUALS_GREATER: {}
								case COMPARISON_GREATER: {MERGE_APPROX_AND_PREC(mstruct) return 2;}
							}
							break;
						}
					}
					break;
				}				
			}
		}
	} else if(isLogicalOr()) {
		if(mstruct.isLogicalOr()) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				APPEND_REF(&mstruct[i]);
			}
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);
		} else {
			APPEND_REF(&mstruct);
			MERGE_APPROX_AND_PREC(mstruct)
			calculatesub(eo, eo, false);			
		}
		return 1;
	} else if(mstruct.isLogicalOr()) {
		transform(STRUCT_LOGICAL_OR);
		for(size_t i = 0; i < mstruct.size(); i++) {
			APPEND_REF(&mstruct[i]);
		}
		MERGE_APPROX_AND_PREC(mstruct)
		calculatesub(eo, eo, false);
		return 1;
	}
	return -1;

}

int MathStructure::merge_logical_xor(MathStructure &mstruct, const EvaluationOptions&, MathStructure*, size_t, size_t, bool) {

	if(equals(mstruct)) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	bool bp1 = representsPositive();
	bool bp2 = mstruct.representsPositive();
	if(bp1 && bp2) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	bool bn1 = representsNonPositive();	
	bool bn2 = mstruct.representsNonPositive();	
	if(bn1 && bn2) {
		clear(true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	if((bn1 && bp2) || (bp1 && bn2)) {
		set(1, 1, 0, true);
		MERGE_APPROX_AND_PREC(mstruct)
		return 1;
	}
	
	return -1;

}


int MathStructure::merge_bitwise_and(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitAnd(mstruct.number()) && (eo.approximation == APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.isInfinite() || o_number.isInfinite() || mstruct.number().isInfinite())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_AND);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_BITWISE_AND: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					return -1;
				}
				case STRUCT_BITWISE_AND: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND_REF(&mstruct[i]);
					}
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					APPEND_REF(&mstruct);
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			break;
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_BITWISE_AND: {
					return 0;
				}
				default: {}
			}	
		}
	}
	return -1;
}
int MathStructure::merge_bitwise_or(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitOr(mstruct.number()) && (eo.approximation == APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.isInfinite() || o_number.isInfinite() || mstruct.number().isInfinite())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_OR);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		case STRUCT_BITWISE_OR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					return -1;
				}
				case STRUCT_BITWISE_OR: {
					for(size_t i = 0; i < mstruct.size(); i++) {
						APPEND_REF(&mstruct[i]);
					}
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					APPEND_REF(&mstruct);
					calculatesub(eo, eo, false);
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
			}
			break;
		}
		default: {
			switch(mstruct.type()) {
				case STRUCT_BITWISE_OR: {
					return 0;
				}
				default: {}
			}	
		}
	}
	return -1;
}
int MathStructure::merge_bitwise_xor(MathStructure &mstruct, const EvaluationOptions &eo, MathStructure*, size_t, size_t, bool) {
	if(mstruct.type() == STRUCT_NUMBER && m_type == STRUCT_NUMBER) {
		Number nr(o_number);
		if(nr.bitXor(mstruct.number()) && (eo.approximation == APPROXIMATION_APPROXIMATE || !nr.isApproximate() || o_number.isApproximate() || mstruct.number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || o_number.isComplex() || mstruct.number().isComplex()) && (eo.allow_infinite || !nr.isInfinite() || o_number.isInfinite() || mstruct.number().isInfinite())) {
			if(o_number == nr) {
				o_number = nr;
				numberUpdated();
				return 2;
			}
			o_number = nr;
			numberUpdated();
			return 1;
		}
		return -1;
	}
	switch(m_type) {
		case STRUCT_VECTOR: {
			switch(mstruct.type()) {
				case STRUCT_VECTOR: {
					if(SIZE < mstruct.size()) return 0;
					for(size_t i = 0; i < mstruct.size(); i++) {
						mstruct[i].ref();
						CHILD(i).add_nocopy(&mstruct[i], OPERATION_LOGICAL_XOR);
						CHILD(i).calculatesub(eo, eo, false);
					}
					MERGE_APPROX_AND_PREC(mstruct)
					return 1;
				}
				default: {
					return -1;
				}
			}
			return -1;
		}
		default: {}
	}
	return -1;
}

#define MERGE_RECURSE			if(recursive) {\
						for(size_t i = 0; i < SIZE; i++) {\
							CHILD(i).calculatesub(eo, feo, true, this, i);\
						}\
						CHILDREN_UPDATED;\
					}

#define MERGE_ALL(FUNC, TRY_LABEL) 	size_t i2, i3 = SIZE;\
					for(size_t i = 0; i < SIZE - 1; i++) {\
						i2 = i + 1;\
						TRY_LABEL:\
						for(; i2 < i; i2++) {\
							int r = CHILD(i2).FUNC(CHILD(i), eo, this, i2, i);\
							if(r == 0) {\
								SWAP_CHILDREN(i2, i);\
								r = CHILD(i2).FUNC(CHILD(i), eo, this, i2, i, true);\
								if(r < 1) {\
									SWAP_CHILDREN(i2, i);\
								}\
							}\
							if(r >= 1) {\
								ERASE(i);\
								b = true;\
								i3 = i;\
								i = i2;\
								i2 = 0;\
								goto TRY_LABEL;\
							}\
						}\
						for(i2 = i + 1; i2 < SIZE; i2++) {\
							int r = CHILD(i).FUNC(CHILD(i2), eo, this, i, i2);\
							if(r == 0) {\
								SWAP_CHILDREN(i, i2);\
								r = CHILD(i).FUNC(CHILD(i2), eo, this, i, i2, true);\
								if(r < 1) {\
									SWAP_CHILDREN(i, i2);\
								} else if(r == 2) {\
									r = 3;\
								} else if(r == 3) {\
									r = 2;\
								}\
							}\
							if(r >= 1) {\
								ERASE(i2);\
								b = true;\
								if(r != 2) {\
									i2 = 0;\
									goto TRY_LABEL;\
								}\
								i2--;\
							}\
						}\
						if(i3 < SIZE) {\
							if(i3 == SIZE - 1) break;\
							i = i3;\
							i3 = SIZE;\
							i2 = i + 1;\
							goto TRY_LABEL;\
						}\
					}
					
#define MERGE_ALL2			if(SIZE == 1) {\
						setToChild(1, false, mparent, index_this + 1);\
					} else if(SIZE == 0) {\
						clear(true);\
					} else {\
						evalSort();\
					}

bool MathStructure::calculatesub(const EvaluationOptions &eo, const EvaluationOptions &feo, bool recursive, MathStructure *mparent, size_t index_this) {	
	if(b_protected) return false;
	bool b = false;
	switch(m_type) {
		case STRUCT_VARIABLE: {
			if(eo.calculate_variables && o_variable->isKnown()) {
				if(eo.approximation == APPROXIMATION_APPROXIMATE || !o_variable->isApproximate()) {
					set(((KnownVariable*) o_variable)->get());
					if(eo.calculate_functions) {
						calculateFunctions(feo);
						unformat(feo);
					}
					b = true;
					calculatesub(eo, feo, true, mparent, index_this);					
				}
			}
			break;
		}
		case STRUCT_POWER: {			
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILD(1).calculatesub(eo, feo, true, this, 1);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).merge_power(CHILD(1), eo) >= 1) {
				b = true;
				setToChild(1, false, mparent, index_this + 1);				
			}
			break;
		}
		case STRUCT_ADDITION: {
			MERGE_RECURSE
			MERGE_ALL(merge_addition, try_add)
			MERGE_ALL2
			break;
		}
		case STRUCT_MULTIPLICATION: {
			MERGE_RECURSE
			if(representsNonMatrix()) {
				MERGE_ALL(merge_multiplication, try_multiply)
			} else {
				size_t i2, i3 = SIZE;
				for(size_t i = 0; i < SIZE - 1; i++) {
					i2 = i + 1;
					try_multiply_matrix:
					bool b_matrix = !CHILD(i).representsNonMatrix();
					if(i2 < i) {
						for(; ; i2--) {
							int r = CHILD(i2).merge_multiplication(CHILD(i), eo, this, i2, i);
							if(r == 0) {
								SWAP_CHILDREN(i2, i);
								r = CHILD(i2).merge_multiplication(CHILD(i), eo, this, i2, i, true);
								if(r < 1) {
									SWAP_CHILDREN(i2, i);
								}
							}
							if(r >= 1) {
								ERASE(i);
								b = true;
								i3 = i;
								i = i2;
								i2 = 0;
								goto try_multiply_matrix;
							}
							if(i2 == 0) break;
							if(b_matrix && !CHILD(i2).representsNonMatrix()) break;
						}
					}
					bool had_matrix = false;
					for(i2 = i + 1; i2 < SIZE; i2++) {
						if(had_matrix && !CHILD(i2).representsNonMatrix()) continue;
						int r = CHILD(i).merge_multiplication(CHILD(i2), eo, this, i, i2);
						if(r == 0) {
							SWAP_CHILDREN(i, i2);
							r = CHILD(i).merge_multiplication(CHILD(i2), eo, this, i, i2, true);
							if(r < 1) {
								SWAP_CHILDREN(i, i2);
							} else if(r == 2) {
								r = 3;
							} else if(r == 3) {
								r = 2;
							}
						}
						if(r >= 1) {
							ERASE(i2);
							b = true;
							if(r != 2) {
								i2 = 0;
								goto try_multiply_matrix;
							}
							i2--;
						}
						if(i == SIZE - 1) break;
						if(b_matrix && !CHILD(i2).representsNonMatrix()) had_matrix = true;
					}
					if(i3 < SIZE) {
						if(i3 == SIZE - 1) break;
						i = i3;
						i3 = SIZE;
						i2 = i + 1;
						goto try_multiply_matrix;
					}												
				}
			}
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_AND: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_and, try_bitand)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_OR: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_or, try_bitor)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_XOR: {
			MERGE_RECURSE
			MERGE_ALL(merge_bitwise_xor, try_bitxor)
			MERGE_ALL2
			break;
		}
		case STRUCT_BITWISE_NOT: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILDREN_UPDATED;
			}
			switch(CHILD(0).type()) {
				case STRUCT_NUMBER: {
					Number nr(CHILD(0).number());
					if(nr.bitNot() && (eo.approximation == APPROXIMATION_APPROXIMATE || !nr.isApproximate() || CHILD(0).number().isApproximate()) && (eo.allow_complex || !nr.isComplex() || CHILD(0).number().isComplex()) && (eo.allow_infinite || !nr.isInfinite() || CHILD(0).number().isInfinite())) {
						set(nr, true);
					}
					break;
				}
				case STRUCT_VECTOR: {
					SET_CHILD_MAP(0);
					for(size_t i = 0; i < SIZE; i++) {
						CHILD(i).setLogicalNot();
					}
					break;
				}
				case STRUCT_BITWISE_NOT: {
					setToChild(1);
					setToChild(1);
					break;
				}
				default: {}
			}
			break;
		}
		case STRUCT_LOGICAL_AND: {			
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculatesub(eo, feo, true, this, i);
					CHILD_UPDATED(i)
					if(CHILD(i).representsNonPositive()) {
						clear(true);
						b = true;
						break;
					}
				}
				if(b) break;				
			}
			MERGE_ALL(merge_logical_and, try_logand)
			if(SIZE == 1) {
				if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					setToChild(1, false, mparent, index_this + 1);
				} else if(CHILD(0).representsPositive()) {
					set(1, 1, 0, true);			
				} else if(CHILD(0).representsNonPositive()) {
					clear(true);
				} else {
					APPEND(m_zero);
					m_type = STRUCT_COMPARISON;
					ct_comp = COMPARISON_GREATER;
				}
			} else if(SIZE == 0) {
				clear(true);
			} else {
				evalSort();
			}
			break;
		}
		case STRUCT_LOGICAL_OR: {
            bool isResistance = false;
            switch (CHILD(0).type()) {
            case STRUCT_MULTIPLICATION:
                if (CHILD(0).CHILD(1) != 0 && CHILD(0).CHILD(1).unit() && CHILD(0).CHILD(1).unit()->name().find("ohm") != string::npos) {
                    isResistance = true;
                }
                break;
            case STRUCT_UNIT:
                if (CHILD(0).unit() && CHILD(0).unit()->name().find("ohm") != string::npos) {
                    isResistance = true;
                }
                break;
            }
            
            if (isResistance) {
                MathStructure mstruct;
                for (size_t i = 0; i < SIZE; i++) {
                    MathStructure mtemp(CHILD(i));
                    mtemp.inverse();
                    mstruct += mtemp;
                }
                mstruct.inverse();
                clear();
                set(mstruct);
                break;
            }
            
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).calculatesub(eo, feo, true, this, i);
					CHILD_UPDATED(i)
					if(CHILD(i).representsPositive()) {
						set(1, 1, 0, true);
						b = true;
						break;
					}
				}
				if(b) break;				
			}
			MERGE_ALL(merge_logical_or, try_logor)
			if(SIZE == 1) {
				if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					setToChild(1, false, mparent, index_this + 1);
				} else if(CHILD(0).representsPositive()) {
					set(1, 1, 0, true);
				} else if(CHILD(0).representsNonPositive()) {
					clear(true);
				} else {
					APPEND(m_zero);
					m_type = STRUCT_COMPARISON;
					ct_comp = COMPARISON_GREATER;
				}
			} else if(SIZE == 0) {
				clear(true);
			} else {
				evalSort();
			}
			break;
		}
		case STRUCT_LOGICAL_XOR: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILD(1).calculatesub(eo, feo, true, this, 1);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).merge_logical_xor(CHILD(1), eo) >= 1) {
				b = true;
				setToChild(1, false, mparent, index_this + 1);				
			}
			break;
		}
		case STRUCT_LOGICAL_NOT: {
			if(recursive) {
				CHILD(0).calculatesub(eo, feo, true, this, 0);
				CHILDREN_UPDATED;
			}
			if(CHILD(0).representsPositive()) {
				clear(true);
				b = true;
			} else if(CHILD(0).representsNonPositive()) {
				set(1, 1, 0, true);
				b = true;
			} else if(CHILD(0).isLogicalNot()) {
				setToChild(1);
				setToChild(1);
				if(!representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
					add(m_zero, OPERATION_GREATER);
					calculatesub(eo, feo, false);
				}
				b = true;
			}
			break;
		}
		case STRUCT_COMPARISON: {
			EvaluationOptions eo2 = eo;
			eo2.assume_denominators_nonzero = false;			
			if(recursive) {					
				CHILD(0).calculatesub(eo2, feo, true, this, 0);
				CHILD(1).calculatesub(eo2, feo, true, this, 1);
				CHILDREN_UPDATED;
			}			
			if(CHILD(0).isNumber() && CHILD(1).isNumber()) {
				ComparisonResult cr = CHILD(1).number().compareApproximately(CHILD(0).number());
				if(cr == COMPARISON_RESULT_UNKNOWN) {
					break;
				}
				switch(ct_comp) {
					case COMPARISON_EQUALS: {
						if(cr == COMPARISON_RESULT_EQUAL) {
							set(1, 1, 0, true);
							b = true;
						} else if(COMPARISON_IS_NOT_EQUAL(cr)) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_NOT_EQUALS: {
						if(cr == COMPARISON_RESULT_EQUAL) {
							clear(true);
							b = true;
						} else if(COMPARISON_IS_NOT_EQUAL(cr)) {
							set(1, 1, 0, true);
							b = true;
						}
						break;
					}
					case COMPARISON_LESS: {
						if(cr == COMPARISON_RESULT_LESS) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_LESS && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_GREATER && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_GREATER: {
						if(cr == COMPARISON_RESULT_GREATER) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_GREATER && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(COMPARISON_IS_EQUAL_OR_GREATER(cr)) {
							set(1, 1, 0, true);
							b = true;
						} else if(cr != COMPARISON_RESULT_EQUAL_OR_LESS && cr != COMPARISON_RESULT_NOT_EQUAL) {
							clear(true);
							b = true;
						}
						break;
					}
				}
				break;		
			}			
			if(!eo.test_comparisons) {
				break;
			}			
			if((CHILD(0).representsUndefined() && !CHILD(1).representsUndefined(true, true, true)) || (CHILD(1).representsUndefined() && !CHILD(0).representsUndefined(true, true, true))) {
				if(ct_comp == COMPARISON_EQUALS) {
					clear(true);
					b = true;
					break;
				} else if(ct_comp == COMPARISON_NOT_EQUALS) {
					set(1, 1, 0, true);
					b = true;
					break;
				}
			}
			if((ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_GREATER) && CHILD(1).isZero()) {
				if(CHILD(0).isLogicalNot() || CHILD(0).isLogicalAnd() || CHILD(0).isLogicalOr() || CHILD(0).isLogicalXor() || CHILD(0).isComparison()) {
					if(ct_comp == COMPARISON_EQUALS_LESS) {
						ERASE(1);
						m_type = STRUCT_LOGICAL_NOT;
						calculatesub(eo, feo, false, mparent, index_this);
					} else {
						setToChild(1, false, mparent, index_this + 1);
					}
					b = true;
				}
			} else if((ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_LESS) && CHILD(0).isZero()) {
				if(CHILD(0).isLogicalNot() || CHILD(1).isLogicalAnd() || CHILD(1).isLogicalOr() || CHILD(1).isLogicalXor() || CHILD(1).isComparison()) {
					if(ct_comp == COMPARISON_EQUALS_GREATER) {
						ERASE(0);
						m_type = STRUCT_LOGICAL_NOT;
						calculatesub(eo, feo, false, mparent, index_this);
					} else {
						setToChild(2, false, mparent, index_this + 1);
					}
					b = true;
				}
			}
			if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
				if((CHILD(0).representsReal(false) && CHILD(1).representsComplex(false)) || (CHILD(1).representsReal(false) && CHILD(0).representsComplex(false))) {
					if(ct_comp == COMPARISON_EQUALS) {
						clear(true);
					} else {
						set(1, 1, 0, true);
					}
					b = true;
				}
			}
			if(b) break;			
			MathStructure *mtest;
			if(!CHILD(1).isZero()) {
				if(!eo.isolate_x || find_x_var().isUndefined()) {
					CHILD(0).calculateSubtract(CHILD(1), eo2);
					CHILD(1).clear();
					mtest = &CHILD(0);
					mtest->ref();
				} else {
					mtest = new MathStructure(CHILD(0));
					mtest->calculateSubtract(CHILD(1), eo2);
				}
			} else {
				mtest = &CHILD(0);
				mtest->ref();				
			}
			bool incomp = false;
			if(mtest->isAddition()) {
				for(size_t i = 1; i < mtest->size(); i++) {
					if((*mtest)[i - 1].isUnitCompatible((*mtest)[i]) == 0) {
						incomp = true;
						break;
					}
				}
			}
			switch(ct_comp) {
				case COMPARISON_EQUALS: {
					if(incomp) {
						clear(true);
						b = true;
					} else if(mtest->representsZero(true)) {
						set(1, 1, 0, true);
						b = true;	
					} else if(mtest->representsNonZero(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_NOT_EQUALS:  {
					if(incomp) {
						set(1, 1, 0, true);
						b = true;
					} else if(mtest->representsNonZero(true)) {
						set(1, 1, 0, true);
						b = true;	
					} else if(mtest->representsZero(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_LESS:  {
					if(incomp) {
					} else if(mtest->representsNegative(true)) {
						set(1, 1, 0, true);
						b = true;	
					} else if(mtest->representsNonNegative(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_GREATER:  {
					if(incomp) {
					} else if(mtest->representsPositive(true)) {
						set(1, 1, 0, true);
						b = true;	
					} else if(mtest->representsNonPositive(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_EQUALS_LESS:  {
					if(incomp) {
					} else if(mtest->representsNonPositive(true)) {
						set(1, 1, 0, true);
						b = true;	
					} else if(mtest->representsPositive(true)) {
						clear(true);
						b = true;
					}
					break;
				}
				case COMPARISON_EQUALS_GREATER:  {
					if(incomp) {
					} else if(mtest->representsNonNegative(true)) {
						set(1, 1, 0, true);
						b = true;	
					} else if(mtest->representsNegative(true)) {
						clear(true);
						b = true;
					}
					break;
				}
			}
			mtest->unref();
			break;
		}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_abs) {
				b = calculateFunctions(eo, false);
				unformat(eo);
				if(b) calculatesub(eo, feo, true, mparent, index_this);
				break;
			}
		}
		default: {
			if(recursive) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).calculatesub(eo, feo, true, this, i)) b = true;
				}
				CHILDREN_UPDATED;
			}
		}
	}
	return b;
}

#define MERGE_INDEX(FUNC, TRY_LABEL)	bool b = false;\
					TRY_LABEL:\
					for(size_t i = 0; i < index; i++) {\
						int r = CHILD(i).FUNC(CHILD(index), eo, this, i, index);\
						if(r == 0) {\
							SWAP_CHILDREN(i, index);\
							r = CHILD(i).FUNC(CHILD(index), eo, this, i, index, true);\
							if(r < 1) {\
								SWAP_CHILDREN(i, index);\
							} else if(r == 2) {\
								r = 3;\
							} else if(r == 3) {\
								r = 2;\
							}\
						}\
						if(r >= 1) {\
							ERASE(index);\
							if(!b && r == 2) {\
								b = true;\
								index = SIZE;\
								break;\
							} else {\
								b = true;\
								index = i;\
								goto TRY_LABEL;\
							}\
						}\
					}\
					for(size_t i = index + 1; i < SIZE; i++) {\
						int r = CHILD(index).FUNC(CHILD(i), eo, this, index, i);\
						if(r == 0) {\
							SWAP_CHILDREN(index, i);\
							r = CHILD(index).FUNC(CHILD(i), eo, this, index, i, true);\
							if(r < 1) {\
								SWAP_CHILDREN(index, i);\
							} else if(r == 2) {\
								r = 3;\
							} else if(r == 3) {\
								r = 2;\
							}\
						}\
						if(r >= 1) {\
							ERASE(i);\
							if(!b && r == 3) {\
								b = true;\
								break;\
							}\
							b = true;\
							if(r != 2) {\
								goto TRY_LABEL;\
							}\
							i--;\
						}\
					}

#define MERGE_INDEX2			if(b && check_size) {\
						if(SIZE == 1) {\
							setToChild(1, false, mparent, index_this + 1);\
						} else if(SIZE == 0) {\
							clear(true);\
						} else {\
							evalSort();\
						}\
						return true;\
					} else {\
						evalSort();\
						return b;\
					}


bool MathStructure::calculateMergeIndex(size_t index, const EvaluationOptions &eo, const EvaluationOptions &feo, MathStructure *mparent, size_t index_this) {
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			return calculateMultiplyIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_ADDITION: {
			return calculateAddIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_POWER: {
			return calculateRaiseExponent(eo, mparent, index_this);
		}
		case STRUCT_LOGICAL_AND: {
			return calculateLogicalAndIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_LOGICAL_OR: {
			return calculateLogicalOrIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_LOGICAL_XOR: {
			return calculateLogicalXorLast(eo, mparent, index_this);
		}
		case STRUCT_BITWISE_AND: {
			return calculateBitwiseAndIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_BITWISE_OR: {
			return calculateBitwiseOrIndex(index, eo, true, mparent, index_this);
		}
		case STRUCT_BITWISE_XOR: {
			return calculateBitwiseXorIndex(index, eo, true, mparent, index_this);
		}
		default: {}
	}
	return calculatesub(eo, feo, false, mparent, index_this);
}
bool MathStructure::calculateLogicalOrLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateLogicalOrIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateLogicalOrIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isLogicalOr()) {
		CALCULATOR->error(true, "calculateLogicalOrIndex() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_logical_or, try_logical_or_index)
	
	if(b && check_size) {
		if(SIZE == 1) {
			if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
				setToChild(1, false, mparent, index_this + 1);
			} else if(CHILD(0).representsPositive()) {
				clear(true);
				o_number.setTrue();					
			} else if(CHILD(0).representsNonPositive()) {
				clear(true);
				o_number.setFalse();					
			} else {
				APPEND(m_zero);
				m_type = STRUCT_COMPARISON;
				ct_comp = COMPARISON_GREATER;
			}
		} else if(SIZE == 0) {
			clear(true);
		} else {
			evalSort();
		}
		return true;
	} else {
		evalSort();
		return false;
	}
	
}
bool MathStructure::calculateLogicalOr(const MathStructure &mor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mor, OPERATION_LOGICAL_OR, true);
	LAST.evalSort();
	return calculateLogicalOrIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateLogicalXorLast(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {

	if(!isLogicalXor()) {
		CALCULATOR->error(true, "calculateLogicalXorLast() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	if(CHILD(0).merge_logical_xor(CHILD(1), eo, this, 0, 1) >= 1) {
		if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
			setToChild(1, false, mparent, index_this + 1);
		} else if(CHILD(0).representsPositive()) {
			clear(true);
			o_number.setTrue();					
		} else if(CHILD(0).representsNonPositive()) {
			clear(true);
			o_number.setFalse();					
		} else {
			APPEND(m_zero);
			m_type = STRUCT_COMPARISON;
			ct_comp = COMPARISON_GREATER;
		}
		return true;
	}
	return false;
	
}
bool MathStructure::calculateLogicalXor(const MathStructure &mxor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mxor, OPERATION_LOGICAL_XOR);
	LAST.evalSort();
	return calculateLogicalXorLast(eo, mparent, index_this);
}
bool MathStructure::calculateLogicalAndLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateLogicalAndIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateLogicalAndIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isLogicalAnd()) {
		CALCULATOR->error(true, "calculateLogicalAndIndex() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_logical_and, try_logical_and_index)
	
	if(b && check_size) {
		if(SIZE == 1) {
			if(CHILD(0).representsBoolean() || (mparent && !mparent->isMultiplication() && mparent->representsBoolean())) {
				setToChild(1, false, mparent, index_this + 1);
			} else if(CHILD(0).representsPositive()) {
				clear(true);
				o_number.setTrue();					
			} else if(CHILD(0).representsNonPositive()) {
				clear(true);
				o_number.setFalse();					
			} else {
				APPEND(m_zero);
				m_type = STRUCT_COMPARISON;
				ct_comp = COMPARISON_GREATER;
			}	
		} else if(SIZE == 0) {
			clear(true);
		} else {
			evalSort();
		}
		return true;
	} else {
		evalSort();
		return false;
	}
	
}
bool MathStructure::calculateLogicalAnd(const MathStructure &mand, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mand, OPERATION_LOGICAL_AND, true);
	LAST.evalSort();
	return calculateLogicalAndIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateInverse(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	return calculateRaise(m_minus_one, eo, mparent, index_this);
}
bool MathStructure::calculateNegate(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(!isMultiplication()) transform(STRUCT_MULTIPLICATION);
	PREPEND(m_minus_one);
	return calculateMultiplyIndex(0, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseNot(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	transform(STRUCT_LOGICAL_NOT);
	return calculatesub(eo, eo, false, mparent, index_this); 
}
bool MathStructure::calculateLogicalNot(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	transform(STRUCT_BITWISE_NOT);
	return calculatesub(eo, eo, false, mparent, index_this); 
}
bool MathStructure::calculateRaiseExponent(const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	if(!isPower()) {
		CALCULATOR->error(true, "calculateRaiseExponent() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}
	if(CHILD(0).merge_power(CHILD(1), eo, this, 0, 1) >= 1) {
		setToChild(1, false, mparent, index_this + 1);
		return true;
	}
	return false;
}
bool MathStructure::calculateRaise(const MathStructure &mexp, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	raise(mexp);
	LAST.evalSort();
	return calculateRaiseExponent(eo, mparent, index_this);
}
bool MathStructure::calculateBitwiseAndLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseAndIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseAndIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseAnd()) {
		CALCULATOR->error(true, "calculateBitwiseAndIndex() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_and, try_bitwise_and_index)
	MERGE_INDEX2
	
}
bool MathStructure::calculateBitwiseAnd(const MathStructure &mand, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mand, OPERATION_BITWISE_AND, true);
	LAST.evalSort();
	return calculateBitwiseAndIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseOrLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseOrIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseOrIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseOr()) {
		CALCULATOR->error(true, "calculateBitwiseOrIndex() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_or, try_bitwise_or_index)
	MERGE_INDEX2
	
}
bool MathStructure::calculateBitwiseOr(const MathStructure &mor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mor, OPERATION_BITWISE_OR, true);
	LAST.evalSort();
	return calculateBitwiseOrIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateBitwiseXorLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateBitwiseXorIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateBitwiseXorIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isBitwiseXor()) {
		CALCULATOR->error(true, "calculateBitwiseXorIndex() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_bitwise_xor, try_bitwise_xor_index)
	MERGE_INDEX2
	
}
bool MathStructure::calculateBitwiseXor(const MathStructure &mxor, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(mxor, OPERATION_BITWISE_XOR, true);
	LAST.evalSort();
	return calculateBitwiseXorIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateMultiplyLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateMultiplyIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateMultiplyIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {

	if(index >= SIZE || !isMultiplication()) {
		CALCULATOR->error(true, "calculateMultiplyIndex() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	bool b = false;		
	try_multiply_matrix_index:
	bool b_matrix = !CHILD(index).representsNonMatrix();
	if(index > 0) {
		for(size_t i = index - 1; ; i--) {
			int r = CHILD(i).merge_multiplication(CHILD(index), eo, this, i, index);
			if(r == 0) {
				SWAP_CHILDREN(i, index);
				r = CHILD(i).merge_multiplication(CHILD(index), eo, this, i, index, true);
				if(r < 1) {
					SWAP_CHILDREN(i, index);
				} else if(r == 2) {
					r = 3;
				} else if(r == 3) {
					r = 2;
				}
			}
			if(r >= 1) {
				ERASE(index);
				if(!b && r == 2) {
					b = true;
					index = SIZE;
					break;
				} else {
					b = true;
					index = i;
					goto try_multiply_matrix_index;
				}
			}
			if(i == 0) break;
			if(b_matrix && !CHILD(i).representsNonMatrix()) break;
		}
	}
	bool had_matrix = false;
	for(size_t i = index + 1; i < SIZE; i++) {
		if(had_matrix && !CHILD(i).representsNonMatrix()) continue;
		int r = CHILD(index).merge_multiplication(CHILD(i), eo, this, index, i);
		if(r == 0) {
			SWAP_CHILDREN(index, i);
			r = CHILD(index).merge_multiplication(CHILD(i), eo, this, index, i, true);
			if(r < 1) {
				SWAP_CHILDREN(index, i);
			} else if(r == 2) {
				r = 3;
			} else if(r == 3) {
				r = 2;
			}
		}
		if(r >= 1) {
			ERASE(i);
			if(!b && r == 3) {
				b = true;
				break;
			}
			b = true;							
			if(r != 2) {								
				goto try_multiply_matrix_index;
			}
			i--;
		}
		if(i == SIZE - 1) break;
		if(b_matrix && !CHILD(i).representsNonMatrix()) had_matrix = true;
	}
	
	MERGE_INDEX2
		
}
bool MathStructure::calculateMultiply(const MathStructure &mmul, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	multiply(mmul, true);
	LAST.evalSort();
	return calculateMultiplyIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateDivide(const MathStructure &mdiv, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	MathStructure *mmul = new MathStructure(mdiv);
	mmul->evalSort();
	multiply_nocopy(mmul, true);
	LAST.calculateInverse(eo, this, SIZE - 1);
	return calculateMultiplyIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateAddLast(const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	return calculateAddIndex(SIZE - 1, eo, check_size, mparent, index_this);
}
bool MathStructure::calculateAddIndex(size_t index, const EvaluationOptions &eo, bool check_size, MathStructure *mparent, size_t index_this) {
	
	if(index >= SIZE || !isAddition()) {
		CALCULATOR->error(true, "calculateAddIndex() error: %s. %s", print().c_str(), _("This is a bug. Please report it."), NULL);
		return false;
	}

	MERGE_INDEX(merge_addition, try_add_index)
	MERGE_INDEX2	
	
}
bool MathStructure::calculateAdd(const MathStructure &madd, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	add(madd, true);
	LAST.evalSort();
	return calculateAddIndex(SIZE - 1, eo, true, mparent, index_this);
}
bool MathStructure::calculateSubtract(const MathStructure &msub, const EvaluationOptions &eo, MathStructure *mparent, size_t index_this) {
	MathStructure *madd = new MathStructure(msub);
	madd->evalSort();	
	add_nocopy(madd, true);
	LAST.calculateNegate(eo, this, SIZE - 1);
	return calculateAddIndex(SIZE - 1, eo, true, mparent, index_this);
}

bool MathStructure::calculateFunctions(const EvaluationOptions &eo, bool recursive) {

	if(m_type == STRUCT_FUNCTION) {

		if(function_value) {
			function_value->unref();
			function_value = NULL;
		}

		if(!o_function->testArgumentCount(SIZE)) {
			return false;
		}

		if(o_function->maxargs() > -1 && (int) SIZE > o_function->maxargs()) {
			REDUCE(o_function->maxargs());
		}
		m_type = STRUCT_VECTOR;
		Argument *arg = NULL, *last_arg = NULL;
		int last_i = 0;

		for(size_t i = 0; i < SIZE; i++) {
			arg = o_function->getArgumentDefinition(i + 1);
			if(arg) {
				last_arg = arg;
				last_i = i;
				if(!arg->test(CHILD(i), i + 1, o_function, eo)) {
					m_type = STRUCT_FUNCTION;
					CHILD_UPDATED(i);
					return false;
				} else {
					CHILD_UPDATED(i);
				}
			}
		}

		if(last_arg && o_function->maxargs() < 0 && last_i >= o_function->minargs()) {
			for(size_t i = last_i + 1; i < SIZE; i++) {
				if(!last_arg->test(CHILD(i), i + 1, o_function, eo)) {
					m_type = STRUCT_FUNCTION;
					CHILD_UPDATED(i);
					return false;
				} else {
					CHILD_UPDATED(i);
				}
			}
		}

		if(!o_function->testCondition(*this)) {
			m_type = STRUCT_FUNCTION;
			return false;
		}
		MathStructure *mstruct = new MathStructure();
		int i = o_function->calculate(*mstruct, *this, eo);
		if(i > 0) {
			set_nocopy(*mstruct, true);
			if(recursive) calculateFunctions(eo);
			mstruct->unref();
			return true;
		} else {
			if(i < 0) {
				i = -i;
				if(o_function->maxargs() > 0 && i > o_function->maxargs()) {
					if(mstruct->isVector()) {
						for(size_t arg_i = 1; arg_i <= SIZE && arg_i <= mstruct->size(); arg_i++) {
							mstruct->getChild(arg_i)->ref();
							setChild_nocopy(mstruct->getChild(arg_i), arg_i);
						}
					}
				} else if(i <= (int) SIZE) {
					mstruct->ref();
					setChild_nocopy(mstruct, i);
				}
			}
			/*if(eo.approximation == APPROXIMATION_EXACT) {
				mstruct->clear();
				EvaluationOptions eo2 = eo;
				eo2.approximation = APPROXIMATION_APPROXIMATE;
				CALCULATOR->beginTemporaryStopMessages();
				if(o_function->calculate(*mstruct, *this, eo2) > 0) {
					function_value = mstruct;
					function_value->ref();
					function_value->calculateFunctions(eo2);
				}
				if(CALCULATOR->endTemporaryStopMessages() > 0 && function_value) {
					function_value->unref();
					function_value = NULL;
				}
			}*/
			m_type = STRUCT_FUNCTION;			
			mstruct->unref();
			return false;
		}
	}
	bool b = false;
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).calculateFunctions(eo)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
	}
	return b;

}

int evalSortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent);
int evalSortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent) {
	if(parent.isMultiplication()) {
		if(!mstruct1.representsNonMatrix() && !mstruct2.representsNonMatrix()) {
			return 0;
		}
	}
	if(parent.isAddition()) {
		if(mstruct1.isMultiplication() && mstruct1.size() > 0) {
			size_t start = 0;
			while(mstruct1[start].isNumber() && mstruct1.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct2.isMultiplication()) {
				if(mstruct2.size() < 1) return -1;
				size_t start2 = 0;
				while(mstruct2[start2].isNumber() && mstruct2.size() > start2 + 1) {
					start2++;
				}
				for(size_t i = 0; i + start < mstruct1.size(); i++) {
					if(i + start2 >= mstruct2.size()) return 1;
					i2 = evalSortCompare(mstruct1[i + start], mstruct2[i + start2], parent);
					if(i2 != 0) return i2;
				}
				if(mstruct1.size() - start == mstruct2.size() - start2) return 0;
				return -1;
			} else {
				i2 = evalSortCompare(mstruct1[start], mstruct2, parent);
				if(i2 != 0) return i2;
			}
		} else if(mstruct2.isMultiplication() && mstruct2.size() > 0) {
			size_t start = 0;
			while(mstruct2[start].isNumber() && mstruct2.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct1.isMultiplication()) {
				return 1;
			} else {
				i2 = evalSortCompare(mstruct1, mstruct2[start], parent);
				if(i2 != 0) return i2;
			}
		} 
	}
	if(mstruct1.type() != mstruct2.type()) {
		if(!parent.isMultiplication()) {
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		if(!parent.isMultiplication() || (!mstruct1.isNumber() && !mstruct2.isNumber())) {
			if(mstruct2.isPower()) {
				int i = evalSortCompare(mstruct1, mstruct2[0], parent);
				if(i == 0) {
					return evalSortCompare(m_one, mstruct2[1], parent);
				}
				return i;
			}
			if(mstruct1.isPower()) {
				int i = evalSortCompare(mstruct1[0], mstruct2, parent);
				if(i == 0) {
					return evalSortCompare(mstruct1[1], m_one, parent);
				}
				return i;
			}
		}
		if(mstruct2.isInverse()) return -1;
		if(mstruct1.isInverse()) return 1;
		if(mstruct2.isDivision()) return -1;
		if(mstruct1.isDivision()) return 1;
		if(mstruct2.isNegate()) return -1;
		if(mstruct1.isNegate()) return 1;
		if(mstruct2.isLogicalAnd()) return -1;
		if(mstruct1.isLogicalAnd()) return 1;
		if(mstruct2.isLogicalOr()) return -1;
		if(mstruct1.isLogicalOr()) return 1;
		if(mstruct2.isLogicalXor()) return -1;
		if(mstruct1.isLogicalXor()) return 1;
		if(mstruct2.isLogicalNot()) return -1;
		if(mstruct1.isLogicalNot()) return 1;
		if(mstruct2.isComparison()) return -1;
		if(mstruct1.isComparison()) return 1;
		if(mstruct2.isBitwiseOr()) return -1;
		if(mstruct1.isBitwiseOr()) return 1;
		if(mstruct2.isBitwiseXor()) return -1;
		if(mstruct1.isBitwiseXor()) return 1;
		if(mstruct2.isBitwiseAnd()) return -1;
		if(mstruct1.isBitwiseAnd()) return 1;
		if(mstruct2.isBitwiseNot()) return -1;
		if(mstruct1.isBitwiseNot()) return 1;
		if(mstruct2.isUndefined()) return -1;
		if(mstruct1.isUndefined()) return 1;
		if(mstruct2.isFunction()) return -1;
		if(mstruct1.isFunction()) return 1;
		if(mstruct2.isAddition()) return -1;
		if(mstruct1.isAddition()) return 1;
		if(mstruct2.isMultiplication()) return -1;
		if(mstruct1.isMultiplication()) return 1;
		if(mstruct2.isPower()) return -1;
		if(mstruct1.isPower()) return 1;
		if(mstruct2.isUnit()) return -1;
		if(mstruct1.isUnit()) return 1;
		if(mstruct2.isSymbolic()) return -1;
		if(mstruct1.isSymbolic()) return 1;
		if(mstruct2.isVariable()) return -1;
		if(mstruct1.isVariable()) return 1;
		if(parent.isMultiplication()) {
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		return -1;
	}
	switch(mstruct1.type()) {
		case STRUCT_NUMBER: {
			if(!mstruct1.number().isComplex() && !mstruct2.number().isComplex()) {
				ComparisonResult cmp = mstruct1.number().compare(mstruct2.number());
				if(cmp == COMPARISON_RESULT_LESS) return -1;
				else if(cmp == COMPARISON_RESULT_GREATER) return 1;
				return 0;
			} else {
				if(!mstruct1.number().hasRealPart()) {
					if(mstruct2.number().hasRealPart()) {
						return 1;
					} else {
						ComparisonResult cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
						if(cmp == COMPARISON_RESULT_LESS) return -1;
						else if(cmp == COMPARISON_RESULT_GREATER) return 1;
						return 0;
					}
				} else if(mstruct2.number().hasRealPart()) {
					ComparisonResult cmp = mstruct1.number().compareRealParts(mstruct2.number());
					if(cmp == COMPARISON_RESULT_EQUAL) {
						cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
					} 
					if(cmp == COMPARISON_RESULT_LESS) return -1;
					else if(cmp == COMPARISON_RESULT_GREATER) return 1;
					return 0;
				} else {
					return -1;
				}
			}
			return -1;
		} 
		case STRUCT_UNIT: {
			if(mstruct1.unit() < mstruct2.unit()) return -1;
			if(mstruct1.unit() == mstruct2.unit()) return 0;
			return 1;
		}
		case STRUCT_SYMBOLIC: {
			if(mstruct1.symbol() < mstruct2.symbol()) return -1;
			else if(mstruct1.symbol() == mstruct2.symbol()) return 0;
			return 1;
		}
		case STRUCT_VARIABLE: {
			if(mstruct1.variable() < mstruct2.variable()) return -1;
			else if(mstruct1.variable() == mstruct2.variable()) return 0;
			return 1;
		}
		case STRUCT_FUNCTION: {
			if(mstruct1.function() < mstruct2.function()) return -1;
			if(mstruct1.function() == mstruct2.function()) {
				for(size_t i = 0; i < mstruct2.size(); i++) {
					if(i >= mstruct1.size()) {
						return -1;	
					}
					int i2 = evalSortCompare(mstruct1[i], mstruct2[i], parent);
					if(i2 != 0) return i2;
				}
				return 0;
			}
			return 1;
		}
		case STRUCT_POWER: {
			int i = evalSortCompare(mstruct1[0], mstruct2[0], parent);
			if(i == 0) {
				return evalSortCompare(mstruct1[1], mstruct2[1], parent);
			}
			return i;
		}
		default: {
			if(mstruct2.size() < mstruct1.size()) return -1;
			else if(mstruct2.size() > mstruct1.size()) return 1;
			int ie;
			for(size_t i = 0; i < mstruct1.size(); i++) {
				ie = evalSortCompare(mstruct1[i], mstruct2[i], parent);
				if(ie != 0) {
					return ie;
				}
			}
		}
	}
	return 0;
}

void MathStructure::evalSort(bool recursive) {
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).evalSort(true);
		}
	}
	//if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_LOGICAL_AND && m_type != STRUCT_LOGICAL_OR && m_type != STRUCT_LOGICAL_XOR && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR) return;
	if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR) return;
	vector<size_t> sorted;
	for(size_t i = 0; i < SIZE; i++) {
		if(i == 0) {
			sorted.push_back(v_order[i]);
		} else {
			if(evalSortCompare(CHILD(i), *v_subs[sorted.back()], *this) >= 0) {
				sorted.push_back(v_order[i]);
			} else if(sorted.size() == 1) {
				sorted.insert(sorted.begin(), v_order[i]);
			} else {
				for(size_t i2 = sorted.size() - 2; ; i2--) {
					if(evalSortCompare(CHILD(i), *v_subs[sorted[i2]], *this) >= 0) {	
						sorted.insert(sorted.begin() + i2 + 1, v_order[i]);
						break;
					}
					if(i2 == 0) {
						sorted.insert(sorted.begin(), v_order[i]);
						break;
					}
				}
			}
		}
	}
	for(size_t i2 = 0; i2 < sorted.size(); i2++) {
		v_order[i2] = sorted[i2];
	}
}

int sortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, const PrintOptions &po);
int sortCompare(const MathStructure &mstruct1, const MathStructure &mstruct2, const MathStructure &parent, const PrintOptions &po) {
	if(parent.isMultiplication()) {
		if(!mstruct1.representsNonMatrix() && !mstruct2.representsNonMatrix()) {
			return 0;
		}
	}
	if(parent.isAddition() && po.sort_options.minus_last) {
		bool m1 = mstruct1.hasNegativeSign(), m2 = mstruct2.hasNegativeSign();
		if(m1 && !m2) {
			return 1;
		} else if(m2 && !m1) {
			return -1;
		}
	}
	bool isdiv1 = false, isdiv2 = false;
	if(!po.negative_exponents) {
		if(mstruct1.isMultiplication()) {
			for(size_t i = 0; i < mstruct1.size(); i++) {
				if(mstruct1[i].isPower() && mstruct1[i][1].hasNegativeSign()) {
					isdiv1 = true;
					break;
				}
			}
		} else if(mstruct1.isPower() && mstruct1[1].hasNegativeSign()) {
			isdiv1 = true;
		}
		if(mstruct2.isMultiplication()) {
			for(size_t i = 0; i < mstruct2.size(); i++) {
				if(mstruct2[i].isPower() && mstruct2[i][1].hasNegativeSign()) {
					isdiv2 = true;
					break;
				}
			}
		} else if(mstruct2.isPower() && mstruct2[1].hasNegativeSign()) {
			isdiv2 = true;
		}
	}
	if(parent.isAddition() && isdiv1 == isdiv2) {
		if(mstruct1.isMultiplication() && mstruct1.size() > 0) {
			size_t start = 0;
			while(mstruct1[start].isNumber() && mstruct1.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct2.isMultiplication()) {
				if(mstruct2.size() < 1) return -1;
				size_t start2 = 0;
				while(mstruct2[start2].isNumber() && mstruct2.size() > start2 + 1) {
					start2++;
				}
				for(size_t i = 0; i + start < mstruct1.size(); i++) {
					if(i + start2 >= mstruct2.size()) return 1;
					i2 = sortCompare(mstruct1[i + start], mstruct2[i + start2], parent, po);
					if(i2 != 0) return i2;
				}
				if(mstruct1.size() - start == mstruct2.size() - start2) return 0;
				if(parent.isMultiplication()) return -1;
				else return 1;
			} else {
				i2 = sortCompare(mstruct1[start], mstruct2, parent, po);
				if(i2 != 0) return i2;
			}
		} else if(mstruct2.isMultiplication() && mstruct2.size() > 0) {
			size_t start = 0;
			while(mstruct2[start].isNumber() && mstruct2.size() > start + 1) {
				start++;
			}
			int i2;
			if(mstruct1.isMultiplication()) {
				return 1;
			} else {
				i2 = sortCompare(mstruct1, mstruct2[start], parent, po);
				if(i2 != 0) return i2;
			}
		} 
	}
	if(mstruct1.type() != mstruct2.type()) {
		if(mstruct1.isVariable() && mstruct2.isSymbolic()) {
			if(parent.isMultiplication()) {
				if(mstruct1.variable()->isKnown()) return -1;
			}
			if(mstruct1.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name < mstruct2.symbol()) return -1;
			else return 1;
		}
		if(mstruct2.isVariable() && mstruct1.isSymbolic()) {
			if(parent.isMultiplication()) {
				if(mstruct2.variable()->isKnown()) return 1;
			}
			if(mstruct1.symbol() < mstruct2.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name) return -1;
			else return 1;
		}
		if(!parent.isMultiplication() || (!mstruct1.isNumber() && !mstruct2.isNumber())) {
			if(mstruct2.isPower()) {
				int i = sortCompare(mstruct1, mstruct2[0], parent, po);
				if(i == 0) {
					return sortCompare(m_one, mstruct2[1], parent, po);
				}
				return i;
			}
			if(mstruct1.isPower()) {
				int i = sortCompare(mstruct1[0], mstruct2, parent, po);
				if(i == 0) {
					return sortCompare(mstruct1[1], m_one, parent, po);
				}
				return i;
			}
		}
		if(parent.isMultiplication()) {
			if(mstruct2.isUnit()) return -1;
			if(mstruct1.isUnit()) return 1;
		}
		if(mstruct2.isInverse()) return -1;
		if(mstruct1.isInverse()) return 1;
		if(mstruct2.isDivision()) return -1;
		if(mstruct1.isDivision()) return 1;
		if(mstruct2.isNegate()) return -1;
		if(mstruct1.isNegate()) return 1;
		if(mstruct2.isLogicalAnd()) return -1;
		if(mstruct1.isLogicalAnd()) return 1;
		if(mstruct2.isLogicalOr()) return -1;
		if(mstruct1.isLogicalOr()) return 1;
		if(mstruct2.isLogicalXor()) return -1;
		if(mstruct1.isLogicalXor()) return 1;
		if(mstruct2.isLogicalNot()) return -1;
		if(mstruct1.isLogicalNot()) return 1;
		if(mstruct2.isComparison()) return -1;
		if(mstruct1.isComparison()) return 1;
		if(mstruct2.isBitwiseOr()) return -1;
		if(mstruct1.isBitwiseOr()) return 1;
		if(mstruct2.isBitwiseXor()) return -1;
		if(mstruct1.isBitwiseXor()) return 1;
		if(mstruct2.isBitwiseAnd()) return -1;
		if(mstruct1.isBitwiseAnd()) return 1;
		if(mstruct2.isBitwiseNot()) return -1;
		if(mstruct1.isBitwiseNot()) return 1;
		if(mstruct2.isUndefined()) return -1;
		if(mstruct1.isUndefined()) return 1;
		if(mstruct2.isFunction()) return -1;
		if(mstruct1.isFunction()) return 1;
		if(mstruct2.isAddition()) return -1;
		if(mstruct1.isAddition()) return 1;
		if(!parent.isMultiplication()) {
			if(isdiv2 && mstruct2.isMultiplication()) return -1;
			if(isdiv1 && mstruct1.isMultiplication()) return 1;
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		if(mstruct2.isMultiplication()) return -1;
		if(mstruct1.isMultiplication()) return 1;
		if(mstruct2.isPower()) return -1;
		if(mstruct1.isPower()) return 1;
		if(mstruct2.isUnit()) return -1;
		if(mstruct1.isUnit()) return 1;
		if(mstruct2.isSymbolic()) return -1;
		if(mstruct1.isSymbolic()) return 1;
		if(mstruct2.isVariable()) return -1;
		if(mstruct1.isVariable()) return 1;
		if(parent.isMultiplication()) {
			if(mstruct2.isNumber()) return -1;
			if(mstruct1.isNumber()) return 1;
		}
		return -1;
	}
	switch(mstruct1.type()) {
		case STRUCT_NUMBER: {
			if(!mstruct1.number().isComplex() && !mstruct2.number().isComplex()) {
				ComparisonResult cmp;
				if(parent.isMultiplication() && mstruct2.number().isNegative() != mstruct1.number().isNegative()) cmp = mstruct2.number().compare(mstruct1.number());
				else cmp = mstruct1.number().compare(mstruct2.number());
				if(cmp == COMPARISON_RESULT_LESS) return -1;
				else if(cmp == COMPARISON_RESULT_GREATER) return 1;
				return 0;
			} else {
				if(!mstruct1.number().hasRealPart()) {
					if(mstruct2.number().hasRealPart()) {
						return 1;
					} else {
						ComparisonResult cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
						if(cmp == COMPARISON_RESULT_LESS) return -1;
						else if(cmp == COMPARISON_RESULT_GREATER) return 1;
						return 0;
					}
				} else if(mstruct2.number().hasRealPart()) {
					ComparisonResult cmp = mstruct1.number().compareRealParts(mstruct2.number());
					if(cmp == COMPARISON_RESULT_EQUAL) {
						cmp = mstruct1.number().compareImaginaryParts(mstruct2.number());
					} 
					if(cmp == COMPARISON_RESULT_LESS) return -1;
					else if(cmp == COMPARISON_RESULT_GREATER) return 1;
					return 0;
				} else {
					return -1;
				}
			}
			return -1;
		} 
		case STRUCT_UNIT: {
			if(mstruct1.unit() == mstruct2.unit()) return 0;
			if(mstruct1.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct1.isPlural(), po.use_reference_names).name < mstruct2.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct2.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name) return -1;
			return 1;
		}
		case STRUCT_SYMBOLIC: {
			if(mstruct1.symbol() < mstruct2.symbol()) return -1;
			else if(mstruct1.symbol() == mstruct2.symbol()) return 0;
			return 1;
		}
		case STRUCT_VARIABLE: {
			if(mstruct1.variable() == mstruct2.variable()) return 0;
			if(parent.isMultiplication()) {
				if(mstruct1.variable()->isKnown() && !mstruct2.variable()->isKnown()) return -1;
				if(!mstruct1.variable()->isKnown() && mstruct2.variable()->isKnown()) return 1;
			}
			if(mstruct1.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names).name < mstruct2.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name) return -1;
			return 1;
		}
		case STRUCT_FUNCTION: {
			if(mstruct1.function() == mstruct2.function()) {
				for(size_t i = 0; i < mstruct2.size(); i++) {
					if(i >= mstruct1.size()) {
						return -1;	
					}
					int i2 = sortCompare(mstruct1[i], mstruct2[i], parent, po);
					if(i2 != 0) return i2;
				}
				return 0;
			}
			if(mstruct1.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names).name < mstruct2.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg).name) return -1;
			return 1;
		}
		case STRUCT_POWER: {
			int i = sortCompare(mstruct1[0], mstruct2[0], parent, po);
			if(i == 0) {
				return sortCompare(mstruct1[1], mstruct2[1], parent, po);
			}
			return i;
		}
		case STRUCT_MULTIPLICATION: {
			if(isdiv1 != isdiv2) {
				if(isdiv1) return 1;
				return -1;
			}
		}
		case STRUCT_COMPARISON: {
			if((mstruct1.comparisonType() == COMPARISON_LESS || mstruct1.comparisonType() == COMPARISON_EQUALS_LESS) && (mstruct2.comparisonType() == COMPARISON_GREATER || mstruct2.comparisonType() == COMPARISON_EQUALS_GREATER)) {
				return 1;
			}
			if((mstruct1.comparisonType() == COMPARISON_GREATER || mstruct1.comparisonType() == COMPARISON_EQUALS_GREATER) && (mstruct2.comparisonType() == COMPARISON_LESS || mstruct2.comparisonType() == COMPARISON_EQUALS_LESS)) {
				return -1;
			}
		}
		default: {
			int ie;
			for(size_t i = 0; i < mstruct1.size(); i++) {
				if(i >= mstruct2.size()) return 1;
				ie = sortCompare(mstruct1[i], mstruct2[i], parent, po);
				if(ie != 0) {
					return ie;
				}
			}
		}
	}
	return 0;
}

void MathStructure::sort(const PrintOptions &po, bool recursive) {
	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).sort(po);
		}
	}
	if(m_type != STRUCT_ADDITION && m_type != STRUCT_MULTIPLICATION && m_type != STRUCT_BITWISE_AND && m_type != STRUCT_BITWISE_OR && m_type != STRUCT_BITWISE_XOR) return;
	vector<size_t> sorted;
	bool b;
	PrintOptions po2 = po;
	po2.sort_options.minus_last = po.sort_options.minus_last && SIZE == 2;
	//!containsUnknowns();
	for(size_t i = 0; i < SIZE; i++) {
		b = false;
		for(size_t i2 = 0; i2 < sorted.size(); i2++) {
			if(sortCompare(CHILD(i), *v_subs[sorted[i2]], *this, po2) < 0) {
				sorted.insert(sorted.begin() + i2, v_order[i]);
				b = true;
				break;
			}
		}
		if(!b) sorted.push_back(v_order[i]);
	}
	if(m_type == STRUCT_ADDITION && SIZE > 2 && po.sort_options.minus_last && v_subs[sorted[0]]->hasNegativeSign()) {
		for(size_t i2 = 1; i2 < sorted.size(); i2++) {
			if(!v_subs[sorted[i2]]->hasNegativeSign()) {
				sorted.insert(sorted.begin(), sorted[i2]);
				sorted.erase(sorted.begin() + (i2 + 1));
				break;
			}
		}
	}
	for(size_t i2 = 0; i2 < sorted.size(); i2++) {
		v_order[i2] = sorted[i2];
	}
}
bool MathStructure::containsOpaqueContents() const {
	if(isFunction()) return true;
	if(isUnit() && o_unit->subtype() != SUBTYPE_BASE_UNIT) return true;
	if(isVariable() && o_variable->isKnown()) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).containsOpaqueContents()) return true;
	}
	return false;
}
bool MathStructure::containsAdditionPower() const {
	if(m_type == STRUCT_POWER && CHILD(0).isAddition()) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).containsAdditionPower()) return true;
	}
	return false;
}

size_t MathStructure::countTotalChildren(bool count_function_as_one) const {
	if((m_type == STRUCT_FUNCTION && count_function_as_one) || SIZE == 0) return 1;
	size_t count = 0;
	for(size_t i = 0; i < SIZE; i++) {
		count += CHILD(i).countTotalChildren() + 1;
	}
	return count;
}
bool test_comparisons(const MathStructure &msave, MathStructure &mthis, const MathStructure &x_var, const EvaluationOptions &eo, bool sub = false);
bool try_isolate_x(MathStructure &mstruct, EvaluationOptions &eo3, const EvaluationOptions &eo);
bool try_isolate_x(MathStructure &mstruct, EvaluationOptions &eo3, const EvaluationOptions &eo) {
	if(mstruct.isProtected()) return false;
	if(mstruct.isComparison()) {
		MathStructure mtest(mstruct);
		eo3.test_comparisons = false;
		eo3.warn_about_denominators_assumed_nonzero = false;
		mtest[0].calculatesub(eo3, eo);
		mtest[1].calculatesub(eo3, eo);
		eo3.test_comparisons = eo.test_comparisons;
		const MathStructure *x_var2;
		if(eo.isolate_var) x_var2 = eo.isolate_var;
		else x_var2 = &mstruct.find_x_var();
		if(x_var2->isUndefined()) return false;
		if(mtest[0] == *x_var2 && !mtest[1].contains(*x_var2)) return false;
		if(mtest.isolate_x(eo3, eo, *x_var2, false)) {
			if(test_comparisons(mstruct, mtest, *x_var2, eo3)) {
				mstruct = mtest;
				return true;
			}
		}
	} else {
		bool b = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(try_isolate_x(mstruct[i], eo3, eo)) b = true;
		}
		return b;
	}
	return false;
}

bool compare_delete(MathStructure &mnum, MathStructure &mden, bool &erase1, bool &erase2, const EvaluationOptions &eo) {
	erase1 = false;
	erase2 = false;
	if(mnum == mden) {
		if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mnum.representsZero(true)) 
		|| mnum.representsNonZero(true)
		|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mnum.representsZero(true) && warn_about_denominators_assumed_nonzero(mnum, eo))) {
			erase1 = true;
			erase2 = true;			
		} else {
			if(mnum.isPower()) {
				mnum.setToChild(1);
				mden.setToChild(1);
				return true;
			}
			return false;
		}
		return true;
	}
	if(!mnum.isPower() && !mden.isPower()) return false;
	MathStructure *mbase1, *mbase2, *mexp1 = NULL, *mexp2 = NULL;
	if(mnum.isPower()) {
		if(!IS_REAL(mnum[1])) return false;
		mexp1 = &mnum[1];
		mbase1 = &mnum[0];
	} else {
		mbase1 = &mnum;
	}
	if(mden.isPower()) {
		if(!IS_REAL(mden[1])) return false;
		mexp2 = &mden[1];
		mbase2 = &mden[0];
	} else {
		mbase2 = &mden;
	}
	if(mbase1->equals(*mbase2)) {
		if(mexp1 && mexp2) {
			if(mexp1->number().isLessThan(mexp2->number())) {
				erase1 = true;
				mexp2->number() -= mexp1->number();
				if(mexp2->isOne()) mden.setToChild(1, true);
			} else {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true))
				|| mbase2->representsNonZero(true)
				|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true) && warn_about_denominators_assumed_nonzero(*mbase2, eo))) {
					erase2 = true;
					mexp1->number() -= mexp2->number();
					if(mexp1->isOne()) mnum.setToChild(1, true);
				} else {
					if(mexp2->number().isFraction()) return false;
					mexp2->number()--;
					mexp1->number() -= mexp2->number();
					if(mexp1->isOne()) mnum.setToChild(1, true);
					if(mexp2->isOne()) mden.setToChild(1, true);
					return true;
				}
				
			}
			return true;	
		} else if(mexp1) {
			if(mexp1->number().isFraction()) {
				erase1 = true;
				mbase2->raise(m_one);
				(*mbase2)[1].number() -= mexp1->number();
				return true;
			}
			if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true))
			|| mbase2->representsNonZero(true)
			|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true) && warn_about_denominators_assumed_nonzero(*mbase2, eo))) {
				mexp1->number()--;
				erase2 = true;
				if(mexp1->isOne()) mnum.setToChild(1, true);
				return true;
			}
			return false;
			
		} else if(mexp2) {
			if(mexp2->number().isFraction()) {
				if((!eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true))
				|| mbase2->representsNonZero(true)
				|| (eo.warn_about_denominators_assumed_nonzero && eo.assume_denominators_nonzero && !mbase2->representsZero(true) && warn_about_denominators_assumed_nonzero(*mbase2, eo))) {
					erase2 = true;
					mbase1->raise(m_one);
					(*mbase1)[1].number() -= mexp2->number();
					return true;
				}
				return false;
			}
			mexp2->number()--;
			erase1 = true;
			if(mexp2->isOne()) mden.setToChild(1, true);
			return true;
		}
	}
	return false;
}

bool factor1(const MathStructure &mstruct, MathStructure &mnum, MathStructure &mden, const EvaluationOptions &eo) {
	mnum.setUndefined();
	mden.setUndefined();
	if(mstruct.isAddition()) {
		bool b_num = false, b_den = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isMultiplication()) {
				for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
					if(mstruct[i][i2].isPower() && mstruct[i][i2][1].hasNegativeSign()) {
						b_den = true;
						if(b_num) break;
					} else {
						b_num = true;
						if(b_den) break;
					}
				}
			} else if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
				b_den = true;
				if(b_num) break;
			} else {
				b_num = true;
				if(b_den) break;
			}
		}
		if(b_num && b_den) {
			MathStructure *mden_cur = NULL;
			vector<int> multi_index;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mnum.isUndefined()) {
					mnum.transform(STRUCT_ADDITION);
				} else {
					mnum.addChild(m_undefined);
				}
				if(mstruct[i].isMultiplication()) {
					if(!mden_cur) {
						mden_cur = new MathStructure();
						mden_cur->setUndefined();
					}
					for(size_t i2 = 0; i2 < mstruct[i].size(); i2++) {
						if(mstruct[i][i2].isPower() && mstruct[i][i2][1].hasNegativeSign()) {
							if(mden_cur->isUndefined()) {
								if(mstruct[i][i2][1].isMinusOne()) {
									*mden_cur = mstruct[i][i2][0];
								} else if(mstruct[i][i2][1].isNumber()) {
									*mden_cur = mstruct[i][i2];
									(*mden_cur)[1].number().negate();
								} else {
									*mden_cur = mstruct[i][i2];
									(*mden_cur)[1][0].number().negate();
								}
							} else {
								if(mstruct[i][i2][1].isMinusOne()) {
									mden_cur->multiply(mstruct[i][i2][0], true);
								} else if(mstruct[i][i2][1].isNumber()) {
									mden_cur->multiply(mstruct[i][i2], true);
									(*mden_cur)[mden_cur->size() - 1][1].number().negate();
								} else {
									mden_cur->multiply(mstruct[i][i2], true);
									(*mden_cur)[mden_cur->size() - 1][1][0].number().negate();
								}
							}
						} else {
							if(mnum[mnum.size() - 1].isUndefined()) {
								mnum[mnum.size() - 1] = mstruct[i][i2];
							} else {
								mnum[mnum.size() - 1].multiply(mstruct[i][i2], true);
							}
						}
					}
					if(mnum[mnum.size() - 1].isUndefined()) mnum[mnum.size() - 1].set(1, 1);
					if(mden_cur->isUndefined()) {
						multi_index.push_back(-1);
					} else {
						multi_index.push_back(mden.size());
						if(mden.isUndefined()) {
							mden.transform(STRUCT_MULTIPLICATION);
							mden[mden.size() - 1].set_nocopy(*mden_cur);
							mden_cur->unref();
						} else {
							mden.addChild_nocopy(mden_cur);
						}
						mden_cur = NULL;
					}
				} else if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
					multi_index.push_back(mden.size());
					if(mden.isUndefined()) {
						mden.transform(STRUCT_MULTIPLICATION);
					} else {
						mden.addChild(m_undefined);
					}
					if(mstruct[i][1].isMinusOne()) {
						mden[mden.size() - 1] = mstruct[i][0];
					} else {
						mden[mden.size() - 1] = mstruct[i];
						unnegate_sign(mden[mden.size() - 1][1]);
					}
					mnum[mnum.size() - 1].set(1, 1);
				} else {
					multi_index.push_back(-1);
					mnum[mnum.size() - 1] = mstruct[i];
				}
			}			
			for(size_t i = 0; i < mnum.size(); i++) {
				if(multi_index[i] < 0 && mnum[i].isOne()) {
					if(mden.size() == 1) {
						mnum[i] = mden[0];
					} else {
						mnum[i] = mden;
					}
				} else {
					for(size_t i2 = 0; i2 < mden.size(); i2++) {
						if((int) i2 != multi_index[i]) {
							mnum[i].multiply(mden[i2], true);
						}
					}					
				}
				mnum[i].calculatesub(eo, eo, false);
			}
			if(mden.size() == 1) {
				mden.setToChild(1);
			} else {
				mden.calculatesub(eo, eo, false);
			}			
			mnum.calculatesub(eo, eo, false);
		}
	} else if(mstruct.isMultiplication()) {
		bool b_num = false, b_den = false;
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
				b_den = true;
				if(b_num) break;
			} else {
				b_num = true;
				if(b_den) break;
			}
		}
		if(b_den && b_num) {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(mstruct[i].isPower() && mstruct[i][1].hasNegativeSign()) {
					if(mden.isUndefined()) {
						if(mstruct[i][1].isMinusOne()) {
							mden = mstruct[i][0];
						} else {
							mden = mstruct[i];
							unnegate_sign(mden[1]);
						}
					} else {
						if(mstruct[i][1].isMinusOne()) {
							mden.multiply(mstruct[i][0], true);
						} else {
							mden.multiply(mstruct[i], true);
							unnegate_sign(mden[mden.size() - 1][1]);
						}
					}
				} else {
					if(mnum.isUndefined()) {
						mnum = mstruct[i];
					} else {
						mnum.multiply(mstruct[i], true);
					}
				}
			}
		}
		mden.calculatesub(eo, eo, false);
	}
	if(!mnum.isUndefined() && !mden.isUndefined()) {
		while(true) {
			mnum.factorize(eo);
			mnum.evalSort(true);
			if(mnum.isMultiplication()) {
				for(size_t i = 0; i < mnum.size(); ) {
					if(mnum[i].isPower() && mnum[i][1].hasNegativeSign()) {
						if(mnum[i][1].isMinusOne()) {
							mnum[i][0].ref();							
							mden.multiply_nocopy(&mnum[i][0], true);
						} else {
							mnum[i].ref();
							mden.multiply_nocopy(&mnum[i], true);
							unnegate_sign(mden[mden.size() - 1][1]);
						}
						mden.calculateMultiplyLast(eo);
						mnum.delChild(i + 1);
					} else {
						i++;
					}
				}
				if(mnum.size() == 0) mnum.set(1, 1);
				else if(mnum.size() == 1) mnum.setToChild(1);
			}
			mden.factorize(eo);
			mden.evalSort(true);
			bool b = false;
			if(mden.isMultiplication()) {
				for(size_t i = 0; i < mden.size(); ) {
					if(mden[i].isPower() && mden[i][1].hasNegativeSign()) {
						MathStructure *mden_inv = new MathStructure(mden[i]);
						if(mden_inv->calculateInverse(eo)) {
							mnum.multiply_nocopy(mden_inv, true);
							mnum.calculateMultiplyLast(eo);
							mden.delChild(i + 1);
							b = true;
						} else {
							mden_inv->unref();
							i++;
						}
					} else {
						i++;
					}
				}
				if(mden.size() == 0) mden.set(1, 1);
				else if(mden.size() == 1) mden.setToChild(1);
			}
			if(!b) break;
		}
		bool erase1, erase2, b;
		if(mnum.isMultiplication()) {
			for(int i = 0; i < (int) mnum.size(); i++) {
				if(mden.isMultiplication()) {
					for(size_t i2 = 0; i2 < mden.size(); i2++) {
						if(compare_delete(mnum[i], mden[i2], erase1, erase2, eo)) {
							if(erase1) mnum.delChild(i + 1);
							if(erase2) mden.delChild(i2 + 1);
							i--;
							b = true;
							break;
						}
					}
				} else {
					if(compare_delete(mnum[i], mden, erase1, erase2, eo)) {
						if(erase1) mnum.delChild(i + 1);
						if(erase2) mden.set(1, 1);
						b = true;
						break;
					}
				}
			}
		} else if(mden.isMultiplication()) {
			for(size_t i = 0; i < mden.size(); i++) {
				if(compare_delete(mnum, mden[i], erase1, erase2, eo)) {
					if(erase1) mnum.set(1, 1);
					if(erase2) mden.delChild(i + 1);
					b = true;
					break;
				}
			}
		} else {
			if(compare_delete(mnum, mden, erase1, erase2, eo)) {
				if(erase1) mnum.set(1, 1);
				if(erase2) mden.set(1, 1);
				b = true;				
			}
		}
		if(mnum.isMultiplication()) {
			if(mnum.size() == 0) mnum.set(1, 1);
			else if(mnum.size() == 1) mnum.setToChild(1);
		}
		if(mden.isMultiplication()) {
			if(mden.size() == 0) mden.set(1, 1);
			else if(mden.size() == 1) mden.setToChild(1);
		}
		return true;
	}
	return false;
}

bool MathStructure::simplify(const EvaluationOptions &eo, bool unfactorize) {

	if(SIZE == 0) return false;
	
	if(unfactorize) {
		EvaluationOptions eo2 = eo;
		eo2.expand = true;
		calculatesub(eo2, eo2);
	}
	if(!isMultiplication() && !isAddition()) {
		bool b = false;
		if(isComparison()) {
			EvaluationOptions eo2 = eo;
			eo2.assume_denominators_nonzero = false;
			for(size_t i = 0; i < SIZE; i++) {
				b = CHILD(i).simplify(eo2, false);
			}
		} else {
			for(size_t i = 0; i < SIZE; i++) {
				b = CHILD(i).simplify(eo, false);
			}
		}
		return b;
	}
	EvaluationOptions eo2 = eo;
	eo2.expand = true;
	MathStructure mden, mnum;
	if(factor1(*this, mnum, mden, eo)) {
		mnum.calculatesub(eo2, eo2);
		mden.calculatesub(eo2, eo2);
		if(mden.isOne()) {
			set_nocopy(mnum);
			return true;
		}
		 if(mnum.countTotalChildren() + mden.countTotalChildren() + 2 < countTotalChildren()) {
			MathStructure mtest(mnum);
			mtest.divide(mden);
			mtest.calculatesub(eo2, eo2);
			if(mnum.countTotalChildren() + mden.countTotalChildren() + 2 <= mtest.countTotalChildren()) {
				set_nocopy(mnum);
				if(!mden.isOne()) {
					MathStructure *mnew = new MathStructure();
					mnew->set_nocopy(mden);
					divide_nocopy(mnew);
				}
			} else {
				set_nocopy(mtest);
			}
			return true;
		} else {
			MathStructure mtest;
			mtest.set_nocopy(mnum);
			if(!mden.isOne()) {
				MathStructure *mnew = new MathStructure();
				mnew->set_nocopy(mden);
				mtest.divide_nocopy(mnew);
			}
			mtest.calculatesub(eo2, eo2);
			if(mtest.countTotalChildren() < countTotalChildren()) {
				set_nocopy(mtest);
				return true;
			}
		}
	}
	return false;
}

void clean_multiplications(MathStructure &mstruct);
void clean_multiplications(MathStructure &mstruct) {
	if(mstruct.isMultiplication()) {
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isMultiplication()) {
				size_t i2 = 0;
				for(; i2 < mstruct[i + i2].size(); i2++) {
					mstruct[i + i2][i2].ref();
					mstruct.insertChild_nocopy(&mstruct[i + i2][i2], i + i2 + 1);
				}
				mstruct.delChild(i + i2 + 1);
			}
		}
	}
	for(size_t i = 0; i < mstruct.size(); i++) {
		clean_multiplications(mstruct[i]);
	}
}

void MathStructure::calculateUncertaintyPropagation(const EvaluationOptions &eo) {
	if(o_uncertainty) return;	
	MathStructure *new_uncertainty = NULL;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isFunction() && CHILD(i).function() == CALCULATOR->f_uncertainty) {
			MathStructure mstruct = CHILD(i)[1];
			CHILD(i).setToChild(1);
			if(o_uncertainty == NULL) {
				new_uncertainty = new MathStructure();
				new_uncertainty->set(CALCULATOR->f_diff, this, &CHILD(i), NULL);
				new_uncertainty->multiply(mstruct);
			} else {
				MathStructure *uncertainty_add = new MathStructure(CALCULATOR->f_diff, this, &CHILD(i), NULL);
				uncertainty_add->multiply(mstruct);
				new_uncertainty->add_nocopy(uncertainty_add, true);
			}
			new_uncertainty->eval(eo);
			break;
		}
	}
	o_uncertainty = new_uncertainty;
}

MathStructure &MathStructure::eval(const EvaluationOptions &eo) {

	unformat(eo);
	bool found_complex_relations = false;
	if(eo.sync_units && syncUnits(false, &found_complex_relations, false)) {
		unformat(eo);
	}	
	EvaluationOptions feo = eo;
	if(eo.structuring == STRUCTURING_FACTORIZE) feo.structuring = STRUCTURING_SIMPLIFY;
	EvaluationOptions eo2 = eo;
	eo2.structuring = STRUCTURING_NONE;
	eo2.expand = false;
	eo2.test_comparisons = false;

	calculateUncertaintyPropagation(eo);

	if(eo.calculate_functions && calculateFunctions(feo)) {
		unformat(eo);
		if(eo.sync_units && syncUnits(false, &found_complex_relations, true, feo)) {
			unformat(eo);
		}
	}
	if(eo2.approximation == APPROXIMATION_TRY_EXACT) {
		EvaluationOptions eo3 = eo2;
		eo3.approximation = APPROXIMATION_EXACT;
		eo3.split_squares = false;
		eo3.assume_denominators_nonzero = false;
		calculatesub(eo3, feo);
		if(eo.sync_units && syncUnits(false, &found_complex_relations, true, feo)) {
			unformat(eo);
			calculatesub(eo3, feo);
		}
		eo2.approximation = APPROXIMATION_APPROXIMATE;
	}
	calculatesub(eo2, feo);
	if(eo.sync_units && syncUnits(false, &found_complex_relations, true, feo)) {
		unformat(eo);
		calculatesub(eo2, feo);
	}
	if(eo2.isolate_x) {
		eo2.assume_denominators_nonzero = false;
		if(isolate_x(eo2, feo)) {
			eo2.assume_denominators_nonzero = eo.assume_denominators_nonzero;
			calculatesub(eo2, feo);
		} else {
			eo2.assume_denominators_nonzero = eo.assume_denominators_nonzero;
		}
	}
	if(eo.expand || (eo.test_comparisons && !found_complex_relations && containsType(STRUCT_COMPARISON))) {
		eo2.test_comparisons = eo.test_comparisons && !found_complex_relations;
		eo2.expand = eo.expand;
		bool b = eo2.test_comparisons;
		if(!b && isAddition()) {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).containsType(STRUCT_ADDITION, false) == 1) {
					b = true;
					break;
				}
			}
		} else if(!b) {
			b = containsType(STRUCT_ADDITION, false) == 1;
		}
		if(b) {
			calculatesub(eo2, feo);
			if(eo2.isolate_x) {
				eo2.assume_denominators_nonzero = false;
				if(isolate_x(eo2, feo)) {
					eo2.assume_denominators_nonzero = eo.assume_denominators_nonzero;
					calculatesub(eo2, feo);
				} else {
					eo2.assume_denominators_nonzero = eo.assume_denominators_nonzero;
				}
			}
		}
	}
	if(eo2.isolate_x && containsType(STRUCT_COMPARISON) && eo2.assume_denominators_nonzero) {		
		if(try_isolate_x(*this, eo2, feo)) {
			calculatesub(eo2, feo);
		}
	}
	eo2.test_comparisons = eo.test_comparisons;
	if(eo2.sync_units && eo2.sync_complex_unit_relations && found_complex_relations) {		
		if(syncUnits(true, NULL, true, feo)) {
			unformat(eo);
			calculatesub(eo2, feo);
			if(eo2.isolate_x) {
				eo2.assume_denominators_nonzero = false;
				isolate_x(eo2, feo);
				if(isolate_x(eo2, feo)) {
					eo2.assume_denominators_nonzero = eo.assume_denominators_nonzero;
					calculatesub(eo2, feo);
				} else {
					eo2.assume_denominators_nonzero = eo.assume_denominators_nonzero;
				}
			}
		}
	}
	if(eo.structuring == STRUCTURING_SIMPLIFY) {
		simplify(eo2, false);
		clean_multiplications(*this);
	} else if(eo.structuring == STRUCTURING_FACTORIZE) {
		factorize(eo2);
		clean_multiplications(*this);
	}
	return *this;
}

bool factorize_find_multiplier(const MathStructure &mstruct, MathStructure &mnew, MathStructure &factor_mstruct, bool only_units = false) {
	factor_mstruct.set(m_one);
	switch(mstruct.type()) {
		case STRUCT_ADDITION: {
			if(!only_units) {
				bool bfrac = false, bint = true;
				idm1(mstruct, bfrac, bint);
				if(bfrac || bint) {
					Number gcd(1, 1);
					idm2(mstruct, bfrac, bint, gcd);
					if((bint || bfrac) && !gcd.isOne()) {		
						if(bfrac) gcd.recip();
						factor_mstruct.set(gcd);
					}
				}
			}
			if(mstruct.size() > 0) {
				size_t i = 0;
				const MathStructure *cur_mstruct;
				while(true) {
					if(mstruct[0].isMultiplication()) {
						if(i >= mstruct[0].size()) {
							break;
						}
						cur_mstruct = &mstruct[0][i];
					} else {
						cur_mstruct = &mstruct[0];
					}
					if(!cur_mstruct->isNumber() && (!only_units || cur_mstruct->isUnit_exp())) {
						const MathStructure *exp = NULL;
						const MathStructure *bas;
						if(cur_mstruct->isPower() && IS_REAL((*cur_mstruct)[1])) {
							exp = cur_mstruct->exponent();
							bas = cur_mstruct->base();
						} else {
							bas = cur_mstruct;
						}
						bool b = true;
						for(size_t i2 = 1; i2 < mstruct.size(); i2++) {
							b = false;
							size_t i3 = 0;
							const MathStructure *cmp_mstruct;
							while(true) {
								if(mstruct[i2].isMultiplication()) {
									if(i3 >= mstruct[i2].size()) {
										break;
									}
									cmp_mstruct = &mstruct[i2][i3];
								} else {
									cmp_mstruct = &mstruct[i2];
								}
								if(cmp_mstruct->equals(*bas)) {
									if(exp) {
										exp = NULL;
									}
									b = true;
									break;
								} else if(cmp_mstruct->isPower() && cmp_mstruct->base()->equals(*bas)) {
									if(exp) {
										if(IS_REAL((*cmp_mstruct)[1])) {
											if(cmp_mstruct->exponent()->number().isLessThan(exp->number())) {
												exp = cmp_mstruct->exponent();
											}
											b = true;
											break;
										} else {
											exp = NULL;
										}
									} else {
										b = true;
										break;
									}
								}
								if(!mstruct[i2].isMultiplication()) {
									break;
								}
								i3++;
							}
							if(!b) break;
						}
						if(b) {
							if(exp) {
								MathStructure *mstruct = new MathStructure(*bas);
								mstruct->raise(*exp);
								if(factor_mstruct.isOne()) {
									factor_mstruct.set_nocopy(*mstruct);
									mstruct->unref();
								} else {
									factor_mstruct.multiply_nocopy(mstruct, true);
								}
							} else {
								if(factor_mstruct.isOne()) factor_mstruct.set(*bas);
								else factor_mstruct.multiply(*bas, true);
							}
						}
					}
					if(!mstruct[0].isMultiplication()) {
						break;
					}
					i++;
				}
			}
			if(!factor_mstruct.isOne()) {
				if(&mstruct != &mnew) mnew.set(mstruct);
				MathStructure *mfactor;
				size_t i = 0;
				while(true) {
					if(factor_mstruct.isMultiplication()) {
						if(i >= factor_mstruct.size()) break;
						mfactor = &factor_mstruct[i];
					} else {
						mfactor = &factor_mstruct;
					}
					for(size_t i2 = 0; i2 < mnew.size(); i2++) {
						switch(mnew[i2].type()) {
							case STRUCT_NUMBER: {
								if(mfactor->isNumber()) {
									mnew[i2].number() /= mfactor->number();
								}
								break;
							}
							case STRUCT_POWER: {
								if(!IS_REAL(mnew[i2][1])) {
									if(mfactor->isNumber()) {
										mnew[i2].transform(STRUCT_MULTIPLICATION);
										mnew[i2].insertChild(MathStructure(1, 1), 1);
										mnew[i2][0].number() /= mfactor->number();
									} else {
										mnew[i2].set(m_one);
									}
								} else if(mfactor->isNumber()) {
									mnew[i2].transform(STRUCT_MULTIPLICATION);
									mnew[i2].insertChild(MathStructure(1, 1), 1);
									mnew[i2][0].number() /= mfactor->number();
								} else if(mfactor->isPower() && IS_REAL((*mfactor)[1])) {
									if(mfactor->equals(mnew[i2])) {
										mnew[i2].set(m_one);
									} else {
										mnew[i2][1].number() -= mfactor->exponent()->number();
										if(mnew[i2][1].number().isOne()) {
											mnew[i2].setToChild(1, true);
										}
									}
								} else {
									mnew[i2][1].number() -= 1;
									if(mnew[i2][1].number().isOne()) {
										mnew[i2].setToChild(1);
									} else if(mnew[i2][1].number().isZero()) {
										mnew[i2].set(m_one);
									}
								}
								break;
							}
							case STRUCT_MULTIPLICATION: {
								bool b = true;
								if(mfactor->isNumber() && (mnew[i2].size() < 1 || !mnew[i2][0].isNumber())) {
									mnew[i2].insertChild(MathStructure(1, 1), 1);
								}
								for(size_t i3 = 0; i3 < mnew[i2].size() && b; i3++) {
									switch(mnew[i2][i3].type()) {
										case STRUCT_NUMBER: {
											if(mfactor->isNumber()) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3].number() /= mfactor->number();
												}
												b = false;
											}
											break;
										}
										case STRUCT_POWER: {
											if(!IS_REAL(mnew[i2][i3][1])) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
													b = false;
												}
											} else if(mfactor->isPower() && IS_REAL((*mfactor)[1]) && mfactor->base()->equals(mnew[i2][i3][0])) {
												if(mfactor->equals(mnew[i2][i3])) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3][1].number() -= mfactor->exponent()->number();
													if(mnew[i2][i3][1].number().isOne()) {
														MathStructure mstruct2(mnew[i2][i3][0]);
														mnew[i2][i3] = mstruct2;
													} else if(mnew[i2][i3][1].number().isZero()) {
														mnew[i2].delChild(i3 + 1);
													}
												}
												b = false;
											} else if(mfactor->equals(mnew[i2][i3][0])) {
												if(mnew[i2][i3][1].number() == 2) {
													MathStructure mstruct2(mnew[i2][i3][0]);
													mnew[i2][i3] = mstruct2;
												} else if(mnew[i2][i3][1].number().isOne()) {
													mnew[i2].delChild(i3 + 1);
												} else {
													mnew[i2][i3][1].number() -= 1;
												}
												b = false;												
											}	
											break;										
										}
										default: {
											if(mfactor->equals(mnew[i2][i3])) {
												mnew[i2].delChild(i3 + 1);
												b = false;
											}
										}
									}
								}
								if(mnew[i2].size() == 1) {
									MathStructure mstruct2(mnew[i2][0]);
									mnew[i2] = mstruct2;
								}
								break;
							}
							default: {
								if(mfactor->isNumber()) {
									mnew[i2].transform(STRUCT_MULTIPLICATION);
									mnew[i2].insertChild(MathStructure(1, 1), 1);
									mnew[i2][0].number() /= mfactor->number();
								} else {
									mnew[i2].set(m_one);
								}
							}
						}
					}
					if(factor_mstruct.isMultiplication()) {
						i++;
					} else {
						break;
					}
				}
				return true;
			}
		}
		default: {}
	}
	return false;
}

struct sym_desc {
	MathStructure sym;
	Number deg_a;
	Number deg_b;
	Number ldeg_a;
	Number ldeg_b;
	Number max_deg;
	size_t max_lcnops;
	bool operator<(const sym_desc &x) const {
		if (max_deg == x.max_deg) return max_lcnops < x.max_lcnops;
		else return max_deg.isLessThan(x.max_deg);
	}
};
typedef std::vector<sym_desc> sym_desc_vec;

void integer_content(const MathStructure &mpoly, Number &icontent);
void interpolate(const MathStructure &gamma, const Number &xi, const MathStructure &xvar, MathStructure &minterp, const EvaluationOptions &eo);
bool get_first_symbol(const MathStructure &mpoly, MathStructure &xvar);
bool divide_in_z(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, sym_desc_vec::const_iterator var, const EvaluationOptions &eo);
bool prem(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mrem, const EvaluationOptions &eo, bool check_args = true);
bool sr_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, sym_desc_vec::const_iterator var, const EvaluationOptions &eo);
void polynomial_smod(const MathStructure &mpoly, const Number &xi, MathStructure &msmod, const EvaluationOptions &eo, MathStructure *mparent = NULL, size_t index_smod = 0);
bool heur_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, const EvaluationOptions &eo, MathStructure *ca, MathStructure *cb, sym_desc_vec::const_iterator var);
void add_symbol(const MathStructure &mpoly, sym_desc_vec &v);
void collect_symbols(const MathStructure &mpoly, sym_desc_vec &v);
void add_symbol(const MathStructure &mpoly, vector<MathStructure> &v);
void collect_symbols(const MathStructure &mpoly, vector<MathStructure> &v);
void get_symbol_stats(const MathStructure &m1, const MathStructure &m2, sym_desc_vec &v);

bool get_first_symbol(const MathStructure &mpoly, MathStructure &xvar) {
	if(IS_A_SYMBOL(mpoly) || mpoly.isUnit()) {
		xvar = mpoly;
		return true;
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) {
			if(get_first_symbol(mpoly[i], xvar)) return true;
		}
	} else if(mpoly.isPower()) {
		return get_first_symbol(mpoly[0], xvar);
	}
	return false;
}

bool MathStructure::polynomialDivide(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, const EvaluationOptions &eo, bool check_args) {

	mquotient.clear();

	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isZero()) {
		mquotient.clear();
		return true;
	}
	if(mden.isNumber()) {
		mquotient = mnum;
		if(mnum.isNumber()) {
			mquotient.number() /= mden.number();
		} else {
			mquotient.calculateDivide(mden, eo);
		}
		return true;
	} else if(mnum.isNumber()) {
		return false;
	}

	if(mnum == mden) {
		mquotient.set(1, 1);
		return true;
	}

	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}

	MathStructure xvar;
	if(!get_first_symbol(mnum, xvar) && !get_first_symbol(mden, xvar)) return false;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	MathStructure dencoeff;
	mden.coefficient(xvar, dendeg, dencoeff);

	MathStructure mrem(mnum);
	
	while(numdeg.isGreaterThanOrEqualTo(dendeg)) {
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		numdeg -= dendeg;
		if(numcoeff == dencoeff) {
			if(numdeg.isZero()) {
				numcoeff.set(1, 1);
			} else {
				numcoeff = xvar;
				if(!numdeg.isOne()) {
					numcoeff.raise(numdeg);
				}
			}
		} else {
			if(dencoeff.isNumber()) {
				if(numcoeff.isNumber()) {
					numcoeff.number() /= dencoeff.number();
				} else {
					numcoeff.calculateDivide(dencoeff, eo);
				}
			} else {
				MathStructure mcopy(numcoeff);
				if(!MathStructure::polynomialDivide(mcopy, dencoeff, numcoeff, eo, false)) {
					return false;
				}
			}
			if(!numdeg.isZero() && !numcoeff.isZero()) {
				if(numcoeff.isOne()) {
					numcoeff = xvar;
					if(!numdeg.isOne()) {
						numcoeff.raise(numdeg);
					}
				} else {
					numcoeff.multiply(xvar, true);
					if(!numdeg.isOne()) {
						numcoeff[numcoeff.size() - 1].raise(numdeg);
					}
					numcoeff.calculateMultiplyLast(eo);
				}
			}
		}
		if(mquotient.isZero()) mquotient = numcoeff;
		else mquotient.add(numcoeff, true);
		numcoeff.calculateMultiply(mden, eo);
		mrem.calculateSubtract(numcoeff, eo);
		if(mrem.isZero()) return true;
		numdeg = mrem.degree(xvar);
	}
	return false;
}

bool divide_in_z(const MathStructure &mnum, const MathStructure &mden, MathStructure &mquotient, sym_desc_vec::const_iterator var, const EvaluationOptions &eo) {
	mquotient.clear();
	if(mden.isZero()) return false;
	if(mnum.isZero()) return true;
	if(mden.isOne()) {
		mquotient = mnum;
		return true;
	}
	if(mnum.isNumber()) {
		if(!mden.isNumber()) {
			return false;
		}
		mquotient = mnum;
		return mquotient.number().divide(mden.number()) && mquotient.isInteger();
	}
	if(mnum == mden) {
		mquotient.set(1, 1);
		return true;
	}
	
	if(mden.isPower()) {
		MathStructure qbar(mnum);
		for(Number ni(mden[1].number()); ni.isPositive(); ni--) {
			if(!divide_in_z(qbar, mden[0], mquotient, var, eo)) return false;
			qbar = mquotient;
		}
		return true;
	}

	if(mden.isMultiplication()) {
		MathStructure qbar(mnum);
		for(size_t i = 0; i < mden.size(); i++) {
			sym_desc_vec sym_stats;
			get_symbol_stats(mnum, mden[i], sym_stats);
			if(!divide_in_z(qbar, mden[i], mquotient, sym_stats.begin(), eo)) return false;
			qbar = mquotient;
		}
		return true;
	}
	
	const MathStructure &xvar = var->sym;

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	if(dendeg.isGreaterThan(numdeg)) return false;
	MathStructure dencoeff;
	MathStructure mrem(mnum);
	mden.coefficient(xvar, dendeg, dencoeff);
	while(numdeg.isGreaterThanOrEqualTo(dendeg)) {
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		MathStructure term;
		if(!divide_in_z(numcoeff, dencoeff, term, var + 1, eo)) break;
		numdeg -= dendeg;
		if(!numdeg.isZero() && !term.isZero()) {
			if(term.isOne()) {
				term = xvar;		
				if(!numdeg.isOne()) {
					term.raise(numdeg);
				}
			} else {
				term.multiply(xvar, true);		
				if(!numdeg.isOne()) {
					term[term.size() - 1].raise(numdeg);
				}
				term.calculateMultiplyLast(eo);
			}
		}
		if(mquotient.isZero()) {
			mquotient = term;
		} else {
			mquotient.calculateAdd(term, eo);
		}
		term.calculateMultiply(mden, eo);
		mrem.calculateSubtract(term, eo);
		if(mrem.isZero()) {
			return true;
		}
		numdeg = mrem.degree(xvar);
	}
	return false;
}

bool prem(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mrem, const EvaluationOptions &eo, bool check_args) {

	mrem.clear();
	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isNumber()) {
		if(!mden.isNumber()) {
			mrem = mden;
		}
		return true;
	}
	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}

	mrem = mnum;
	MathStructure eb(mden);
	Number rdeg = mrem.degree(xvar);
	Number bdeg = eb.degree(xvar);
	MathStructure blcoeff;
	if(bdeg.isLessThanOrEqualTo(rdeg)) {
		eb.coefficient(xvar, bdeg, blcoeff);
		if(bdeg == 0) {
			eb.clear();
		} else {
			MathStructure mpow(xvar);
			mpow.raise(bdeg);
			POWER_CLEAN(mpow)
			mpow.calculateMultiply(blcoeff, eo);
			eb.calculateSubtract(mpow, eo);
		}
	} else {
		blcoeff.set(1, 1);
	}

	Number delta(rdeg);
	delta -= bdeg;
	delta++;
	int i = 0;
	while(rdeg.isGreaterThanOrEqualTo(bdeg) && !mrem.isZero()) {
		MathStructure rlcoeff;
		mrem.coefficient(xvar, rdeg, rlcoeff);
		MathStructure term(xvar);
		term.raise(rdeg);
		term[1].number() -= bdeg;
		POWER_CLEAN(term)
		term.calculateMultiply(rlcoeff, eo);
		term.calculateMultiply(eb, eo);
		if(rdeg == 0) {
			mrem = term;
			mrem.calculateNegate(eo);
		} else {
			if(!rdeg.isZero()) {
				rlcoeff.multiply(xvar, true);
				if(!rdeg.isOne()) rlcoeff[rlcoeff.size() - 1].raise(rdeg);
				rlcoeff.calculateMultiplyLast(eo);
			}
			mrem.calculateSubtract(rlcoeff, eo);
			mrem.calculateMultiply(blcoeff, eo);
			mrem.calculateSubtract(term, eo);
		}
		rdeg = mrem.degree(xvar);
		i++;
	}
	delta -= i;
	blcoeff.raise(delta);
	mrem.calculateMultiply(blcoeff, eo);
	return true;
}


bool sr_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, sym_desc_vec::const_iterator var, const EvaluationOptions &eo) {

	const MathStructure &xvar = var->sym;

	MathStructure c, d;	
	Number adeg = m1.degree(xvar);
	Number bdeg = m2.degree(xvar);
	Number cdeg, ddeg;
	if(adeg.isGreaterThanOrEqualTo(bdeg)) {
		c = m1;
		d = m2;
		cdeg = adeg;
		ddeg = bdeg;
	} else {
		c = m2;
		d = m1;
		cdeg = bdeg;
		ddeg = adeg;
	}

	MathStructure cont_c, cont_d;
	c.polynomialContent(xvar, cont_c, eo);
	d.polynomialContent(xvar, cont_d, eo);
	MathStructure gamma;
	MathStructure::gcd(cont_c, cont_d, gamma, eo, false);
	if(ddeg.isZero()) {
		mgcd = gamma;
		return true;
	}
	MathStructure prim_c, prim_d;	
	c.polynomialPrimpart(xvar, cont_c, prim_c, eo);
	d.polynomialPrimpart(xvar, cont_d, prim_d, eo);
	c = prim_c;
	d = prim_d;

	MathStructure r;
	MathStructure ri(1, 1);
	MathStructure psi(1, 1);
	Number delta(cdeg);
	delta -= ddeg;

	while(true) {

		prem(c, d, xvar, r, eo, false);
		if(r.isZero()) {
			mgcd = gamma;
			MathStructure mprim;
			d.polynomialPrimpart(xvar, mprim, eo);
			mgcd.calculateMultiply(mprim, eo);
			return true;
		}

		c = d;
		cdeg = ddeg;
		
		MathStructure psi_pow(psi);
		psi_pow.calculateRaise(delta, eo);
		ri.calculateMultiply(psi_pow, eo);
		if(!divide_in_z(r, ri, d, var, eo)) {
			return false;
		}
		ddeg = d.degree(xvar);
		if(ddeg.isZero()) {
			if(r.isNumber()) {
				mgcd = gamma;
			} else {
				r.polynomialPrimpart(xvar, mgcd, eo);
				mgcd.calculateMultiply(gamma, eo);
			}
			return true;
		}

		c.lcoefficient(xvar, ri);
		if(delta.isOne()) {
			psi = ri;
		} else if(!delta.isZero()) {
			MathStructure ri_pow(ri);
			ri_pow.calculateRaise(delta, eo);
			MathStructure psi_pow(psi);
			delta--;
			psi_pow.calculateRaise(delta, eo);
			divide_in_z(ri_pow, psi_pow, psi, var + 1, eo);
		}
		delta = cdeg;
		delta -= ddeg;
		
	}
	
	return false;
	
}

Number MathStructure::maxCoefficient() {
	if(isNumber()) {
		Number nr(o_number);
		nr.abs();
		return nr;
	} else if(isAddition()) {
		Number cur_max(overallCoefficient());
		cur_max.abs();
		for(size_t i = 0; i < SIZE; i++) {
			Number a(CHILD(i).overallCoefficient());
			a.abs();
			if(a.isGreaterThan(cur_max)) cur_max = a;
		}
		return cur_max;
	} else if(isMultiplication()) {
		Number nr(overallCoefficient());
		nr.abs();
		return nr;
	} else {
		return nr_one;
	}
}
void polynomial_smod(const MathStructure &mpoly, const Number &xi, MathStructure &msmod, const EvaluationOptions &eo, MathStructure *mparent, size_t index_smod) {
	if(mpoly.isNumber()) {
		msmod = mpoly;
		msmod.number().smod(xi);
	} else if(mpoly.isAddition()) {
		msmod.clear();
		msmod.setType(STRUCT_ADDITION);
		msmod.resizeVector(mpoly.size(), m_zero);
		for(size_t i = 0; i < mpoly.size(); i++) {
			polynomial_smod(mpoly[i], xi, msmod[i], eo, &msmod, i);
		}
		msmod.calculatesub(eo, eo, false, mparent, index_smod);
	} else if(mpoly.isMultiplication()) {
		msmod = mpoly;
		if(msmod.size() > 0 && msmod[0].isNumber()) {
			if(!msmod[0].number().smod(xi) || msmod[0].isZero()) {
				msmod.clear();
			}
		}
	} else {
		msmod = mpoly;
	}
}

void interpolate(const MathStructure &gamma, const Number &xi, const MathStructure &xvar, MathStructure &minterp, const EvaluationOptions &eo) {
	MathStructure e(gamma);
	Number rxi(xi);
	rxi.recip();
	minterp.clear();
	for(int i = 0; !e.isZero(); i++) {
		MathStructure gi;
		polynomial_smod(e, xi, gi, eo);
		if(minterp.isZero() && !gi.isZero()) {
			minterp = gi;
			if(i != 0) {
				if(minterp.isOne()) {
					minterp = xvar;
					if(i != 1) minterp.raise(i);
				} else {
					minterp.multiply(xvar, true);
					if(i != 1) minterp[minterp.size() - 1].raise(i);
					minterp.calculateMultiplyLast(eo);
				}
			}
		} else if(!gi.isZero()) {
			minterp.add(gi, true);
			if(i != 0) {
				if(minterp[minterp.size() - 1].isOne()) {
					minterp[minterp.size() - 1] = xvar;
					if(i != 1) minterp[minterp.size() - 1].raise(i);
				} else {
					minterp[minterp.size() - 1].multiply(xvar, true);
					if(i != 1) minterp[minterp.size() - 1][minterp[minterp.size() - 1].size() - 1].raise(i);
					minterp[minterp.size() - 1].calculateMultiplyLast(eo);
				}
			}
		}
		if(!gi.isZero()) e.calculateSubtract(gi, eo);
		e.calculateMultiply(rxi, eo);
	}
	minterp.calculatesub(eo, eo, false);
}

bool heur_gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mgcd, const EvaluationOptions &eo, MathStructure *ca, MathStructure *cb, sym_desc_vec::const_iterator var) {

	if (m1.isZero() || m2.isZero())	return false;

	if(m1.isNumber() && m2.isNumber()) {
		mgcd = m1;
		if(!mgcd.number().gcd(m2.number())) mgcd.set(1, 1);
		if(ca) {
			*ca = m1;
			ca->number() /= mgcd.number();
		}
		if(cb) {
			*cb = m2;
			cb->number() /= mgcd.number();
		}
		return true;
	}

//	std::cout << "HEUR 1 a: " << m1.print() << " b: " << m2.print() << std::endl;

	const MathStructure &xvar = var->sym;

	Number gc;
	integer_content(m1, gc);
	Number rgc;
	integer_content(m2, rgc);
//	std::cout << " ic1: " << gc.print() << " ic2: " << rgc.print() << std::endl;
	gc.gcd(rgc);
	rgc = gc;
	rgc.recip();
//	std::cout << "HEUR gc: " << gc.print() << " rgc: " << rgc.print() << std::endl;
	MathStructure p(m1);
	p.calculateMultiply(rgc, eo);
	MathStructure q(m2);
	q.calculateMultiply(rgc, eo);
	Number maxdeg(p.degree(xvar));
	Number maxdeg2(q.degree(xvar));
	if(maxdeg2.isGreaterThan(maxdeg)) maxdeg = maxdeg2;
	
	Number mp(p.maxCoefficient());
	Number mq(q.maxCoefficient());
	Number xi;
	if(mp.isGreaterThan(mq)) {
		xi = mq;
	} else {
		xi = mp;
	}
	xi *= 2;
	xi += 2;
	
//	std::cout << "HEUR 2 xi: " << xi.print() << " p: " << p.print() << " q: " << q.print() << std::endl;

	for(int t = 0; t < 6; t++) {

		if((maxdeg * xi.integerLength()).isGreaterThan(100000)) {
			return false;
		}

		MathStructure cp, cq;
		MathStructure gamma;
		MathStructure psub(p);
		psub.calculateReplace(xvar, xi, eo);
		MathStructure qsub(q);
		qsub.calculateReplace(xvar, xi, eo);

		if(heur_gcd(psub, qsub, gamma, eo, &cp, &cq, var + 1)) {

//			std::cout << "HEUR 4-2 gamma: " << gamma.print() << " xi: " << xi.print() << std::endl;
			interpolate(gamma, xi, xvar, mgcd, eo);
//			std::cout << "HEUR 4-3 gamma: " << gamma.print() << " g: " << mgcd.print() << std::endl;

			Number ig;
			integer_content(mgcd, ig);
			ig.recip();
			mgcd.calculateMultiply(ig, eo); 
//			std::cout << "HEUR 3 g: " << mgcd.print() << " p: " << p.print() << std::endl;

			MathStructure dummy;
			if(divide_in_z(p, mgcd, ca ? *ca : dummy, var, eo) && divide_in_z(q, mgcd, cb ? *cb : dummy, var, eo)) {
				mgcd.calculateMultiply(gc, eo);
				return true;
			}
		}

//		std::cout << "xi: " << xi.print() << std::endl;
		Number xi2(xi);
		xi2.isqrt();
		xi2.isqrt();
		xi *= xi2;
		xi *= 73794;
		xi.iquo(27011);
//		std::cout << "xi: " << xi.print() << std::endl;
		
	}
	
	return false;
	
}

int MathStructure::polynomialUnit(const MathStructure &xvar) const {
	MathStructure coeff;
	lcoefficient(xvar, coeff);	
	if(coeff.hasNegativeSign()) return -1;
	return 1;
}

void integer_content(const MathStructure &mpoly, Number &icontent) {
	if(mpoly.isNumber()) {
		icontent = mpoly.number();
		icontent.abs();
	} else if(mpoly.isAddition()) {
		icontent.clear();
		Number l(1, 1);
		for(size_t i = 0; i < mpoly.size(); i++) {
			if(mpoly[i].isNumber()) {
				if(!icontent.isOne()) {
					Number c = icontent;				
					icontent = mpoly[i].number().numerator();
					icontent.gcd(c);
				}
				Number l2 = l;
				l = mpoly[i].number().denominator();
				l.lcm(l2);
			} else if(mpoly[i].isMultiplication()) {
				if(!icontent.isOne()) {
					Number c = icontent;				
					icontent = mpoly[i].overallCoefficient().numerator();
					icontent.gcd(c);
				}
				Number l2 = l;
				l = mpoly[i].overallCoefficient().denominator();
				l.lcm(l2);
			} else {
				icontent.set(1, 1);
			}
		}
		icontent /= l;
	} else if(mpoly.isMultiplication()) {
		icontent = mpoly.overallCoefficient();
		icontent.abs();
	} else {
		icontent.set(1, 1);
	}
}

bool MathStructure::lcm(const MathStructure &m1, const MathStructure &m2, MathStructure &mlcm, const EvaluationOptions &eo, bool check_args) {
	if(m1.isNumber() && m2.isNumber()) {
		mlcm = m1;
		return mlcm.number().lcm(m2.number());
	}
	if(check_args && (!m1.isRationalPolynomial() || !m2.isRationalPolynomial())) {
		return false;
	}
	MathStructure ca, cb;
	MathStructure::gcd(m1, m2, mlcm, eo, &ca, &cb, false);
	mlcm.calculateMultiply(ca, eo);
	mlcm.calculateMultiply(cb, eo);
	return true;
}

void MathStructure::polynomialContent(const MathStructure &xvar, MathStructure &mcontent, const EvaluationOptions &eo) const {
	if(isZero()) {
		mcontent.clear();
		return;
	}
	if(isNumber()) {
		mcontent = *this;
		mcontent.number().setNegative(false);
		return;
	}
	MathStructure c;
	integer_content(*this, c.number());
	MathStructure r(*this);
	r.calculateDivide(c, eo);
	MathStructure lcoeff;
	r.lcoefficient(xvar, lcoeff);
	if(lcoeff.isInteger()) {
		mcontent = c;
		return;
	}
	Number deg(r.degree(xvar));
	Number ldeg(r.ldegree(xvar));
	if(deg == ldeg) {
		mcontent = lcoeff;
		if(lcoeff.polynomialUnit(xvar) == -1) {
			c.number().negate();
		}
		mcontent.calculateMultiply(c, eo);
		return;
	}
	mcontent.clear();
	MathStructure mtmp, coeff;
	for(Number i(ldeg); i.isLessThanOrEqualTo(deg); i++) {
		coefficient(xvar, i, coeff);
		mtmp = mcontent;
		MathStructure::gcd(coeff, mtmp, mcontent, eo, NULL, NULL, false);
	}
	mcontent.calculateMultiply(c, eo);
}

void MathStructure::polynomialPrimpart(const MathStructure &xvar, MathStructure &mprim, const EvaluationOptions &eo) const {
	if(isZero()) {
		mprim.clear();
		return;
	}
	if(isNumber()) {
		mprim.set(1, 1);
		return;
	}

	MathStructure c;
	polynomialContent(xvar, c, eo);
	if(c.isZero()) {
		mprim.clear();
		return;
	}
	bool b = (polynomialUnit(xvar) == -1);
	if(c.isNumber()) {
		if(b) c.number().negate();
		mprim = *this;
		mprim.calculateDivide(c, eo);
		return;
	}
	if(b) c.calculateNegate(eo);
	MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
}
void MathStructure::polynomialUnitContentPrimpart(const MathStructure &xvar, int &munit, MathStructure &mcontent, MathStructure &mprim, const EvaluationOptions &eo) const {

	if(isZero()) {
		munit = 1;
		mcontent.clear();
		mprim.clear();
		return;
	}

	if(isNumber()) {
		if(o_number.isNegative()) {
			munit = -1;
			mcontent = *this;
			mcontent.number().abs();
		} else {
			munit = 1;
			mcontent = *this;
		}
		mprim.set(1, 1);
		return;
	}

	munit = polynomialUnit(xvar);
	polynomialContent(xvar, mcontent, eo);

	if(mcontent.isZero()) {
		mprim.clear();
		return;
	}
	if(mcontent.isNumber()) {
		mprim = *this;
		if(munit == -1) {
			Number c(mcontent.number());
			c.negate();
			mprim.calculateDivide(c, eo);
		} else {
			mprim.calculateDivide(mcontent, eo);
		}
		return;
	}
	if(munit == -1) {
		MathStructure c(mcontent);
		c.calculateNegate(eo);
		MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
	} else {
		MathStructure::polynomialQuotient(*this, mcontent, xvar, mprim, eo, false);
	}
	
}


void MathStructure::polynomialPrimpart(const MathStructure &xvar, const MathStructure &c, MathStructure &mprim, const EvaluationOptions &eo) const {
	if(isZero() || c.isZero()) {
		mprim.clear();
		return;
	}
	if(isNumber()) {
		mprim.set(1, 1);
		return;
	}
	bool b = (polynomialUnit(xvar) == -1);
	if(c.isNumber()) {
		MathStructure cn(c);
		if(b) cn.number().negate();
		mprim = *this;
		mprim.calculateDivide(cn, eo);
		return;
	}
	if(b) {
		MathStructure cn(c);
		cn.calculateNegate(eo);
		MathStructure::polynomialQuotient(*this, cn, xvar, mprim, eo, false);
	} else {
		MathStructure::polynomialQuotient(*this, c, xvar, mprim, eo, false);
	}
}

const Number& MathStructure::degree(const MathStructure &xvar) const {
	const Number *c = NULL;
	const MathStructure *mcur = NULL;
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			if(!c) {
				c = &nr_one;
			}
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if(!c || c->isLessThan((*mcur)[1].number())) {
				c = &(*mcur)[1].number();
			}
		} else if(mcur->isMultiplication()) {
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				if((*mcur)[i2] == xvar) {
					if(!c) {
						c = &nr_one;
					}
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					if(!c || c->isLessThan((*mcur)[i2][1].number())) {
						c = &(*mcur)[i2][1].number();
					}
				}
			}
		}
		if(!isAddition()) break;
	}
	if(!c) return nr_zero;
	return *c;
}
const Number& MathStructure::ldegree(const MathStructure &xvar) const {
	const Number *c = NULL;
	const MathStructure *mcur = NULL;
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			c = &nr_one;
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if(!c || c->isGreaterThan((*mcur)[1].number())) {
				c = &(*mcur)[1].number();
			}
		} else if(mcur->isMultiplication()) {
			bool b = false;
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				if((*mcur)[i2] == xvar) {
					c = &nr_one;
					b = true;					
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					if(!c || c->isGreaterThan((*mcur)[i2][1].number())) {
						c = &(*mcur)[i2][1].number();
					}
					b = true;
				}
			}
			if(!b) return nr_zero;
		} else {
			return nr_zero;
		}
		if(!isAddition()) break;
	}
	if(!c) return nr_zero;
	return *c;
}
void MathStructure::lcoefficient(const MathStructure &xvar, MathStructure &mcoeff) const {
	coefficient(xvar, degree(xvar), mcoeff);
}
void MathStructure::tcoefficient(const MathStructure &xvar, MathStructure &mcoeff) const {
	coefficient(xvar, ldegree(xvar), mcoeff);
}
void MathStructure::coefficient(const MathStructure &xvar, const Number &pownr, MathStructure &mcoeff) const {
	const MathStructure *mcur = NULL;
	mcoeff.clear();
	for(size_t i = 0; ; i++) {
		if(isAddition()) {
			if(i >= SIZE) break;
			mcur = &CHILD(i);
		} else {
			mcur = this;
		}
		if((*mcur) == xvar) {
			if(pownr.isOne()) {
				if(mcoeff.isZero()) mcoeff.set(1, 1);
				else mcoeff.add(m_one, true);
			}
		} else if(mcur->isPower() && (*mcur)[0] == xvar && (*mcur)[1].isNumber()) {
			if((*mcur)[1].number() == pownr) {
				if(mcoeff.isZero()) mcoeff.set(1, 1);
				else mcoeff.add(m_one, true);
			}
		} else if(mcur->isMultiplication()) {
			bool b = false;
			for(size_t i2 = 0; i2 < mcur->size(); i2++) {
				bool b2 = false;
				if((*mcur)[i2] == xvar) {
					b = true;
					if(pownr.isOne()) b2 = true;
				} else if((*mcur)[i2].isPower() && (*mcur)[i2][0] == xvar && (*mcur)[i2][1].isNumber()) {
					b = true;
					if((*mcur)[i2][1].number() == pownr) b2 = true;
				}
				if(b2) {
					if(mcoeff.isZero()) {
						for(size_t i3 = 0; i3 < mcur->size(); i3++) {
							if(i3 != i2) {
								if(mcoeff.isZero()) mcoeff = (*mcur)[i3];
								else mcoeff.multiply((*mcur)[i3], true);
							}
						}
					} else {
						mcoeff.add(m_zero, true);
						for(size_t i3 = 0; i3 < mcur->size(); i3++) {
							if(i3 != i2) {
								if(mcoeff[mcoeff.size() - 1].isZero()) mcoeff[mcoeff.size() - 1] = (*mcur)[i3];
								else mcoeff[mcoeff.size() - 1].multiply((*mcur)[i3], true);
							}
						}
					}
					break;
				}
			}
			if(!b && pownr.isZero()) {
				if(mcoeff.isZero()) mcoeff = *mcur;
				else mcoeff.add(*mcur, true);
			}
		} else if(pownr.isZero()) {
			if(mcoeff.isZero()) mcoeff = *mcur;
			else mcoeff.add(*mcur, true);
		}
		if(!isAddition()) break;
	}
	mcoeff.evalSort();
}

bool MathStructure::polynomialQuotient(const MathStructure &mnum, const MathStructure &mden, const MathStructure &xvar, MathStructure &mquotient, const EvaluationOptions &eo, bool check_args) {
	mquotient.clear();
	if(mden.isZero()) {
		//division by zero
		return false;
	}
	if(mnum.isZero()) {
		mquotient.clear();
		return true;
	}
	if(mden.isNumber() && mnum.isNumber()) {
		mquotient = mnum;
		if(IS_REAL(mden) && IS_REAL(mnum)) {
			mquotient.number() /= mden.number();
		} else {
			mquotient.calculateDivide(mden, eo);
		}
		return true;
	}
	if(mnum == mden) {
		mquotient.set(1, 1);
		return true;
	}
	if(check_args && (!mnum.isRationalPolynomial() || !mden.isRationalPolynomial())) {
		return false;
	}

	Number numdeg = mnum.degree(xvar);
	Number dendeg = mden.degree(xvar);
	MathStructure dencoeff;
	mden.coefficient(xvar, dendeg, dencoeff);
	MathStructure mrem(mnum);
	while(numdeg.isGreaterThanOrEqualTo(dendeg)) {
		MathStructure numcoeff;
		mrem.coefficient(xvar, numdeg, numcoeff);
		numdeg -= dendeg;
		if(numcoeff == dencoeff) {
			if(numdeg.isZero()) {
				numcoeff.set(1, 1);
			} else {
				numcoeff = xvar;
				if(!numdeg.isOne()) {
					numcoeff.raise(numdeg);
				}
			}
		} else {
			if(dencoeff.isNumber()) {
				if(numcoeff.isNumber()) {
					numcoeff.number() /= dencoeff.number();
				} else {
					numcoeff.calculateDivide(dencoeff, eo);
				}
			} else {
				MathStructure mcopy(numcoeff);
				if(!MathStructure::polynomialDivide(mcopy, dencoeff, numcoeff, eo, false)) {
					return false;
				}
			}
			if(!numdeg.isZero() && !numcoeff.isZero()) {
				if(numcoeff.isOne()) {
					numcoeff = xvar;
					if(!numdeg.isOne()) {
						numcoeff.raise(numdeg);
					}
				} else {
					numcoeff.multiply(xvar, true);
					if(!numdeg.isOne()) {
						numcoeff[numcoeff.size() - 1].raise(numdeg);
					}
					numcoeff.calculateMultiplyLast(eo);
				}
			}
		}
		if(mquotient.isZero()) mquotient = numcoeff;
		else mquotient.calculateAdd(numcoeff, eo);
		numcoeff.calculateMultiply(mden, eo);
		mrem.calculateSubtract(numcoeff, eo);
		if(mrem.isZero()) break;
		numdeg = mrem.degree(xvar);
	}
	return true;

}

void add_symbol(const MathStructure &mpoly, sym_desc_vec &v) {
	sym_desc_vec::const_iterator it = v.begin(), itend = v.end();
	while (it != itend) {
		if(it->sym == mpoly) return;
		++it;
	}
	sym_desc d;
	d.sym = mpoly;
	v.push_back(d);
}

void collect_symbols(const MathStructure &mpoly, sym_desc_vec &v) {
	if(IS_A_SYMBOL(mpoly) || mpoly.isUnit()) {
		add_symbol(mpoly, v);
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) collect_symbols(mpoly[i], v);
	} else if(mpoly.isPower()) {
		collect_symbols(mpoly[0], v);
	}
}

void add_symbol(const MathStructure &mpoly, vector<MathStructure> &v) {
	vector<MathStructure>::const_iterator it = v.begin(), itend = v.end();
	while (it != itend) {
		if(*it == mpoly) return;
		++it;
	}
	v.push_back(mpoly);
}

void collect_symbols(const MathStructure &mpoly, vector<MathStructure> &v) {
	if(IS_A_SYMBOL(mpoly)) {
		add_symbol(mpoly, v);
	} else if(mpoly.isAddition() || mpoly.isMultiplication()) {
		for(size_t i = 0; i < mpoly.size(); i++) collect_symbols(mpoly[i], v);
	} else if(mpoly.isPower()) {
		collect_symbols(mpoly[0], v);
	}
}

void get_symbol_stats(const MathStructure &m1, const MathStructure &m2, sym_desc_vec &v) {
	collect_symbols(m1, v);
	collect_symbols(m2, v);
	sym_desc_vec::iterator it = v.begin(), itend = v.end();
	while (it != itend) {
		it->deg_a = m1.degree(it->sym);
		it->deg_b = m2.degree(it->sym);
		if(it->deg_a.isGreaterThan(it->deg_b)) {
			it->max_deg = it->deg_a;
		} else {
			it->max_deg = it->deg_b;
		}
		it->ldeg_a = m1.ldegree(it->sym);
		it->ldeg_b = m2.ldegree(it->sym);
		MathStructure mcoeff;
		m1.lcoefficient(it->sym, mcoeff);
		it->max_lcnops = mcoeff.size();
		m2.lcoefficient(it->sym, mcoeff);
		if(mcoeff.size() > it->max_lcnops) it->max_lcnops = mcoeff.size();
		++it;
	}
	std::sort(v.begin(), v.end());

}

bool MathStructure::gcd(const MathStructure &m1, const MathStructure &m2, MathStructure &mresult, const EvaluationOptions &eo, MathStructure *ca, MathStructure *cb, bool check_args) {
	if(m1.isOne() || m2.isOne()) {
		if(ca) *ca = m1;
		if(cb) *cb = m2;
		mresult.set(1, 1);
		return true;
	}
	if(m1.isNumber() && m2.isNumber()) {
		mresult = m1;
		if(!mresult.number().gcd(m2.number())) mresult.set(1, 1);
		if(ca || cb) {
			if(mresult.isZero()) {
				if(ca) ca->clear();
				if(cb) cb->clear();
			} else {
				if(ca) {
					*ca = m1;
					ca->number() /= mresult.number();
				}
				if(cb) {
					*cb = m2;
					cb->number() /= mresult.number();
				}
			}
		}
		return true;
	}
	if(m1 == m2) {
		if(ca) ca->set(1, 1);
		if(cb) cb->set(1, 1);
		mresult = m1;
		return true;
	}
	if(m1.isZero()) {
		if(ca) ca->clear();
		if(cb) cb->set(1, 0);
		mresult = m2;
		return true;
	}
	if(m2.isZero()) {
		if(ca) ca->set(1, 0);
		if(cb) cb->clear();
		mresult = m1;
		return true;
	}
	if(check_args && (!m1.isRationalPolynomial() || !m2.isRationalPolynomial())) {
		return false;
	}
	if(m1.isMultiplication()) {
		if(m2.isMultiplication() && m2.size() > m1.size())
			goto factored_2;
factored_1:
		mresult.clear();
		mresult.setType(STRUCT_MULTIPLICATION);
		MathStructure acc_ca;
		acc_ca.setType(STRUCT_MULTIPLICATION);
		MathStructure part_2(m2);
		MathStructure part_ca, part_cb;
		for (size_t i = 0; i < m1.size(); i++) {			
			mresult.addChild(m_zero);
			MathStructure::gcd(m1[i], part_2, mresult[i], eo, &part_ca, &part_cb, false);
			acc_ca.addChild(part_ca);
			part_2 = part_cb;
		}
		mresult.calculatesub(eo, eo, false);
		if(ca) {
			*ca = acc_ca;
			ca->calculatesub(eo, eo, false);
		}
		if(cb) *cb = part_2;
		return true;
	} else if(m2.isMultiplication()) {
		if(m1.isMultiplication() && m1.size() > m2.size())
			goto factored_1;
factored_2:
		mresult.clear();
		mresult.setType(STRUCT_MULTIPLICATION);
		MathStructure acc_cb;
		acc_cb.setType(STRUCT_MULTIPLICATION);
		MathStructure part_1(m1);
		MathStructure part_ca, part_cb;
		for(size_t i = 0; i < m2.size(); i++) {			
			mresult.addChild(m_zero);
			MathStructure::gcd(part_1, m2[i], mresult[i], eo, &part_ca, &part_cb, false);
			acc_cb.addChild(part_cb);
			part_1 = part_ca;
		}
		mresult.calculatesub(eo, eo, false);
		if(ca) *ca = part_1;
		if(cb) {
			*cb = acc_cb;
			cb->calculatesub(eo, eo, false);
		}
		return true;
	}
	if(m1.isPower()) {
		MathStructure p(m1[0]);
		if(m2.isPower()) {
			if(m1[0] == m2[0]) {
				if(m1[1].number().isLessThan(m2[1].number())) {
					if(ca) ca->set(1, 1);
					if(cb) {
						*cb = m2;
						(*cb)[1].number() -= m1[1].number();
						POWER_CLEAN((*cb))
					}
					mresult = m1;
					return true;
				} else {
					if(ca) {
						*ca = m1;
						(*ca)[1].number() -= m2[1].number();
						POWER_CLEAN((*ca))
					}
					if(cb) cb->set(1, 1);
					mresult = m2;
					return true;
				}
			} else {
				MathStructure p_co, pb_co;
				MathStructure p_gcd;
				if(!MathStructure::gcd(m1[0], m2[0], p_gcd, eo, &p_co, &pb_co, false)) return false;
				if(p_gcd.isOne()) {
					if(ca) *ca = m1;
					if(cb) *cb = m2;
					mresult.set(1, 1);
					return true;
				} else {
					if(m1[1].number().isLessThan(m2[1].number())) {
						mresult = p_gcd;
						mresult.calculateRaise(m1[1], eo);
						mresult.multiply(m_zero, true);
						p_co.calculateRaise(m1[1], eo);
						pb_co.calculateRaise(m2[1], eo);
						p_gcd.raise(m2[1]);
						p_gcd[1].number() -= m1[1].number();
						p_gcd.calculateRaiseExponent(eo);
						p_gcd.calculateMultiply(pb_co, eo);
						bool b = MathStructure::gcd(p_co, p_gcd, mresult[mresult.size() - 1], eo, ca, cb, false);
						mresult.calculateMultiplyLast(eo);
						return b;
					} else {
						mresult = p_gcd;
						mresult.calculateRaise(m2[1], eo);
						mresult.multiply(m_zero, true);
						p_co.calculateRaise(m1[1], eo);
						pb_co.calculateRaise(m2[1], eo);
						p_gcd.raise(m1[1]);
						p_gcd[1].number() -= m2[1].number();
						p_gcd.calculateRaiseExponent(eo);
						p_gcd.calculateMultiply(p_co, eo);
						bool b = MathStructure::gcd(p_gcd, pb_co, mresult[mresult.size() - 1], eo, ca, cb, false);
						mresult.calculateMultiplyLast(eo);
						return b;
					}
				}
			}
		} else {
			if(m1[0] == m2) {
				if(ca) {
					*ca = m1;
					(*ca)[1].number()--;
					POWER_CLEAN((*ca))
				}
				if(cb) cb->set(1, 1);
				mresult = m2;
				return true;
			}
			MathStructure p_co, bpart_co;
			MathStructure p_gcd;
			if(!MathStructure::gcd(m1[0], m2, p_gcd, eo, &p_co, &bpart_co, false)) return false;
			if(p_gcd.isOne()) {
				if(ca) *ca = m1;
				if(cb) *cb = m2;
				mresult.set(1, 1);
				return true;
			} else {
				mresult = p_gcd;
				mresult.multiply(m_zero, true);
				p_co.calculateRaise(m1[1], eo);
				p_gcd.raise(m1[1]);
				p_gcd[1].number()--;
				p_gcd.calculateRaiseExponent(eo);
				p_gcd.calculateMultiply(p_co, eo);
				bool b = MathStructure::gcd(p_gcd, bpart_co, mresult[mresult.size() - 1], eo, ca, cb, false);
				mresult.calculateMultiplyLast(eo);
				return b;
			}
		}
	} else if(m2.isPower()) {
		if(m2[0] == m1) {
			if(ca) ca->set(1, 1);
			if(cb) {
				*cb = m2;
				(*cb)[1].number()--;
				POWER_CLEAN((*cb))
			}
			mresult = m1;
			return true;
		}
		MathStructure p_co, apart_co;
		MathStructure p_gcd;
		if(!MathStructure::gcd(m1, m2[0], p_gcd, eo, &apart_co, &p_co, false)) return false;
		if(p_gcd.isOne()) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
			mresult.set(1, 1);
			return true;
		} else {
			mresult = p_gcd;
			mresult.multiply(m_zero, true);
			p_co.calculateRaise(m2[1], eo);
			p_gcd.raise(m2[1]);
			p_gcd[1].number()--;
			p_gcd.calculateRaiseExponent(eo);
			p_gcd.calculateMultiply(p_co, eo);
			bool b = MathStructure::gcd(apart_co, p_gcd, mresult[mresult.size() - 1], eo, ca, cb, false);
			mresult.calculateMultiplyLast(eo);
			return b;
		}
	}
	if(IS_A_SYMBOL(m1) || m1.isUnit()) {
		MathStructure bex(m2);
		bex.calculateReplace(m1, m_zero, eo);
		if(!bex.isZero()) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
			mresult.set(1, 1);
			return true;
		}
	}

	if(IS_A_SYMBOL(m2) || m2.isUnit()) {
		MathStructure aex(m1);
		aex.calculateReplace(m2, m_zero, eo);
		if(!aex.isZero()) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
			mresult.set(1, 1);
			return true;
		}
	}
	
	sym_desc_vec sym_stats;
	get_symbol_stats(m1, m2, sym_stats);

	sym_desc_vec::const_iterator var = sym_stats.begin();
	const MathStructure &xvar = var->sym;

	Number ldeg_a(var->ldeg_a);
	Number ldeg_b(var->ldeg_b);
	Number min_ldeg;
	if(ldeg_a.isLessThan(ldeg_b)) {
		min_ldeg = ldeg_a;
	} else {
		min_ldeg = ldeg_b;
	}

	if(min_ldeg.isPositive()) {
		MathStructure aex(m1), bex(m2);
		MathStructure common(xvar);
		if(!min_ldeg.isOne()) common.raise(min_ldeg);
		aex.calculateDivide(common, eo);
		bex.calculateDivide(common, eo);
		MathStructure::gcd(aex, bex, mresult, eo, ca, cb, false);
		mresult.calculateMultiply(common, eo);
		return true;
	}
	if(var->deg_a.isZero()) {
		if(cb) {
			MathStructure c, mprim;
			int u;
			m2.polynomialUnitContentPrimpart(xvar, u, c, mprim, eo);
			MathStructure::gcd(m1, c, mresult, eo, ca, cb, false);
			if(u == -1) {
				cb->calculateNegate(eo);
			}
			cb->calculateMultiply(mprim, eo);
		} else {
			MathStructure c;
			m2.polynomialContent(xvar, c, eo);
			MathStructure::gcd(m1, c, mresult, eo, ca, cb, false);
		}
		return true;
	} else if(var->deg_b.isZero()) {
		if(ca) {
			MathStructure c, mprim;
			int u;
			m1.polynomialUnitContentPrimpart(xvar, u, c, mprim, eo);
			MathStructure::gcd(c, m2, mresult, eo, ca, cb, false);
			if(u == -1) {
				ca->calculateNegate(eo);
			}
			ca->calculateMultiply(mprim, eo);
		} else {
			MathStructure c;
			m1.polynomialContent(xvar, c, eo);
			MathStructure::gcd(c, m2, mresult, eo, ca, cb, false);
		}
		return true;
	}
	if(!heur_gcd(m1, m2, mresult, eo, ca, cb, var)) {
		sr_gcd(m1, m2, mresult, var, eo);
		if(mresult.isOne()) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
		} else {
			if(ca) {
				MathStructure::polynomialDivide(m1, mresult, *ca, eo, false);
			}
			if(cb) {
				MathStructure::polynomialDivide(m2, mresult, *cb, eo, false);
			}
		}
	} else {
		if(mresult.isOne()) {
			if(ca) *ca = m1;
			if(cb) *cb = m2;
		}
	}
	return true;
}

bool sqrfree_differentiate(const MathStructure &mpoly, const MathStructure &x_var, MathStructure &mdiff, const EvaluationOptions &eo) {
	if(mpoly == x_var) {
		mdiff.set(1, 1);
		return true;
	}
	switch(mpoly.type()) {
		case STRUCT_ADDITION: {
			mdiff.clear();
			mdiff.setType(STRUCT_ADDITION);
			for(size_t i = 0; i < mpoly.size(); i++) {
				mdiff.addChild(m_zero);
				if(!sqrfree_differentiate(mpoly[i], x_var, mdiff[i], eo)) return false;
			}
			mdiff.calculatesub(eo, eo, false);
			break;
		}
		case STRUCT_VARIABLE: {}
		case STRUCT_FUNCTION: {}
		case STRUCT_SYMBOLIC: {}
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			mdiff.clear();
			break;
		}
		case STRUCT_POWER: {
			if(mpoly[0] == x_var) {
				mdiff = mpoly[1];
				mdiff.multiply(x_var);
				if(!mpoly[1].number().isTwo()) {
					mdiff[1].raise(mpoly[1]);
					mdiff[1][1].number()--;
				}
				mdiff.evalSort(true);
			} else {
				mdiff.clear();
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mpoly.size() < 1) {
				mdiff.clear();
				break;
			} else if(mpoly.size() < 2) {
				return sqrfree_differentiate(mpoly[0], x_var, mdiff, eo);
			}
			mdiff.clear();
			for(size_t i = 0; i < mpoly.size(); i++) {
				if(mpoly[i] == x_var) {
					if(mpoly.size() == 2) {
						if(i == 0) mdiff = mpoly[1];
						else mdiff = mpoly[0];
					} else {
						mdiff.setType(STRUCT_MULTIPLICATION);
						for(size_t i2 = 0; i2 < mpoly.size(); i2++) {
							if(i2 != i) {
								mdiff.addChild(mpoly[i2]);
							}
						}
					}
					break;
				} else if(mpoly[i].isPower() && mpoly[i][0] == x_var) {
					mdiff = mpoly;
					if(mpoly[i][1].number().isTwo()) {
						mdiff[i].setToChild(1);
					} else {
						mdiff[i][1].number()--;
					}
					if(mdiff[0].isNumber()) {
						mdiff[0].number() *= mpoly[i][1].number();
					} else {
						mdiff.insertChild(mpoly[i][1].number(), 1);
					}
					mdiff.evalSort();
					break;
				}
			}
			break;
		}
		default: {
			return false;
		}	
	}
	return true;
}


bool sqrfree_yun(const MathStructure &a, const MathStructure &xvar, MathStructure &factors, const EvaluationOptions &eo) {
	factors.clearVector();
//	std::cout << "A: " << a.print() << std::endl;
	MathStructure w(a);
	MathStructure z;
	if(!sqrfree_differentiate(a, xvar, z, eo)) {
//		printf("A Failed\n");
		return false;
	}
//	std::cout << "Z: " << z.print() << std::endl;
	MathStructure g;
	if(!MathStructure::gcd(w, z, g, eo)) {
//		printf("B Failed\n");
		return false;
	}
//	std::cout << "G: " << g.print() << std::endl;
	if(g.isOne()) {
		factors.addChild(a);
		return true;
	}
	MathStructure y;
	MathStructure tmp;
	do {	
		tmp = w;
		if(!MathStructure::polynomialQuotient(tmp, g, xvar, w, eo)) {
//			printf("D Failed\n");
			return false;
		}
//		std::cout << "W2: " << w.print() << std::endl;
		if(!MathStructure::polynomialQuotient(z, g, xvar, y, eo)) {
//			printf("E Failed\n");
			return false;
		}
//		std::cout << "Y2: " << y.print() << std::endl;
		if(!sqrfree_differentiate(w, xvar, tmp, eo)) {
//			printf("F Failed\n");
			return false;
		}
		z = y;
//		std::cout << "Z3: " << z.print() << std::endl;
//		std::cout << "TMP: " << tmp.print() << std::endl;
		z.calculateSubtract(tmp, eo);
//		std::cout << "Z2: " << z.print() << std::endl;
		if(!MathStructure::gcd(w, z, g, eo)) {
//			printf("H Failed\n");
			return false;
		}
//		std::cout << "G2: " << g.print() << std::endl;
		factors.addChild(g);
	} while (!z.isZero());
	return true;
}

void lcmcoeff(const MathStructure &e, const Number &l, Number &nlcm);
void lcmcoeff(const MathStructure &e, const Number &l, Number &nlcm) {
	if(e.isNumber() && e.number().isRational()) {
		nlcm = e.number().denominator();
		nlcm.lcm(l);
	} else if(e.isAddition()) {
		nlcm.set(1, 1);
		for(size_t i = 0; i < e.size(); i++) {
			Number c(nlcm);
			lcmcoeff(e[i], c, nlcm);
		}
		nlcm.lcm(l);
	} else if(e.isMultiplication()) {
		nlcm.set(1, 1);
		for(size_t i = 0; i < e.size(); i++) {
			Number c(nlcm);
			lcmcoeff(e[i], nr_one, c);
			nlcm *= c;
		}
		nlcm.lcm(l);
	} else if(e.isPower()) {
		if(IS_A_SYMBOL(e[0]) || e[0].isUnit()) {
			nlcm = l;
		} else {
			lcmcoeff(e[0], l, nlcm);
			nlcm ^= e[1].number();
		}
	} else {
		nlcm = l;
	}
}

void lcm_of_coefficients_denominators(const MathStructure &e, Number &nlcm) {
	return lcmcoeff(e, nr_one, nlcm);
}

void multiply_lcm(const MathStructure &e, const Number &lcm, MathStructure &mmul, const EvaluationOptions &eo);
void multiply_lcm(const MathStructure &e, const Number &lcm, MathStructure &mmul, const EvaluationOptions &eo) {
	if(e.isMultiplication()) {
		Number lcm_accum(1, 1);
		mmul.clear();
		for(size_t i = 0; i < e.size(); i++) {
			Number op_lcm;
			lcmcoeff(e[i], nr_one, op_lcm);
			if(mmul.isZero()) {
				multiply_lcm(e[i], op_lcm, mmul, eo);
				if(mmul.isOne()) mmul.clear();
			} else {
				mmul.multiply(m_one, true);
				multiply_lcm(e[i], op_lcm, mmul[mmul.size() - 1], eo);
				if(mmul[mmul.size() - 1].isOne()) {
					mmul.delChild(i + 1);
					if(mmul.size() == 1) mmul.setToChild(1);
				}
			}
			lcm_accum *= op_lcm;
		}
		Number lcm2(lcm);
		lcm2 /= lcm_accum;
		if(mmul.isZero()) {
			mmul = lcm2;
		} else if(!lcm2.isOne()) {
			if(mmul.size() > 0 && mmul[0].isNumber()) {
				mmul[0].number() *= lcm2;
			} else {				
				mmul.multiply(lcm2, true);
			}
		}
		mmul.evalSort();
	} else if(e.isAddition()) {
		mmul.clear();
		for (size_t i = 0; i < e.size(); i++) {
			if(mmul.isZero()) {
				multiply_lcm(e[i], lcm, mmul, eo);
			} else {
				mmul.add(m_zero, true);
				multiply_lcm(e[i], lcm, mmul[mmul.size() - 1], eo);
			}
		}
		mmul.evalSort();
	} else if(e.isPower()) {
		if(IS_A_SYMBOL(e[0]) || e[0].isUnit()) {
			mmul = e;
			if(!lcm.isOne()) {
				mmul *= lcm;
				mmul.evalSort();
			}
		} else {
			mmul = e;
			Number lcm_exp = e[1].number();
			lcm_exp.recip();
			multiply_lcm(e[0], lcm ^ lcm_exp, mmul[0], eo);
			if(mmul[0] != e[0]) {
				mmul.calculatesub(eo, eo, false);
			}
		}
	} else if(e.isNumber()) {
		mmul = e;
		mmul.number() *= lcm;	
	} else if(IS_A_SYMBOL(e) || e.isUnit()) {
		mmul = e;
		if(!lcm.isOne()) {
			mmul *= lcm;
			mmul.evalSort();
		}
	} else {
		mmul = e;
		if(!lcm.isOne()) {
			mmul.calculateMultiply(lcm, eo);
			mmul.evalSort();
		}
	}
}

//from GiNaC
void sqrfree(MathStructure &mpoly, const vector<MathStructure> &symbols, const EvaluationOptions &eo);
void sqrfree(MathStructure &mpoly, const EvaluationOptions &eo) {
	vector<MathStructure> symbols;
	collect_symbols(mpoly, symbols);
	sqrfree(mpoly, symbols, eo);
}
void sqrfree(MathStructure &mpoly, const vector<MathStructure> &symbols, const EvaluationOptions &eo) {

	EvaluationOptions eo2 = eo;
	eo2.assume_denominators_nonzero = true;
	eo2.warn_about_denominators_assumed_nonzero = false;
	eo2.reduce_divisions = true;
	eo2.keep_zero_units = false;
	eo2.expand = true;

	if(mpoly.size() == 0) {
		return;
	}
	if(symbols.empty()) return;
	
	const MathStructure &xvar = symbols[0];

	Number nlcm;
	lcm_of_coefficients_denominators(mpoly, nlcm);
	
	MathStructure tmp;
	multiply_lcm(mpoly, nlcm, tmp, eo2);

	MathStructure factors;
	if(!sqrfree_yun(tmp, xvar, factors, eo2)) {
		factors.clearVector();
		factors.addChild(tmp);
	}

	vector<MathStructure> newsymbols;
	for(size_t i = 1; i < symbols.size(); i++) {
		newsymbols.push_back(symbols[i]);
	}

	if(newsymbols.size() > 0) {
		for(size_t i = 0; i < factors.size(); i++) {
			sqrfree(factors[i], newsymbols, eo);
		}
	}

	mpoly.set(1, 1);
	for(size_t i = 0; i < factors.size(); i++) {
		if(!factors[i].isOne()) {
			if(mpoly.isOne()) {
				mpoly = factors[i];
				if(i != 0) mpoly.raise(MathStructure(i + 1, 1));
			} else {
				mpoly.multiply(factors[i], true);
				mpoly[mpoly.size() - 1].raise(MathStructure(i + 1, 1));
			}
		}
	}

	if(mpoly.isZero()) {
		CALCULATOR->error(true, "mpoly is zero: %s. %s", tmp.print().c_str(), _("This is a bug. Please report it."), NULL);
		return;
	}
	MathStructure mquo;
	MathStructure mpoly_expand(mpoly);
	EvaluationOptions eo3 = eo;
	eo3.expand = true;
	mpoly_expand.calculatesub(eo3, eo3);

	MathStructure::polynomialQuotient(tmp, mpoly_expand, xvar, mquo, eo2);
	if(mquo.isZero()) {
		CALCULATOR->error(true, "quo is zero: %s. %s", tmp.print().c_str(), _("This is a bug. Please report it."), NULL);
		return;
	}

	if(newsymbols.size() > 0) {
		sqrfree(mquo, newsymbols, eo);
	}
	if(!mquo.isOne()) {
		mpoly.multiply(mquo, true);
	}
	if(!nlcm.isOne()) {
		nlcm.recip();
		mpoly.multiply(nlcm, true);
	}

	eo3.expand = false;
	mpoly.calculatesub(eo3, eo3, false);

}

bool MathStructure::integerFactorize() {
	if(!isNumber()) return false;
	vector<Number> factors;
	if(!o_number.factorize(factors)) return false;
	if(factors.size() == 1) return true;
	clear(true);
	bool b_pow = false;
	Number *lastnr = NULL;
	for(size_t i = 0; i < factors.size(); i++) {
		if(lastnr && factors[i] == *lastnr) {
			if(!b_pow) {
				LAST.raise(m_one);
				b_pow = true;
			}
			LAST[1].number()++;
		} else {
			APPEND(factors[i]);
		}
		lastnr = &factors[i];
	}
	m_type = STRUCT_MULTIPLICATION;
	return true;
}
bool MathStructure::factorize(const EvaluationOptions &eo) {

	MathStructure mden, mnum;
	if(containsDivision() && factor1(*this, mnum, mden, eo)) {
		set_nocopy(mnum);
		if(!mden.isOne()) {
			MathStructure *mnew = new MathStructure();
			mnew->set_nocopy(mden);
			divide_nocopy(mnew);
		}
		return true;
	}
	
	evalSort(true);
	if(isAddition() && isRationalPolynomial()) {
		MathStructure mcopy(*this);
		sqrfree(*this, eo);
		if(!equals(mcopy)) {
			MathStructure mcopy2(*this);
			EvaluationOptions eo2 = eo;
			eo2.expand = true;
			mcopy2.calculatesub(eo2, eo2);
			if(mcopy != mcopy2) {
				/*for(size_t i = 0; i < mcopy.size(); i++) {
					std::cout << "MCOPY " << i << ": " << mcopy[i].print() << std::endl;
					for(size_t i2 = 0; i2 < mcopy[i].size(); i2++) {
						std::cout << "MCOPY " << i << "-" << i2 << ": " << mcopy[i][i2].print() << std::endl;
					}
				}
				for(size_t i = 0; i < mcopy2.size(); i++) {
					std::cout << "MCOPY2 " << i << ": " << mcopy2[i].print() << std::endl;
					for(size_t i2 = 0; i2 < mcopy2[i].size(); i2++) {
						std::cout << "MCOPY2 " << i << "-" << i2 << ": " << mcopy2[i][i2].print() << std::endl;
					}
				}
				std::cout << "WRONG! this: " << print() << " copy: " << mcopy.print() << " copy2: " << mcopy2.print() << std::endl;*/
				CALCULATOR->error(true, "factorized result is wrong: %s. %s", mcopy.print().c_str(), _("This is a bug. Please report it."), NULL);
				set(mcopy);
			}
		}
		if(!isAddition()) {
			factorize(eo);
			return true;
		}
	}

	switch(type()) {
		case STRUCT_ADDITION: {
			if(SIZE <= 3 && SIZE > 1) {
				MathStructure *xvar = NULL;
				Number nr2(1, 1);
				if(CHILD(0).isPower() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo()) {
					xvar = &CHILD(0)[0];
				} else if(CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isNumber()) {
					if(CHILD(0)[1].isPower()) {
						if(CHILD(0)[1][0].size() == 0 && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isTwo()) {
							xvar = &CHILD(0)[1][0];
							nr2.set(CHILD(0)[0].number());
						}
					}
				}
				if(xvar) {
					bool factorable = false;
					Number nr1, nr0;
					if(SIZE == 2 && CHILD(1).isNumber()) {
						factorable = true;
						nr0 = CHILD(1).number();
					} else if(SIZE == 3 && CHILD(2).isNumber()) {
						nr0 = CHILD(2).number();
						if(CHILD(1).isMultiplication()) {
							if(CHILD(1).size() == 2 && CHILD(1)[0].isNumber() && xvar->equals(CHILD(1)[1])) {
								nr1 = CHILD(1)[0].number();
								factorable = true;
							}
						} else if(xvar->equals(CHILD(1))) {
							nr1.set(1, 1);
							factorable = true;
						}
					}
					if(factorable) {
						Number nr4ac(4, 1);
						nr4ac *= nr2;
						nr4ac *= nr0;
						Number nrtwo(2, 1);
						Number nr2a(2, 1);
						nr2a *= nr2;
						Number sqrtb24ac(nr1);
						sqrtb24ac.raise(nrtwo);
						sqrtb24ac -= nr4ac;
						/*if(sqrtb24ac.isNegative()) {
							factorable = false;
						}
						MathStructure mstructb24(sqrtb24ac);
						if(factorable) {
							nrtwo.set(1, 2);							
							mstructb24.number().raise(nrtwo);							
							if(!sqrtb24ac.isApproximate() && mstructb24.number().isApproximate()) {
								factorable = false;
							}
						}
						if(factorable) {*/
						if(!sqrtb24ac.isNegative()) {
							nrtwo.set(1, 2);
							MathStructure mstructb24(sqrtb24ac);
							if(eo.approximation == APPROXIMATION_EXACT && !sqrtb24ac.isApproximate()) {
								sqrtb24ac.raise(nrtwo);
								if(sqrtb24ac.isApproximate()) {
									mstructb24.raise(nrtwo);
								} else {
									mstructb24.set(sqrtb24ac);
								}
							} else {
								mstructb24.number().raise(nrtwo);
							}
							MathStructure m1(nr1), m2(nr1);
							Number mul1(1, 1), mul2(1, 1);
							if(mstructb24.isNumber()) {
								m1.number() += mstructb24.number();
								m1.number() /= nr2a;
								if(m1.number().isRational() && !m1.number().isInteger()) {
									mul1 = m1.number().denominator();
									m1.number() *= mul1;
								}
								m2.number() -= mstructb24.number();
								m2.number() /= nr2a;
								if(m2.number().isRational() && !m2.number().isInteger()) {
									mul2 = m2.number().denominator();
									m2.number() *= mul2;
								}
							} else {
								m1.calculateAdd(mstructb24, eo);
								m1.calculateDivide(nr2a, eo);
								if(m1.isNumber()) {
									if(m1.number().isRational() && !m1.number().isInteger()) {
										mul1 = m1.number().denominator();
										m1.number() *= mul1;
									}
								} else {
									bool bint = false, bfrac = false;
									idm1(m1, bfrac, bint);
									if(bfrac) {
										idm2(m1, bfrac, bint, mul1);
										idm3(m1, mul1, true);
									}
								}
								m2.calculateSubtract(mstructb24, eo);
								m2.calculateDivide(nr2a, eo);
								if(m2.isNumber()) {
									if(m2.number().isRational() && !m2.number().isInteger()) {
										mul2 = m2.number().denominator();
										m2.number() *= mul2;
									}
								} else {
									bool bint = false, bfrac = false;
									idm1(m2, bfrac, bint);
									if(bfrac) {
										idm2(m2, bfrac, bint, mul2);
										idm3(m2, mul2, true);
									}
								}
							}
							nr2 /= mul1;
							nr2 /= mul2;
							if(m1 == m2 && mul1 == mul2) {
								MathStructure xvar2(*xvar);
								if(!mul1.isOne()) xvar2 *= mul1;
								set(m1);
								add(xvar2, true);
								raise(MathStructure(2, 1));
								if(!nr2.isOne()) {
									multiply(nr2);
								}
							} else {				
								m1.add(*xvar, true);
								if(!mul1.isOne()) m1[m1.size() - 1] *= mul1;
								m2.add(*xvar, true);
								if(!mul2.isOne()) m2[m2.size() - 1] *= mul2;
								clear(true);
								m_type = STRUCT_MULTIPLICATION;
								if(!nr2.isOne()) {
									APPEND_NEW(nr2);
								}
								APPEND(m1);
								APPEND(m2);
							}
							EvaluationOptions eo2 = eo;
							eo2.expand = false;
							calculatesub(eo2, eo2, false);
							return true;
						}
					}
				}
			}
			MathStructure *factor_mstruct = new MathStructure(1, 1);
			MathStructure mnew;
			if(factorize_find_multiplier(*this, mnew, *factor_mstruct)) {
				mnew.factorize(eo);
				clear(true);
				m_type = STRUCT_MULTIPLICATION;
				APPEND_REF(factor_mstruct);
				APPEND(mnew);
				EvaluationOptions eo2 = eo;
				eo2.expand = false;
				calculatesub(eo2, eo2, false);
				factor_mstruct->unref();
				return true;
			}
			factor_mstruct->unref();
			if(SIZE > 1 && CHILD(SIZE - 1).isNumber() && CHILD(SIZE - 1).number().isInteger()) {
				MathStructure *xvar = NULL;
				Number qnr(1, 1);
				int degree = 1;
				bool overflow = false;
				int qcof = 1;
				if(CHILD(0).isPower() && CHILD(0)[0].size() == 0 && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isInteger() && CHILD(0)[1].number().isPositive()) {
					xvar = &CHILD(0)[0];
					degree = CHILD(0)[1].number().intValue(&overflow);
				} else if(CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isNumber() && CHILD(0)[0].number().isInteger()) {
					if(CHILD(0)[1].isPower()) {
						if(CHILD(0)[1][0].size() == 0 && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isInteger() && CHILD(0)[1][1].number().isPositive()) {
							xvar = &CHILD(0)[1][0];
							qcof = CHILD(0)[0].number().intValue(&overflow);
							if(!overflow) {
								if(qcof < 0) qcof = -qcof;
								degree = CHILD(0)[1][1].number().intValue(&overflow);
							}
						}
					}
				}
				int pcof = 1;
				if(!overflow) {
					pcof = CHILD(SIZE - 1).number().intValue(&overflow);
					if(pcof < 0) pcof = -pcof;
				}
				if(xvar && !overflow && degree <= 1000 && degree > 2) {
					bool b = true;
					for(size_t i = 1; b && i < SIZE - 1; i++) {
						switch(CHILD(i).type()) {
							case STRUCT_NUMBER: {
								b = false;
								break;
							}
							case STRUCT_POWER: {
								if(!CHILD(i)[1].isNumber() || !xvar->equals(CHILD(i)[0]) || !CHILD(i)[1].number().isInteger() || !CHILD(i)[1].number().isPositive()) {
									b = false;
								}
								break;
							}
							case STRUCT_MULTIPLICATION: {
								if(!CHILD(i).size() == 2 || !CHILD(i)[0].isNumber()) {
									b = false;
								} else if(CHILD(i)[1].isPower()) {
									if(!CHILD(i)[1][1].isNumber() || !xvar->equals(CHILD(i)[1][0]) || !CHILD(i)[1][1].number().isInteger() || !CHILD(i)[1][1].number().isPositive()) {
										b = false;
									}
								} else if(!xvar->equals(CHILD(i)[1])) {
									b = false;
								}
								break;
							}
							default: {
								if(!xvar->equals(CHILD(i))) {
									b = false;
								}
							}
						}
					}
					if(b) {
						vector<Number> factors;
						factors.resize(degree + 1, Number());
						factors[0] = CHILD(SIZE - 1).number();
						vector<int> ps;
						vector<int> qs;
						vector<Number> zeroes;
						int curdeg = 1, prevdeg = 0;
						for(size_t i = 0; b && i < SIZE - 1; i++) {
							switch(CHILD(i).type()) {
								case STRUCT_POWER: {
									curdeg = CHILD(i)[1].number().intValue(&overflow);
									if(curdeg == prevdeg || curdeg > degree || prevdeg > 0 && curdeg > prevdeg || overflow) {
										b = false;
									} else {
										factors[curdeg].set(1, 1);
									}
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(CHILD(i)[1].isPower()) {
										curdeg = CHILD(i)[1][1].number().intValue(&overflow);
									} else {
										curdeg = 1;
									}
									if(curdeg == prevdeg || curdeg > degree || prevdeg > 0 && curdeg > prevdeg || overflow) {
										b = false;
									} else {
										factors[curdeg] = CHILD(i)[0].number();
									}
									break;
								}
								default: {
									curdeg = 1;
									factors[curdeg].set(1, 1);
								}
							}
							prevdeg = curdeg;
						}
						while(b && degree > 2) {
							for(int i = 1; i <= 1000; i++) {
								if(i > pcof) break;
								if(pcof % i == 0) ps.push_back(i);
							}
							for(int i = 1; i <= 1000; i++) {
								if(i > qcof) break;
								if(qcof % i == 0) qs.push_back(i);
							}
							Number itest;
							int i2;
							size_t pi = 0, qi = 0;
							Number nrtest(ps[0], qs[0]);
							while(true) {
								itest.clear(); i2 = degree;
								while(true) {
									itest += factors[i2];
									if(i2 == 0) break;
									itest *= nrtest;
									i2--;
								}
								if(itest.isZero()) {
									break;
								}
								if(nrtest.isPositive()) {
									nrtest.negate();
								} else {
									qi++;
									if(qi == qs.size()) {
										qi = 0;
										pi++;
										if(pi == ps.size()) {
											break;
										}
									}
									nrtest.set(ps[pi], qs[qi]);
								}
							}
							if(itest.isZero()) {
								itest.clear(); i2 = degree;
								Number ntmp(factors[i2]);
								for(; i2 > 0; i2--) {
									itest += ntmp;
									ntmp = factors[i2 - 1];
									factors[i2 - 1] = itest;
									itest *= nrtest;
								}
								degree--;
								nrtest.negate();
								zeroes.push_back(nrtest);
								if(degree == 2) {
									break;
								}
								qcof = factors[degree].intValue(&overflow);
								if(!overflow) {
									if(qcof < 0) qcof = -qcof;
									pcof = factors[0].intValue(&overflow);
									if(!overflow) {
										if(pcof < 0) pcof = -pcof;
									}
								}
								if(overflow) {
									break;
								}
							} else {
								break;
							}
							ps.clear();
							qs.clear();
						}
						if(zeroes.size() > 0) {
							MathStructure mleft;
							MathStructure mtmp;
							MathStructure *mcur;
							for(int i = degree; i >= 0; i--) {
								if(!factors[i].isZero()) {
									if(mleft.isZero()) {
										mcur = &mleft;
									} else {
										mleft.add(m_zero, true);
										mcur = &mleft[mleft.size() - 1];
									}
									if(i > 1) {
										if(!factors[i].isOne()) {
											mcur->multiply(*xvar);
											(*mcur)[0].set(factors[i]);
											mcur = &(*mcur)[1];
										} else {
											mcur->set(*xvar);
										}
										mtmp.set(i, 1);
										mcur->raise(mtmp);
									} else if(i == 1) {
										if(!factors[i].isOne()) {
											mcur->multiply(*xvar);
											(*mcur)[0].set(factors[i]);
										} else {
											mcur->set(*xvar);
										}
									} else {
										mcur->set(factors[i]);
									}
								}
							}
							mleft.factorize(eo);
							vector<int> powers;
							vector<size_t> powers_i;
							int dupsfound = 0;
							for(size_t i = 0; i < zeroes.size() - 1; i++) {
								while(i + 1 < zeroes.size() && zeroes[i] == zeroes[i + 1]) {
									dupsfound++;
									zeroes.erase(zeroes.begin() + (i + 1));
								}
								if(dupsfound > 0) {
									powers_i.push_back(i);
									powers.push_back(dupsfound + 1);
									dupsfound = 0;
								}
							}
							MathStructure xvar2(*xvar);
							Number *nrmul;
							if(mleft.isMultiplication()) {
								set(mleft);
								evalSort();
								if(CHILD(0).isNumber()) {
									nrmul = &CHILD(0).number();
								} else if(CHILD(0).isMultiplication() && CHILD(0).size() > 0 && CHILD(0)[0].isNumber()) {
									nrmul = &CHILD(0)[0].number();
								} else {
									PREPEND(m_one);
									nrmul = &CHILD(0).number();
								}
							} else {
								clear(true);
								m_type = STRUCT_MULTIPLICATION;
								APPEND(m_one);
								APPEND(mleft);
								nrmul = &CHILD(0).number();
							}
							size_t pi = 0;
							for(size_t i = 0; i < zeroes.size(); i++) {
								if(zeroes[i].isInteger()) {
									APPEND(xvar2);
								} else {
									APPEND(m_zero);
								}
								mcur = &CHILD(SIZE - 1);
								if(pi < powers_i.size() && powers_i[pi] == i) {
									mcur->raise(MathStructure(powers[pi], 1));
									mcur = &(*mcur)[0];
									if(zeroes[i].isInteger()) {
										mcur->add(zeroes[i]);
									} else {
										Number nr(zeroes[i].denominator());
										mcur->add(zeroes[i].numerator());
										(*mcur)[0] *= xvar2;
										(*mcur)[0][0].number() = nr;
										nr.raise(powers[pi]);
										nrmul->divide(nr);										
									}
									pi++;
								} else {
									if(zeroes[i].isInteger()) {
										mcur->add(zeroes[i]);
									} else {
										nrmul->divide(zeroes[i].denominator());
										mcur->add(zeroes[i].numerator());
										(*mcur)[0] *= xvar2;
										(*mcur)[0][0].number() = zeroes[i].denominator();
									}
								}
							}
							if(CHILD(0).isNumber() && CHILD(0).number().isOne()) {
								ERASE(0);
							} else if(CHILD(0).isMultiplication() && CHILD(0).size() > 0 && CHILD(0)[0].isNumber() && CHILD(0)[0].number().isOne()) {
								if(CHILD(0).size() == 1) {
									ERASE(0);
								} else if(CHILD(0).size() == 2) {
									CHILD(0).setToChild(2, true);
								} else {
									CHILD(0).delChild(1);
								}
							}
							evalSort(true);
							Number dupspow;
							for(size_t i = 0; i < SIZE - 1; i++) {
								mcur = NULL;
								if(CHILD(i).isPower()) {
									if(CHILD(i)[0].isAddition() && CHILD(i)[1].isNumber()) {
										mcur = &CHILD(i)[0];
									}
								} else if(CHILD(i).isAddition()) {
									mcur = &CHILD(i);
								}
								while(mcur && i + 1 < SIZE) {
									if(CHILD(i + 1).isPower()) {
										if(CHILD(i + 1)[0].isAddition() && CHILD(i + 1)[1].isNumber() && mcur->equals(CHILD(i + 1)[0])) {
											dupspow += CHILD(i + 1)[1].number();
										} else {
											mcur = NULL;
										}
									} else if(CHILD(i + 1).isAddition() && mcur->equals(CHILD(i + 1))) {
										dupspow++;
									} else {
										mcur = NULL;
									}
									if(mcur) {
										ERASE(i + 1);
									}
								}
								if(!dupspow.isZero()) {
									if(CHILD(i).isPower()) {
										CHILD(i)[1].number() += dupspow;
									} else {
										dupspow++;
										CHILD(i) ^= dupspow;
									}
									dupspow.clear();
								}
							}
							if(SIZE == 1) {
								setToChild(1, true);
							} else {
								EvaluationOptions eo2 = eo;
								eo2.expand = false;
								calculatesub(eo2, eo2, false);
							}
							return true;
						}
					}
				}
			}
			//x^2-y^2=(x+y)(x-y)
			if(SIZE == 2 && CHILD(0).isPower() && CHILD(0)[1].isNumber() && CHILD(0)[1].number().isTwo() && CHILD(1).isMultiplication() && CHILD(1).size() == 2 && CHILD(1)[0].isMinusOne() && CHILD(1)[1].isPower() && CHILD(1)[1][1].isNumber() && CHILD(1)[1][1].number().isTwo()) {
				if(CHILD(0)[0].representsNonMatrix() && CHILD(1)[1][0].representsNonMatrix()) {
					CHILD(0).setToChild(1, true);
					CHILD(1)[1].setToChild(1, true);
					MathStructure *m2 = new MathStructure(*this);
					(*m2)[1].setToChild(2, true);
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					calculatesub(eo2, eo2, false);
					multiply_nocopy(m2, true);
					return true;
				}
			}
			//-x^2+y^2=(x+y)(-y+x)
			if(SIZE == 2 && CHILD(1).isPower() && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isTwo() && CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isMinusOne() && CHILD(0)[1].isPower() && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number().isTwo()) {
				if(CHILD(1)[0].representsNonMatrix() && CHILD(0)[1][0].representsNonMatrix()) {
					CHILD(1).setToChild(1, true);
					CHILD(0)[1].setToChild(1, true);
					MathStructure *m2 = new MathStructure(*this);
					(*m2)[0].setToChild(2, true);
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					calculatesub(eo2, eo2, false);
					multiply_nocopy(m2, true);
					return true;
				}
			}
			//x^3-y^3=(x-y)(x^2+xy+y^2)
			if(SIZE == 2 && CHILD(0).isPower() && CHILD(0)[1].isNumber() && CHILD(0)[1].number() == 3 && CHILD(1).isMultiplication() && CHILD(1).size() == 2 && CHILD(1)[0].isMinusOne() && CHILD(1)[1].isPower() && CHILD(1)[1][1].isNumber() && CHILD(1)[1][1].number() == 3) {
				if(CHILD(0)[0].representsNonMatrix() && CHILD(1)[1][0].representsNonMatrix()) {
					MathStructure *m2 = new MathStructure(*this);
					(*m2)[0].setToChild(1, true);
					(*m2)[1][1].setToChild(1, true);
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					m2->calculatesub(eo2, eo2, false);					
					CHILD(0)[1].set(2, 1, 0, true);
					CHILD(1).setToChild(2, true);
					CHILD(1)[1].set(2, 1, 0, true);
					MathStructure *m3 = new MathStructure(CHILD(0)[0]);
					m3->calculateMultiply(CHILD(1)[0], eo2);
					add_nocopy(m3, true);
					calculatesub(eo2, eo2, false);
					multiply_nocopy(m2, true);
					return true;
				}
			}
			//-x^3+y^3=(-x+y)(x^2+xy+y^2)
			if(SIZE == 2 && CHILD(1).isPower() && CHILD(1)[1].isNumber() && CHILD(1)[1].number() == 3 && CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0].isMinusOne() && CHILD(0)[1].isPower() && CHILD(0)[1][1].isNumber() && CHILD(0)[1][1].number() == 3) {
				if(CHILD(1)[0].representsNonMatrix() && CHILD(0)[1][0].representsNonMatrix()) {
					MathStructure *m2 = new MathStructure(*this);
					(*m2)[1].setToChild(1, true);
					(*m2)[0][1].setToChild(1, true);
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					m2->calculatesub(eo2, eo2, false);					
					CHILD(1)[1].set(2, 1, 0, true);
					CHILD(0).setToChild(2, true);
					CHILD(0)[1].set(2, 1, 0, true);
					MathStructure *m3 = new MathStructure(CHILD(0)[0]);
					m3->calculateMultiply(CHILD(1)[0], eo2);
					add_nocopy(m3, true);
					calculatesub(eo2, eo2, false);
					multiply_nocopy(m2, true);
					return true;
				}
			}			
		}
		default: {
			bool b = false;
			if(isComparison()) {
				EvaluationOptions eo2 = eo;
				eo2.assume_denominators_nonzero = false;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).factorize(eo2)) {
						CHILD_UPDATED(i);
						b = true;
					}
				}
			} else {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).factorize(eo)) {
						CHILD_UPDATED(i);
						b = true;
					}
				}
				if(b) {
					EvaluationOptions eo2 = eo;
					eo2.expand = false;
					calculatesub(eo2, eo2, false);
				}
			}			
		}
	}
	return false;
}

void MathStructure::swapChildren(size_t index1, size_t index2) {
	if(index1 > 0 && index2 > 0 && index1 <= SIZE && index2 <= SIZE) {
		SWAP_CHILDREN(index1 - 1, index2 - 1)
	}
}
void MathStructure::childToFront(size_t index) {
	if(index > 0 && index <= SIZE) {
		CHILD_TO_FRONT(index - 1)
	}
}
void MathStructure::addChild(const MathStructure &o) {
	APPEND(o);
}
void MathStructure::addChild_nocopy(MathStructure *o) {
	APPEND_POINTER(o);
}
void MathStructure::delChild(size_t index) {
	if(index > 0 && index <= SIZE) {
		ERASE(index - 1);
	}
}
void MathStructure::insertChild(const MathStructure &o, size_t index) {
	if(index > 0 && index <= v_subs.size()) {
		v_order.insert(v_order.begin() + (index - 1), v_subs.size());
		v_subs.push_back(new MathStructure(o));
		CHILD_UPDATED(index - 1);
	} else {
		addChild(o);
	}
}
void MathStructure::insertChild_nocopy(MathStructure *o, size_t index) {
	if(index > 0 && index <= v_subs.size()) {
		v_order.insert(v_order.begin() + (index - 1), v_subs.size());
		v_subs.push_back(o);
		CHILD_UPDATED(index - 1);
	} else {
		addChild_nocopy(o);
	}
}
void MathStructure::setChild(const MathStructure &o, size_t index) {
	if(index > 0 && index <= SIZE) {
		CHILD(index - 1) = o;
		CHILD_UPDATED(index - 1);
	}
}
void MathStructure::setChild_nocopy(MathStructure *o, size_t index) {
	if(index > 0 && index <= SIZE) {
		v_subs[v_order[index - 1]]->unref();
		v_subs[v_order[index - 1]] = o;
		CHILD_UPDATED(index - 1);
	}
}
const MathStructure *MathStructure::getChild(size_t index) const {
	if(index > 0 && index <= v_order.size()) {
		return &CHILD(index - 1);
	}
	return NULL;
}
MathStructure *MathStructure::getChild(size_t index) {
	if(index > 0 && index <= v_order.size()) {
		return &CHILD(index - 1);
	}
	return NULL;
}
size_t MathStructure::countChildren() const {
	return SIZE;
}
size_t MathStructure::size() const {
	return SIZE;
}
const MathStructure *MathStructure::base() const {
	if(m_type == STRUCT_POWER && SIZE >= 1) {
		return &CHILD(0);
	}
	return NULL;
}
const MathStructure *MathStructure::exponent() const {
	if(m_type == STRUCT_POWER && SIZE >= 2) {
		return &CHILD(1);
	}
	return NULL;
}
MathStructure *MathStructure::base() {
	if(m_type == STRUCT_POWER && SIZE >= 1) {
		return &CHILD(0);
	}
	return NULL;
}
MathStructure *MathStructure::exponent() {
	if(m_type == STRUCT_POWER && SIZE >= 2) {
		return &CHILD(1);
	}
	return NULL;
}

StructureType MathStructure::type() const {
	return m_type;
}
void MathStructure::unformat(const EvaluationOptions &eo) {
	for(size_t i = 0; i < SIZE; i++) {
		CHILD(i).unformat(eo);
	}
	switch(m_type) {
		case STRUCT_INVERSE: {
			APPEND(m_minus_one);
			m_type = STRUCT_POWER;	
		}
		case STRUCT_NEGATE: {
			PREPEND(m_minus_one);
			m_type = STRUCT_MULTIPLICATION;
		}
		case STRUCT_DIVISION: {
			CHILD(1).raise(m_minus_one);
			m_type = STRUCT_MULTIPLICATION;
		}
		case STRUCT_UNIT: {
			if(o_prefix && !eo.keep_prefixes) {
				if(o_prefix == CALCULATOR->decimal_null_prefix || o_prefix == CALCULATOR->binary_null_prefix) {
					o_prefix = NULL;
				} else {
					Unit *u = o_unit;
					Prefix *p = o_prefix;
					set(p->value());
					multiply(u);
				}
			}
			b_plural = false;
		}
		default: {}
	}
}

void idm1(const MathStructure &mnum, bool &bfrac, bool &bint) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if((!bfrac || bint) && mnum.number().isRational()) {
				if(!mnum.number().isInteger()) {
					bint = false;
					bfrac = true;
				}
			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if((!bfrac || bint) && mnum.size() > 0 && mnum[0].isNumber() && mnum[0].number().isRational()) {
				if(!mnum[0].number().isInteger()) {
					bint = false;
					bfrac = true;
				}
				
			} else {
				bint = false;
			}
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size() && (!bfrac || bint); i++) {
				idm1(mnum[i], bfrac, bint);
			}
			break;
		}
		default: {
			bint = false;
		}
	}
}
void idm2(const MathStructure &mnum, bool &bfrac, bool &bint, Number &nr) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			if(mnum.number().isRational()) {
				if(mnum.number().isInteger()) {
					if(bint) {
						if(mnum.number().isOne()) {
							bint = false;
						} else if(nr.isOne()) {
							nr = mnum.number();
						} else if(nr != mnum.number()) {
							nr.gcd(mnum.number());
							if(nr.isOne()) bint = false;
						}
					}
				} else {
					if(nr.isOne()) {
						nr = mnum.number().denominator();
					} else {
						Number nden(mnum.number().denominator());
						if(nr != nden) {
							Number ngcd(nden);
							ngcd.gcd(nr);
							nden /= ngcd;
							nr *= nden;
						}
					}
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber() && mnum[0].number().isRational()) {
				if(mnum[0].number().isInteger()) {
					if(bint) {
						if(mnum[0].number().isOne()) {
							bint = false;
						} else if(nr.isOne()) {
							nr = mnum[0].number();
						} else if(nr != mnum[0].number()) {
							nr.gcd(mnum[0].number());
							if(nr.isOne()) bint = false;
						}
					}
				} else {
					if(nr.isOne()) {
						nr = mnum[0].number().denominator();
					} else {
						Number nden(mnum[0].number().denominator());
						if(nr != nden) {
							Number ngcd(nden);
							ngcd.gcd(nr);
							nden /= ngcd;
							nr *= nden;
						}
					}
				}
			}
			break;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < mnum.size() && (bfrac || bint); i++) {
				idm2(mnum[i], bfrac, bint, nr);
			}
			break;
		}
		default: {}
	}
}
void idm3(MathStructure &mnum, Number &nr, bool expand) {
	switch(mnum.type()) {
		case STRUCT_NUMBER: {
			mnum.number() *= nr;
			mnum.numberUpdated();
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(mnum.size() > 0 && mnum[0].isNumber()) {
				mnum[0].number() *= nr;
				if(mnum[0].number().isOne() && mnum.size() != 1) {
					mnum.delChild(1);
					if(mnum.size() == 1) mnum.setToChild(1, true);
				}
			} else {
				mnum.insertChild(nr, 1);
			}
			break;
		}
		case STRUCT_ADDITION: {
			if(expand) {
				for(size_t i = 0; i < mnum.size(); i++) {
					idm3(mnum[i], nr, true);
				}
				break;
			}
		}
		default: {
			mnum.transform(STRUCT_MULTIPLICATION);
			mnum.insertChild(nr, 1);
		}
	}
}

bool is_unit_multiexp(const MathStructure &mstruct) {
	if(mstruct.isUnit_exp()) return true;
	if(mstruct.isMultiplication()) {
		for(size_t i3 = 0; i3 < mstruct.size(); i3++) {
			if(!mstruct[i3].isUnit_exp()) {
				return false;
				break;
			}
		}
		return true;
	}
	if(mstruct.isPower() && mstruct[0].isMultiplication()) {
		for(size_t i3 = 0; i3 < mstruct[0].size(); i3++) {
			if(!mstruct[0][i3].isUnit_exp()) {
				return false;
				break;
			}
		}
		return true;
	}
	return false;
}
bool MathStructure::improve_division_multipliers(const PrintOptions &po) {
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			size_t inum = 0, iden = 0;
			bool bfrac = false, bint = true, bdiv = false, bnonunitdiv = false;
			size_t index1 = 0, index2 = 0;
			bool dofrac = !po.negative_exponents;
			for(size_t i2 = 0; i2 < SIZE; i2++) {
				if(CHILD(i2).isPower() && CHILD(i2)[1].isMinusOne()) {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2)[0])) {
						if(iden == 0) index1 = i2;
						iden++;
						bdiv = true;
						if(!CHILD(i2)[0].isUnit()) bnonunitdiv = true;
						if(CHILD(i2)[0].containsType(STRUCT_ADDITION)) {
							dofrac = true;
						}
					}
				} else if(!bdiv && !po.negative_exponents && CHILD(i2).isPower() && CHILD(i2)[1].hasNegativeSign()) {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2)[0])) {
						if(!bdiv) index1 = i2;
						bdiv = true;
						if(!CHILD(i2)[0].isUnit()) bnonunitdiv = true;
					}
				} else {
					if(!po.place_units_separately || !is_unit_multiexp(CHILD(i2))) {
						if(inum == 0) index2 = i2;
						inum++;
					}
				}
			}
			if(!bdiv || !bnonunitdiv) break;
			if(iden > 1 && !po.negative_exponents) {
				size_t i2 = index1 + 1;
				while(i2 < SIZE) {
					if(CHILD(i2).isPower() && CHILD(i2)[1].isMinusOne()) {
						CHILD(index1)[0].multiply(CHILD(i2)[0], true);
						ERASE(i2);
					} else {
						i2++;
					}
				}
				iden = 1;
			}
			if(bint) bint = inum > 0 && iden == 1;
			if(inum > 0) idm1(CHILD(index2), bfrac, bint);
			if(iden > 0) idm1(CHILD(index1)[0], bfrac, bint);
			bool b = false;
			if(!dofrac) bfrac = false;
			if(bint || bfrac) {
				Number nr(1, 1);
				if(inum > 0) idm2(CHILD(index2), bfrac, bint, nr);
				if(iden > 0) idm2(CHILD(index1)[0], bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					if(bint) {
						nr.recip();
					}
					if(inum == 0) {
						PREPEND(MathStructure(nr));
					} else if(inum > 1 && !CHILD(index2).isNumber()) {
						idm3(*this, nr, !po.allow_factorization);
					} else {
						idm3(CHILD(index2), nr, !po.allow_factorization);
					}
					if(iden > 0) {
						idm3(CHILD(index1)[0], nr, !po.allow_factorization);
					} else {
						MathStructure mstruct(nr);
						mstruct.raise(m_minus_one);
						insertChild(mstruct, index1);
					}
					b = true;
				}
			}
			/*if(!po.negative_exponents && SIZE == 2 && CHILD(1).isAddition()) {
				MathStructure factor_mstruct(1, 1);
				if(factorize_find_multiplier(CHILD(1), CHILD(1), factor_mstruct)) {
					transform(STRUCT_MULTIPLICATION);
					PREPEND(factor_mstruct);
				}
			}*/
			return b;
		}
		case STRUCT_DIVISION: {
			bool bint = true, bfrac = false;
			idm1(CHILD(0), bfrac, bint);
			idm1(CHILD(1), bfrac, bint);
			if(bint || bfrac) {
				Number nr(1, 1);
				idm2(CHILD(0), bfrac, bint, nr);
				idm2(CHILD(1), bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					if(bint) {
						nr.recip();
					}
					idm3(CHILD(0), nr, !po.allow_factorization);
					idm3(CHILD(1), nr, !po.allow_factorization);
					return true;
				}
			}
			break;
		}
		case STRUCT_INVERSE: {
			bool bint = false, bfrac = false;
			idm1(CHILD(0), bfrac, bint);
			if(bint || bfrac) {
				Number nr(1, 1);
				idm2(CHILD(0), bfrac, bint, nr);
				if((bint || bfrac) && !nr.isOne()) {
					setToChild(1, true);
					idm3(*this, nr, !po.allow_factorization);
					transform_nocopy(STRUCT_DIVISION, new MathStructure(nr));
					SWAP_CHILDREN(0, 1);
					return true;
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(1).isMinusOne()) {
				bool bint = false, bfrac = false;
				idm1(CHILD(0), bfrac, bint);
				if(bint || bfrac) {
					Number nr(1, 1);
					idm2(CHILD(0), bfrac, bint, nr);
					if((bint || bfrac) && !nr.isOne()) {
						idm3(CHILD(0), nr, !po.allow_factorization);
						transform(STRUCT_MULTIPLICATION);
						PREPEND(MathStructure(nr));
						return true;
					}
				}
				break;
			}
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).improve_division_multipliers()) b = true;
			}
			return b;
		}
	}
	return false;
}

void MathStructure::setPrefixes(const PrintOptions &po, MathStructure *parent, size_t pindex) {
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			size_t i = SIZE, im = 0;
			bool b_im = false;
			for(size_t i2 = 0; i2 < SIZE; i2++) {
				if(CHILD(i2).isUnit_exp()) {
					if((CHILD(i2).isUnit() && CHILD(i2).prefix()) || (CHILD(i2).isPower() && CHILD(i2)[0].prefix())) {
						b = false;
						return;
					}
					if(po.use_prefixes_for_currencies || (CHILD(i2).isPower() && !CHILD(i2)[0].unit()->isCurrency()) || (CHILD(i2).isUnit() && !CHILD(i2).unit()->isCurrency())) {
						b = true;
						if(i > i2) {i = i2; b_im = false;}
						break;
					} else if(i < i2) {
						i = i2;
						b_im = false;
					}
				} else if(CHILD(i2).isPower() && CHILD(i2)[0].isMultiplication()) {
					for(size_t i3 = 0; i3 < CHILD(i2)[0].size(); i3++) {
						if(CHILD(i2)[0][i3].isUnit_exp()) {
							if((CHILD(i2)[0][i3].isUnit() && CHILD(i2)[0][i3].prefix()) || (CHILD(i2)[0][i3].isPower() && CHILD(i2)[0][i3][0].prefix())) {
								b = false;
								return;
							}
							if(po.use_prefixes_for_currencies || (CHILD(i2)[0][i3].isPower() && !CHILD(i2)[0][i3][0].unit()->isCurrency()) || (CHILD(i2)[0][i3].isUnit() && !CHILD(i2)[0][i3].unit()->isCurrency())) {
								b = true;
								if(i > i2) {
									i = i2;
									im = i3;
									b_im = true;
								}
								break;
							} else if(i < i2 || (i == i2 && im < i3)) {
								i = i2;
								im = i3;
								b_im = true;
							}
						}
					}
					if(b) break;
				}
			}
			if(b) {
				Number exp(1, 1);
				Number exp2(1, 1);
				bool b2 = false;
				MathStructure *munit = NULL, *munit2 = NULL;
				if(b_im) munit = &CHILD(i)[0][im];
				else munit = &CHILD(i);
				if(CHILD(i).isPower()) {
					if(CHILD(i)[1].isNumber() && CHILD(i)[1].number().isInteger()) {
						if(b_im && munit->isPower()) {
							if((*munit)[1].isNumber() && (*munit)[1].number().isInteger()) {
								exp = CHILD(i)[1].number();
								exp *= (*munit)[1].number();
							} else {
								b = false;
							}
						} else {
							exp = CHILD(i)[1].number();
						}
					} else {
						b = false;
					}
				}
				if(po.use_denominator_prefix && !exp.isNegative()) {
					for(size_t i2 = i + 1; i2 < SIZE; i2++) {
						if(CHILD(i2).isPower() && CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isNegative()) {
							if(CHILD(i2)[0].isUnit()) {
								munit2 = &CHILD(i2)[0];
								if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
									break;
								}
								if(!b) {
									b = true;
									exp = CHILD(i2)[1].number();
									munit = munit2;
								} else {
									b2 = true;
									exp2 = CHILD(i2)[1].number();
								}
								break;
							} else if(CHILD(i2)[0].isMultiplication()) {
								for(size_t im2 = 0; im2 < CHILD(i2)[0].size(); im2++) {
									if(CHILD(i2)[0][im2].isUnit_exp() && (CHILD(i2)[0][im2].isUnit() || CHILD(i2)[0][im2][1].isNumber() && CHILD(i2)[0][im2][1].number().isNegative() && CHILD(i2)[0][im2][1].number().isInteger())) {
										Number exp_multi(1);
										if(CHILD(i2)[0][im2].isUnit()) {
											munit2 = &CHILD(i2)[0][im2];
										} else {
											munit2 = &CHILD(i2)[0][im2][0];
											exp_multi = CHILD(i2)[0][im2][1].number();
										}
										if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
											break;
										}
										if(!b) {
											b = true;
											exp = CHILD(i2)[1].number();
											exp *= exp_multi;
											i = i2;
										} else {
											b2 = true;
											exp2 = CHILD(i2)[1].number();
											exp2 *= exp_multi;
										}
										break;
									}
								}
							}
						}
					}
				} else if(exp.isNegative() && b) {
					for(size_t i2 = i + 1; i2 < SIZE; i2++) {
						bool b3 = false;
						if(CHILD(i2).isPower() && !(CHILD(i2)[1].isNumber() && CHILD(i2)[1].number().isNegative())) {
							if(CHILD(i2)[0].isUnit()) {
								exp2 = exp;
								exp = CHILD(i2)[1].number();
								munit2 = &CHILD(i2);
							} else if(CHILD(i2)[0].isMultiplication()) {
								for(size_t im2 = 0; im2 < CHILD(i2)[0].size(); im2++) {
									if(CHILD(i2)[0][im2].isUnit_exp() && (CHILD(i2)[0][im2].isUnit() || CHILD(i2)[0][im2][1].isNumber() && CHILD(i2)[0][im2][1].number().isNegative() && CHILD(i2)[0][im2][1].number().isInteger())) {
										exp2 = exp;
										Number exp_multi(1);
										if(CHILD(i2)[0][im2].isUnit()) {
											munit2 = &CHILD(i2)[0][im2];
										} else {
											munit2 = &CHILD(i2)[0][im2][0];
											exp_multi = CHILD(i2)[0][im2][1].number();
										}
										exp = CHILD(i2)[1].number();
										exp *= exp_multi;
										break;
									}
								}
							}
							if(munit2) {
								if(munit2->prefix() || !CHILD(i2)[1].number().isInteger()) {
									if(!po.use_denominator_prefix) {
										b = false;
									}
									break;
								}
								b3 = true;
							}
						} else if(CHILD(i2).isUnit()) {
							exp2 = exp;
							exp.set(1, 1);
							b3 = true;
							munit2 = &CHILD(i2);
						}
						if(b3) {
							if(po.use_denominator_prefix) {
								b2 = true;
								MathStructure *munit3 = munit;
								munit = munit2;
								munit2 = munit3;
							} else {
								munit = munit2;
							}
							break;
						}
					}
				}
				Number exp10;
				if(b) {
					if(po.prefix) {
						if(po.prefix != CALCULATOR->decimal_null_prefix && po.prefix != CALCULATOR->binary_null_prefix) {
							if(munit->isUnit()) munit->setPrefix(po.prefix);
							else (*munit)[0].setPrefix(po.prefix);
							if(CHILD(0).isNumber()) {
								CHILD(0).number() /= po.prefix->value(exp);
							} else {
								PREPEND(po.prefix->value(exp));
								CHILD(0).number().recip();
							}
							if(b2) {
								exp10 = CHILD(0).number();
								exp10.log(10);
								exp10.floor();
							}
						}
					} else if(po.use_unit_prefixes && CHILD(0).isNumber() && exp.isInteger()) {
						exp10 = CHILD(0).number();
						exp10.log(10);
						exp10.floor();
						if(b2) {	
							Number tmp_exp(exp10);
							tmp_exp.setNegative(false);
							Number e1(3, 1);
							e1 *= exp;
							Number e2(3, 1);
							e2 *= exp2;
							e2.setNegative(false);
							int i4 = 0;
							while(true) {
								tmp_exp -= e1;
								if(!tmp_exp.isPositive()) {
									break;
								}
								if(exp10.isNegative()) i4++;
								tmp_exp -= e2;
								if(tmp_exp.isNegative()) {
									break;
								}
								if(!exp10.isNegative())	i4++;
							}
							e2.setNegative(exp10.isNegative());
							e2 *= i4;
							exp10 -= e2;
						}
						DecimalPrefix *p = CALCULATOR->getBestDecimalPrefix(exp10, exp, po.use_all_prefixes);
						if(p) {
							Number test_exp(exp10);
							test_exp -= p->exponent(exp);
							if(test_exp.isInteger()) {
								if((exp10.isPositive() && exp10.compare(test_exp) == COMPARISON_RESULT_LESS) || (exp10.isNegative() && exp10.compare(test_exp) == COMPARISON_RESULT_GREATER)) {
									CHILD(0).number() /= p->value(exp);
									if(munit->isUnit()) munit->setPrefix(p);
									else (*munit)[0].setPrefix(p);
								}
							}
						}
					}
					if(b2 && CHILD(0).isNumber() && ((po.prefix && po.prefix != CALCULATOR->decimal_null_prefix && po.prefix != CALCULATOR->binary_null_prefix) || po.use_unit_prefixes)) {
						exp10 = CHILD(0).number();
						exp10.log(10);
						exp10.floor();
						DecimalPrefix *p = CALCULATOR->getBestDecimalPrefix(exp10, exp2, po.use_all_prefixes);
						if(p) {
							Number test_exp(exp10);
							test_exp -= p->exponent(exp2);
							if(test_exp.isInteger()) {
								if((exp10.isPositive() && exp10.compare(test_exp) == COMPARISON_RESULT_LESS) || (exp10.isNegative() && exp10.compare(test_exp) == COMPARISON_RESULT_GREATER)) {
									CHILD(0).number() /= p->value(exp2);
									if(munit2->isUnit()) munit2->setPrefix(p);
									else (*munit2)[0].setPrefix(p);
								}
							}
						}
					}	
				}
				break;
			}
		}
		case STRUCT_UNIT: {
			if(!o_prefix && (po.prefix && po.prefix != CALCULATOR->decimal_null_prefix && po.prefix != CALCULATOR->binary_null_prefix)) {
				transform(STRUCT_MULTIPLICATION, m_one);
				SWAP_CHILDREN(0, 1);
				setPrefixes(po, parent, pindex);
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(0).isUnit()) {
				if(CHILD(1).isNumber() && CHILD(1).number().isReal() && !o_prefix && (po.prefix && po.prefix != CALCULATOR->decimal_null_prefix && po.prefix != CALCULATOR->binary_null_prefix)) {
					transform(STRUCT_MULTIPLICATION, m_one);
					SWAP_CHILDREN(0, 1);
					setPrefixes(po, parent, pindex);
				}
				break;
			}
		}
		default: {
			for(size_t i = 0; i < SIZE; i++) {
				CHILD(i).setPrefixes(po, this, i + 1);
			}
		}
	}
}
bool split_unit_powers(MathStructure &mstruct);
bool split_unit_powers(MathStructure &mstruct) {
	bool b = false;
	for(size_t i = 0; i < mstruct.size(); i++) {
		if(split_unit_powers(mstruct[i])) {
			mstruct.childUpdated(i + 1);
			b = true;
		}
	}
	if(mstruct.isPower() && mstruct[0].isMultiplication()) {
		bool b2 = mstruct[1].isNumber();
		for(size_t i = 0; i < mstruct[0].size(); i++) {
			if(mstruct[0][i].isPower() && (!b2 || !mstruct[0][i][1].isNumber())) return b;
		}
		MathStructure mpower(mstruct[1]);
		mstruct.setToChild(1);
		for(size_t i = 0; i < mstruct.size(); i++) {
			if(mstruct[i].isPower()) mstruct[i][1].number() *= mpower.number();
			else mstruct[i].raise(mpower);
		}
		mstruct.childrenUpdated();
		return true;
	}
	return b;
}
void MathStructure::postFormatUnits(const PrintOptions &po, MathStructure*, size_t) {
	switch(m_type) {
		case STRUCT_DIVISION: {
			if(po.place_units_separately) {
				vector<size_t> nums;
				bool b1 = false, b2 = false;
				if(CHILD(0).isMultiplication()) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isUnit_exp()) {
							nums.push_back(i);
						} else {
							b1 = true;
						}
					}
					b1 = b1 && !nums.empty();
				} else if(CHILD(0).isUnit_exp()) {
					b1 = true;
				}
				vector<size_t> dens;
				if(CHILD(1).isMultiplication()) {
					for(size_t i = 0; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isUnit_exp()) {
							dens.push_back(i);
						} else {
							b2 = true;
						}
					}
					b2 = b2 && !dens.empty();
				} else if(CHILD(1).isUnit_exp()) {
					if(CHILD(0).isUnit_exp()) {
						b1 = false;
					} else {
						b2 = true;
					}
				}
				if(b2 && !b1) b1 = true;
				if(b1) {
					MathStructure num = m_undefined;
					if(CHILD(0).isUnit_exp()) {
						num = CHILD(0);
						CHILD(0).set(m_one);
					} else if(nums.size() > 0) {
						num = CHILD(0)[nums[0]];
						for(size_t i = 1; i < nums.size(); i++) {
							num.multiply(CHILD(0)[nums[i]], i > 1);
						}
						for(size_t i = 0; i < nums.size(); i++) {
							CHILD(0).delChild(nums[i] + 1 - i);
						}
						if(CHILD(0).size() == 1) {
							CHILD(0).setToChild(1, true);
						}
					}
					MathStructure den = m_undefined;
					if(CHILD(1).isUnit_exp()) {
						den = CHILD(1);
						setToChild(1, true);
					} else if(dens.size() > 0) {
						den = CHILD(1)[dens[0]];
						for(size_t i = 1; i < dens.size(); i++) {
							den.multiply(CHILD(1)[dens[i]], i > 1);
						}
						for(size_t i = 0; i < dens.size(); i++) {
							CHILD(1).delChild(dens[i] + 1 - i);
						}
						if(CHILD(1).size() == 1) {
							CHILD(1).setToChild(1, true);
						}
					}
					if(num.isUndefined()) {
						transform(STRUCT_DIVISION, den);
					} else {
						if(!den.isUndefined()) {
							num.transform(STRUCT_DIVISION, den);
						}
						multiply(num, false);
					}
					if(CHILD(0).isDivision()) {
						if(CHILD(0)[0].isMultiplication()) {
							if(CHILD(0)[0].size() == 1) {
								CHILD(0)[0].setToChild(1, true);
							} else if(CHILD(0)[0].size() == 0) {
								CHILD(0)[0] = 1;
							}
						}
						if(CHILD(0)[1].isMultiplication()) {
							if(CHILD(0)[1].size() == 1) {
								CHILD(0)[1].setToChild(1, true);
							} else if(CHILD(0)[1].size() == 0) {
								CHILD(0).setToChild(1, true);
							}
						} else if(CHILD(0)[1].isOne()) {
							CHILD(0).setToChild(1, true);
						}
						if(CHILD(0).isDivision() && CHILD(0)[1].isNumber() && CHILD(0)[0].isMultiplication() && CHILD(0)[0].size() > 1 && CHILD(0)[0][0].isNumber()) {
							MathStructure *msave = new MathStructure;
							if(CHILD(0)[0].size() == 2) {
								msave->set(CHILD(0)[0][1]);
								CHILD(0)[0].setToChild(1, true);
							} else {
								msave->set(CHILD(0)[0]);
								CHILD(0)[0].setToChild(1, true);
								msave->delChild(1);
							}
							if(isMultiplication()) {
								insertChild_nocopy(msave, 2);
							} else {
								CHILD(0).multiply_nocopy(msave);
							}
						}
					}
					bool do_plural = po.short_multiplication;
					CHILD(0).postFormatUnits(po, this, 1);
					CHILD_UPDATED(0);
					switch(CHILD(0).type()) {
						case STRUCT_NUMBER: {
							if(CHILD(0).isZero() || CHILD(0).number().isOne() || CHILD(0).number().isMinusOne() || CHILD(0).number().isFraction()) {
								do_plural = false;
							}
							break;
						}
						case STRUCT_DIVISION: {
							if(CHILD(0)[0].isNumber() && CHILD(0)[1].isNumber()) {
								if(CHILD(0)[0].number().isLessThanOrEqualTo(CHILD(0)[1].number())) {
									do_plural = false;
								}
							}
							break;
						}
						case STRUCT_INVERSE: {
							if(CHILD(0)[0].isNumber() && CHILD(0)[0].number().isGreaterThanOrEqualTo(1)) {
								do_plural = false;
							}
							break;
						}
						default: {}
					}
					split_unit_powers(CHILD(1));
					switch(CHILD(1).type()) {
						case STRUCT_UNIT: {
							CHILD(1).setPlural(do_plural);
							break;
						}
						case STRUCT_POWER: {
							CHILD(1)[0].setPlural(do_plural);
							break;
						}
						case STRUCT_MULTIPLICATION: {
							if(po.limit_implicit_multiplication) CHILD(1)[0].setPlural(do_plural);
							else CHILD(1)[CHILD(1).size() - 1].setPlural(do_plural);
							break;
						}
						case STRUCT_DIVISION: {
							switch(CHILD(1)[0].type()) {
								case STRUCT_UNIT: {
									CHILD(1)[0].setPlural(do_plural);
									break;
								}
								case STRUCT_POWER: {
									CHILD(1)[0][0].setPlural(do_plural);
									break;
								}
								case STRUCT_MULTIPLICATION: {
									if(po.limit_implicit_multiplication) CHILD(1)[0][0].setPlural(do_plural);
									else CHILD(1)[0][CHILD(1)[0].size() - 1].setPlural(do_plural);
									break;
								}
								default: {}
							}
							break;
						}
						default: {}						
					}
				}
			} else {
				for(size_t i = 0; i < SIZE; i++) {
					CHILD(i).postFormatUnits(po, this, i + 1);
					CHILD_UPDATED(i);
				}
			}
			break;
		}
		case STRUCT_UNIT: {
			b_plural = false;
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(SIZE > 1 && CHILD(1).isUnit_exp() && CHILD(0).isNumber()) {
				bool do_plural = po.short_multiplication && !(CHILD(0).isZero() || CHILD(0).number().isOne() || CHILD(0).number().isMinusOne() || CHILD(0).number().isFraction());
				size_t i = 2;
				for(; i < SIZE; i++) {
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(false);
					} else if(CHILD(i).isPower() && CHILD(i)[0].isUnit()) {
						CHILD(i)[0].setPlural(false);
					} else {
						break;
					}
				}
				if(do_plural) {
					if(po.limit_implicit_multiplication) i = 1;
					else i--;
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(true);
					} else {
						CHILD(i)[0].setPlural(true);
					}
				}
			} else if(SIZE > 0) {
				int last_unit = -1;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isUnit()) {
						CHILD(i).setPlural(false);
						if(!po.limit_implicit_multiplication || last_unit < 0) {
							last_unit = i;
						}
					} else if(CHILD(i).isPower() && CHILD(i)[0].isUnit()) {
						CHILD(i)[0].setPlural(false);
						if(!po.limit_implicit_multiplication || last_unit < 0) {
							last_unit = i;
						}
					} else if(last_unit >= 0) {
						break;
					}
				}
				if(po.short_multiplication && last_unit > 0) {
					if(CHILD(last_unit).isUnit()) {
						CHILD(last_unit).setPlural(true);
					} else {
						CHILD(last_unit)[0].setPlural(true);
					}
				}
			}
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(0).isUnit()) {
				CHILD(0).setPlural(false);
				break;
			}
		}
		default: {
			for(size_t i = 0; i < SIZE; i++) {
				CHILD(i).postFormatUnits(po, this, i + 1);
				CHILD_UPDATED(i);
			}
		}
	}
}
bool MathStructure::factorizeUnits() {
	switch(m_type) {
		case STRUCT_ADDITION: {
			MathStructure *factor_mstruct = new MathStructure(1, 1);
			MathStructure mnew;
			if(factorize_find_multiplier(*this, mnew, *factor_mstruct, true)) {
				set(mnew, true);
				if(factor_mstruct->isMultiplication()) {
					for(size_t i = 0; i < factor_mstruct->size(); i++) {
						multiply_nocopy(factor_mstruct->getChild(i + 1), true);
						factor_mstruct->getChild(i + 1)->ref();
					}
					factor_mstruct->unref();
				} else {
					multiply_nocopy(factor_mstruct);
				}
				return true;
			} else {
				factor_mstruct->unref();
			}
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).factorizeUnits()) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}
	}
}
void MathStructure::prefixCurrencies() {
	if(isMultiplication() && (!hasNegativeSign() || CALCULATOR->place_currency_code_before_negative)) {
		int index = -1;
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isUnit_exp()) {
				if(CHILD(i).isUnit() && CHILD(i).unit()->isCurrency()) {
					if(index >= 0) {
						index = -1;
						break;
					}
					index = i;
				} else {
					index = -1;
					break;
				}
			}
		}
		if(index >= 0) {
			v_order.insert(v_order.begin(), v_order[index]);
			v_order.erase(v_order.begin() + (index + 1));
		}
	} else {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).prefixCurrencies();
		}
	}
}
void MathStructure::format(const PrintOptions &po) {
	if(!po.preserve_format) {
		if(po.place_units_separately) {
			factorizeUnits();
		}
		sort(po);
		if(po.improve_division_multipliers) {
			if(improve_division_multipliers(po)) sort(po);
		}
		setPrefixes(po);
	}
	formatsub(po);
	if(!po.preserve_format) {
		postFormatUnits(po);
		if(po.sort_options.prefix_currencies && po.abbreviate_names && CALCULATOR->place_currency_code_before) {
			prefixCurrencies();
		}
	}
}
void MathStructure::formatsub(const PrintOptions &po, MathStructure *parent, size_t pindex, bool recursive) {

	if(recursive) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).formatsub(po, this, i + 1);
			CHILD_UPDATED(i);
		}
	}
	switch(m_type) {
		case STRUCT_ADDITION: {
			break;
		}
		case STRUCT_NEGATE: {
			break;
		}
		case STRUCT_DIVISION: {
			if(po.preserve_format) break;
			if(CHILD(0).isAddition() && CHILD(0).size() > 0 && CHILD(0)[0].isNegate()) {
				int imin = 1;
				for(size_t i = 1; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				bool b = CHILD(1).isAddition() && CHILD(1).size() > 0 && CHILD(1)[0].isNegate();
				if(b) {
					imin++;
					for(size_t i = 1; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isNegate()) {
							imin++;
						} else {
							imin--;
						}
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isNegate()) {
							CHILD(0)[i].setToChild(1, true);
						} else {
							CHILD(0)[i].transform(STRUCT_NEGATE);
						}
					}
					if(b) {
						for(size_t i = 0; i < CHILD(1).size(); i++) {
							if(CHILD(1)[i].isNegate()) {
								CHILD(1)[i].setToChild(1, true);
							} else {
								CHILD(1)[i].transform(STRUCT_NEGATE);
							}
						}
					} else {
						transform(STRUCT_NEGATE);
					}
					break;
				}
			} else if(CHILD(1).isAddition() && CHILD(1).size() > 0 && CHILD(1)[0].isNegate()) {
				int imin = 1;
				for(size_t i = 1; i < CHILD(1).size(); i++) {
					if(CHILD(1)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].isNegate()) {
							CHILD(1)[i].setToChild(1, true);
						} else {
							CHILD(1)[i].transform(STRUCT_NEGATE);
						}
					}
					transform(STRUCT_NEGATE);
				}
			}
			break;
		}
		case STRUCT_INVERSE: {
			if(po.preserve_format) break;
			if((!parent || !parent->isMultiplication()) && CHILD(0).isAddition() && CHILD(0).size() > 0 && CHILD(0)[0].isNegate()) {
				int imin = 1;
				for(size_t i = 1; i < CHILD(0).size(); i++) {
					if(CHILD(0)[i].isNegate()) {
						imin++;
					} else {
						imin--;
					}
				}
				if(imin > 0 || (imin == 0 && parent && parent->isNegate())) {
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isNegate()) {
							CHILD(0)[i].setToChild(1, true);
						} else {
							CHILD(0)[i].transform(STRUCT_NEGATE);
						}
					}
					transform(STRUCT_NEGATE);
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(po.preserve_format) break;
			if(CHILD(0).isNegate()) {
				if(CHILD(0)[0].isOne()) {
					ERASE(0);
					if(SIZE == 1) {
						setToChild(1, true);
					}
				} else {
					CHILD(0).setToChild(1, true);
				}
				transform(STRUCT_NEGATE);
				CHILD(0).formatsub(po, this, 1, false);
				break;
			}
			
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isInverse()) {
					if(!po.negative_exponents || !CHILD(i)[0].isNumber()) {
						b = true;
						break;
					}
				} else if(CHILD(i).isDivision()) {
					if(!CHILD(i)[0].isNumber() || !CHILD(i)[1].isNumber() || (!po.negative_exponents && CHILD(i)[0].number().isOne())) {
						b = true;
						break;
					}
				}
			}

			if(b) {
				MathStructure *den = new MathStructure();
				MathStructure *num = new MathStructure();
				num->setUndefined();
				short ds = 0, ns = 0;
				MathStructure *mnum = NULL, *mden = NULL;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isInverse()) {
						mden = &CHILD(i)[0];
					} else if(CHILD(i).isDivision()) {
						mnum = &CHILD(i)[0];
						mden = &CHILD(i)[1];
					} else {
						mnum = &CHILD(i);
					}
					if(mnum) {
						if(ns > 0) {
							if(mnum->isMultiplication() && num->isNumber()) {
								for(size_t i2 = 0; i2 < mnum->size(); i2++) {
									num->multiply((*mnum)[i2], true);
								}
							} else {
								num->multiply(*mnum, ns > 1);
							}
						} else {
							num->set(*mnum);
						}						
						ns++;
						mnum = NULL;
					}
					if(mden) {
						if(ds > 0) {	
							if(mden->isMultiplication() && den->isNumber()) {
								for(size_t i2 = 0; i2 < mden->size(); i2++) {
									den->multiply((*mden)[i2], true);
								}
							} else {
								den->multiply(*mden, ds > 1);
							}							
						} else {
							den->set(*mden);
						}
						ds++;
						mden = NULL;
					}
				}
				clear(true);
				m_type = STRUCT_DIVISION;
				if(num->isUndefined()) num->set(m_one);				
				APPEND_POINTER(num);
				APPEND_POINTER(den);
				num->formatsub(po, this, 1, false);
				den->formatsub(po, this, 2, false);
				formatsub(po, parent, pindex, false);
				break;
			}

			size_t index = 0;
			if(CHILD(0).isOne()) {
				index = 1;
			}
			switch(CHILD(index).type()) {
				case STRUCT_POWER: {
					if(!CHILD(index)[0].isUnit_exp()) {
						break;
					}
				}
				case STRUCT_UNIT: {
					if(index == 0) {
						if(!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && (!parent->isDivision() || pindex != 2))) {
							PREPEND(m_one);
						}
					}
					break;
				}
				default: {
					if(index == 1) {
						ERASE(0);
						if(SIZE == 1) {
							setToChild(1, true);
						}
					}
				}
			}
			break;
		}
		case STRUCT_UNIT: {
			if(po.preserve_format) break;
			if(!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && !(parent->isDivision() && pindex == 2))) {
				multiply(m_one);
				SWAP_CHILDREN(0, 1);
			}
			break;
		}
		case STRUCT_POWER: {
			if(po.preserve_format) break;
			if(!po.negative_exponents && CHILD(1).isNegate() && (!CHILD(0).isVector() || !CHILD(1).isMinusOne())) {
				if(CHILD(1)[0].isOne()) {
					m_type = STRUCT_INVERSE;
					ERASE(1);
				} else {
					CHILD(1).setToChild(1, true);
					transform(STRUCT_INVERSE);
				}
				formatsub(po, parent, pindex, false);
			} else if(po.halfexp_to_sqrt && ((CHILD(1).isDivision() && CHILD(1)[0].isNumber() && CHILD(1)[0].number().isInteger() && CHILD(1)[1].isNumber() && CHILD(1)[1].number().isTwo() && ((!po.negative_exponents && CHILD(0).countChildren() == 0) || CHILD(1)[0].isOne())) || (CHILD(1).isNumber() && CHILD(1).number().denominatorIsTwo() && ((!po.negative_exponents && CHILD(0).countChildren() == 0) || CHILD(1).number().numeratorIsOne())) || (CHILD(1).isInverse() && CHILD(1)[0].isNumber() && CHILD(1)[0].number() == 2))) {
				if(CHILD(1).isInverse() || (CHILD(1).isDivision() && CHILD(1)[0].number().isOne()) || (CHILD(1).isNumber() && CHILD(1).number().numeratorIsOne())) {
					m_type = STRUCT_FUNCTION;
					ERASE(1)
					o_function = CALCULATOR->f_sqrt;
				} else {
					if(CHILD(1).isNumber()) {
						CHILD(1).number() -= Number(1, 2);
					} else {
						Number nr = CHILD(1)[0].number();
						nr /= CHILD(1)[1].number();
						nr.floor();
						CHILD(1).set(nr);
					}
					if(CHILD(1).number().isOne()) {
						setToChild(1, true);
						if(parent && parent->isMultiplication()) {
							parent->insertChild(MathStructure(CALCULATOR->f_sqrt, this, NULL), pindex + 1);
						} else {
							multiply(MathStructure(CALCULATOR->f_sqrt, this, NULL));
						}
					} else {
						if(parent && parent->isMultiplication()) {
							parent->insertChild(MathStructure(CALCULATOR->f_sqrt, &CHILD(0), NULL), pindex + 1);
						} else {
							multiply(MathStructure(CALCULATOR->f_sqrt, &CHILD(0), NULL));
						}
					}
				}
				formatsub(po, parent, pindex, false);
			} else if(CHILD(0).isUnit_exp() && (!parent || (!parent->isPower() && !parent->isMultiplication() && !parent->isInverse() && !(parent->isDivision() && pindex == 2)))) {
				multiply(m_one);
				SWAP_CHILDREN(0, 1);
			}
			break;
		}
		case STRUCT_NUMBER: {
			if(o_number.isNegative()) {
				o_number.negate();
				transform(STRUCT_NEGATE);
				formatsub(po, parent, pindex);
			} else if(po.number_fraction_format == FRACTION_COMBINED && po.base != BASE_SEXAGESIMAL && po.base != BASE_TIME && o_number.isRational() && !o_number.isInteger()) {
				if(o_number.isFraction()) {
					Number num(o_number.numerator());
					Number den(o_number.denominator());
					clear(true);
					if(num.isOne()) {
						m_type = STRUCT_INVERSE;
					} else {
						m_type = STRUCT_DIVISION;
						APPEND_NEW(num);
					}
					APPEND_NEW(den);
				} else {
					Number frac(o_number);
					frac.frac();
					MathStructure *num = new MathStructure(frac.numerator());
					num->transform(STRUCT_DIVISION, frac.denominator());
					o_number.trunc();
					add_nocopy(num);
				}
			} else if((po.number_fraction_format == FRACTION_FRACTIONAL || po.base == BASE_ROMAN_NUMERALS) && po.base != BASE_SEXAGESIMAL && po.base != BASE_TIME && o_number.isRational() && !o_number.isInteger()) {
				Number num(o_number.numerator());
				Number den(o_number.denominator());
				clear(true);
				if(num.isOne()) {
					m_type = STRUCT_INVERSE;
				} else {
					m_type = STRUCT_DIVISION;
					APPEND_NEW(num);
				}
				APPEND_NEW(den);
			} else if(po.number_fraction_format == FRACTION_DECIMAL_EXACT && po.base != BASE_SEXAGESIMAL && po.base != BASE_TIME && o_number.isRational() && !o_number.isInteger() && !o_number.isApproximate()) {
				string str_den = "";
				InternalPrintStruct ips_n;
				if(isApproximate()) ips_n.parent_approximate = true;
				ips_n.parent_precision = precision();
				ips_n.den = &str_den;
				PrintOptions po2 = po;
				po2.is_approximate = NULL;
				o_number.print(po2, ips_n);
				if(!str_den.empty()) {
					Number num(o_number.numerator());
					Number den(o_number.denominator());
					clear(true);
					if(num.isOne()) {
						m_type = STRUCT_INVERSE;
					} else {
						m_type = STRUCT_DIVISION;
						APPEND_NEW(num);
					}
					APPEND_NEW(den);
				}
			} else if(o_number.isComplex()) {
				if(o_number.hasRealPart()) {
					Number re(o_number.realPart());
					Number im(o_number.imaginaryPart());
					MathStructure *mstruct = new MathStructure(im);
					if(im.isOne()) {
						mstruct->set(CALCULATOR->v_i);
					} else {
						mstruct->multiply_nocopy(new MathStructure(CALCULATOR->v_i));
					}
					o_number = re;
					add_nocopy(mstruct);
					formatsub(po, parent, pindex);
				} else {
					Number im(o_number.imaginaryPart());
					if(im.isOne()) {
						set(CALCULATOR->v_i, true);
					} else if(im.isMinusOne()) {
						set(CALCULATOR->v_i, true);
						transform(STRUCT_NEGATE);
					} else {
						o_number = im;
						multiply_nocopy(new MathStructure(CALCULATOR->v_i));
					}
					formatsub(po, parent, pindex);
				}
			}
			break;
		}
		default: {}
	}
}

int namelen(const MathStructure &mstruct, const PrintOptions &po, const InternalPrintStruct&, bool *abbreviated = NULL) {
	const string *str;
	switch(mstruct.type()) {
		case STRUCT_FUNCTION: {
			const ExpressionName *ename = &mstruct.function()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		case STRUCT_VARIABLE:  {
			const ExpressionName *ename = &mstruct.variable()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		case STRUCT_SYMBOLIC:  {
			str = &mstruct.symbol();
			if(abbreviated) *abbreviated = false;
			break;
		}
		case STRUCT_UNIT:  {
			const ExpressionName *ename = &mstruct.unit()->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, mstruct.isPlural(), po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			str = &ename->name;
			if(abbreviated) *abbreviated = ename->abbreviation;
			break;
		}
		default: {if(abbreviated) *abbreviated = false; return 0;}
	}
	if(text_length_is_one(*str)) return 1;
	return str->length();
}

bool MathStructure::needsParenthesis(const PrintOptions &po, const InternalPrintStruct &ips, const MathStructure &parent, size_t index, bool flat_division, bool) const {
	if(o_uncertainty) return true;
	switch(parent.type()) {
		case STRUCT_MULTIPLICATION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return flat_division;}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}				
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return false;}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return po.excessive_parenthesis;}
				default: {return true;}
			}
		}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_POWER: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_OR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_XOR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_BITWISE_NOT: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_OR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_XOR: {return flat_division || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_NOT: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return flat_division || po.excessive_parenthesis;}				
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return false;}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_ADDITION: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return index > 1 || po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}				
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return false;}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_POWER: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return index == 1 || flat_division || po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return index == 1 || flat_division || po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return true;}
				case STRUCT_NEGATE: {return index == 1 || CHILD(0).needsParenthesis(po, ips, parent, index, flat_division);}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return index == 1 || po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return index == 1 || po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}				
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return false;}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_NEGATE: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return true;}
				case STRUCT_NEGATE: {return true;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return po.excessive_parenthesis;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return po.excessive_parenthesis;}
				case STRUCT_COMPARISON: {return true;}				
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return false;}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_XOR: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return flat_division;}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return false;}				
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return false;}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_BITWISE_AND: {}
		case STRUCT_BITWISE_OR: {}
		case STRUCT_BITWISE_XOR: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return flat_division;}
				case STRUCT_INVERSE: {return flat_division;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}				
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return false;}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_COMPARISON: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return po.excessive_parenthesis;}
				case STRUCT_DIVISION: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_INVERSE: {return flat_division && po.excessive_parenthesis;}
				case STRUCT_ADDITION: {return po.excessive_parenthesis;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return false;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return false;}
				case STRUCT_COMPARISON: {return true;}				
				case STRUCT_FUNCTION: {return false;}
				case STRUCT_VECTOR: {return false;}
				case STRUCT_NUMBER: {return false;}
				case STRUCT_VARIABLE: {return false;}
				case STRUCT_SYMBOLIC: {return false;}
				case STRUCT_UNIT: {return false;}
				case STRUCT_UNDEFINED: {return false;}
				default: {return true;}
			}
		}
		case STRUCT_LOGICAL_NOT: {}
		case STRUCT_BITWISE_NOT: {
			switch(m_type) {
				case STRUCT_MULTIPLICATION: {return true;}
				case STRUCT_DIVISION: {return true;}
				case STRUCT_INVERSE: {return true;}
				case STRUCT_ADDITION: {return true;}
				case STRUCT_POWER: {return po.excessive_parenthesis;}
				case STRUCT_NEGATE: {return po.excessive_parenthesis;}
				case STRUCT_BITWISE_AND: {return true;}
				case STRUCT_BITWISE_OR: {return true;}
				case STRUCT_BITWISE_XOR: {return true;}
				case STRUCT_BITWISE_NOT: {return true;}
				case STRUCT_LOGICAL_AND: {return true;}
				case STRUCT_LOGICAL_OR: {return true;}
				case STRUCT_LOGICAL_XOR: {return true;}
				case STRUCT_LOGICAL_NOT: {return true;}
				case STRUCT_COMPARISON: {return true;}				
				case STRUCT_FUNCTION: {return po.excessive_parenthesis;}
				case STRUCT_VECTOR: {return po.excessive_parenthesis;}
				case STRUCT_NUMBER: {return po.excessive_parenthesis;}
				case STRUCT_VARIABLE: {return po.excessive_parenthesis;}
				case STRUCT_SYMBOLIC: {return po.excessive_parenthesis;}
				case STRUCT_UNIT: {return po.excessive_parenthesis;}
				case STRUCT_UNDEFINED: {return po.excessive_parenthesis;}
				default: {return true;}
			}
		}
		case STRUCT_FUNCTION: {
			return false;
		}
		case STRUCT_VECTOR: {
			return false;
		}
		default: {
			return true;
		}
	}
}

int MathStructure::neededMultiplicationSign(const PrintOptions &po, const InternalPrintStruct &ips, const MathStructure &parent, size_t index, bool par, bool par_prev, bool flat_division, bool flat_power) const {
	if(!po.short_multiplication) return MULTIPLICATION_SIGN_OPERATOR;
	if(index <= 1) return MULTIPLICATION_SIGN_NONE;
	if(par_prev && par) return MULTIPLICATION_SIGN_NONE;
	if(par_prev) {
		if(isUnit_exp()) return MULTIPLICATION_SIGN_SPACE;
		if(isMultiplication() || isDivision()) {
			for(size_t i = 0; i < SIZE; i++) {
				if(!CHILD(i).isUnit_exp()) {
					return MULTIPLICATION_SIGN_OPERATOR;
				}
			}
			return MULTIPLICATION_SIGN_SPACE;
		}
		return MULTIPLICATION_SIGN_OPERATOR;
	}
	int t = parent[index - 2].type();
	if(flat_power && t == STRUCT_POWER) return MULTIPLICATION_SIGN_OPERATOR;
	if(par && t == STRUCT_POWER) return MULTIPLICATION_SIGN_SPACE;
	if(par) return MULTIPLICATION_SIGN_NONE;
	bool abbr_prev = false, abbr_this = false;
	int namelen_prev = namelen(parent[index - 2], po, ips, &abbr_prev);
	int namelen_this = namelen(*this, po, ips, &abbr_this);	
	switch(t) {
		case STRUCT_MULTIPLICATION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {if(flat_division) return MULTIPLICATION_SIGN_OPERATOR; return MULTIPLICATION_SIGN_SPACE;}
		case STRUCT_ADDITION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_POWER: {if(flat_power) return MULTIPLICATION_SIGN_OPERATOR; break;}
		case STRUCT_NEGATE: {break;}
		case STRUCT_BITWISE_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_COMPARISON: {return MULTIPLICATION_SIGN_OPERATOR;}		
		case STRUCT_FUNCTION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_VECTOR: {break;}
		case STRUCT_NUMBER: {break;}
		case STRUCT_VARIABLE: {break;}
		case STRUCT_SYMBOLIC: {
			break;
		}
		case STRUCT_UNIT: {
			if(m_type == STRUCT_UNIT) {
				if(!po.limit_implicit_multiplication && !abbr_prev && !abbr_this) {
					return MULTIPLICATION_SIGN_SPACE;
				} 
				if(po.place_units_separately) {
					return MULTIPLICATION_SIGN_OPERATOR_SHORT;
				} else {
					return MULTIPLICATION_SIGN_OPERATOR;
				}
			} else if(m_type == STRUCT_NUMBER) {
				if(namelen_prev > 1) {
					return MULTIPLICATION_SIGN_SPACE;
				} else {
					return MULTIPLICATION_SIGN_NONE;
				}
			}
			//return MULTIPLICATION_SIGN_SPACE;
		}
		case STRUCT_UNDEFINED: {break;}
		default: {return MULTIPLICATION_SIGN_OPERATOR;}
	}
	switch(m_type) {
		case STRUCT_MULTIPLICATION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_INVERSE: {}
		case STRUCT_DIVISION: {if(flat_division) return MULTIPLICATION_SIGN_OPERATOR; return MULTIPLICATION_SIGN_SPACE;}
		case STRUCT_ADDITION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_POWER: {return CHILD(0).neededMultiplicationSign(po, ips, parent, index, par, par_prev, flat_division, flat_power);}
		case STRUCT_NEGATE: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_BITWISE_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_AND: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_OR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_XOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_LOGICAL_NOT: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_COMPARISON: {return MULTIPLICATION_SIGN_OPERATOR;}		
		case STRUCT_FUNCTION: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_VECTOR: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_NUMBER: {return MULTIPLICATION_SIGN_OPERATOR;}
		case STRUCT_VARIABLE: {}
		case STRUCT_SYMBOLIC: {
			if(po.limit_implicit_multiplication && t != STRUCT_NUMBER) return MULTIPLICATION_SIGN_OPERATOR;
			if(t != STRUCT_NUMBER && ((namelen_prev > 1 || namelen_this > 1) || equals(parent[index - 2]))) return MULTIPLICATION_SIGN_OPERATOR;
			if(namelen_this > 1 || (m_type == STRUCT_SYMBOLIC && !po.allow_non_usable)) return MULTIPLICATION_SIGN_SPACE;
			return MULTIPLICATION_SIGN_NONE;
		}
		case STRUCT_UNIT: {
			if(t == STRUCT_POWER && parent[index - 2][0].isUnit_exp()) {
				return MULTIPLICATION_SIGN_NONE;
			}			
			return MULTIPLICATION_SIGN_SPACE;
		}
		case STRUCT_UNDEFINED: {return MULTIPLICATION_SIGN_OPERATOR;}
		default: {return MULTIPLICATION_SIGN_OPERATOR;}
	}
}

string MathStructure::print(const PrintOptions &po, const InternalPrintStruct &ips) const {
	if(ips.depth == 0 && po.is_approximate) *po.is_approximate = false;
	string print_str;
	InternalPrintStruct ips_n = ips;
	if(isApproximate()) ips_n.parent_approximate = true;
	if(precision() > 0 && (ips_n.parent_precision < 1 || precision() < ips_n.parent_precision)) ips_n.parent_precision = precision();
	switch(m_type) {
		case STRUCT_NUMBER: {
			print_str = o_number.print(po, ips_n);
			break;
		}
		case STRUCT_SYMBOLIC: {
			if(po.allow_non_usable) {
				print_str = s_sym;
			} else {
				print_str = "\"";
				print_str += s_sym;
				print_str += "\"";
			}
			break;
		}
		case STRUCT_ADDITION: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					if(CHILD(i).type() == STRUCT_NEGATE) {
						if(po.spacious) print_str += " ";
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) print_str += SIGN_MINUS;
						else print_str += "-";
						if(po.spacious) print_str += " ";
						ips_n.wrap = CHILD(i)[0].needsParenthesis(po, ips_n, *this, i + 1, true, true);
						print_str += CHILD(i)[0].print(po, ips_n);
					} else {
						if(po.spacious) print_str += " ";
						print_str += "+";
						if(po.spacious) print_str += " ";
						ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
						print_str += CHILD(i).print(po, ips_n);
					}
				} else {
					ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
					print_str += CHILD(i).print(po, ips_n);
				}
			}
			break;
		}
		case STRUCT_NEGATE: {
			if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MINUS, po.can_display_unicode_string_arg))) print_str += SIGN_MINUS;
			else print_str = "-";
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, ips_n);
			break;
		}
		case STRUCT_MULTIPLICATION: {
			ips_n.depth++;
			bool par_prev = false;
			for(size_t i = 0; i < SIZE; i++) {
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				if(!po.short_multiplication && i > 0) {
					if(po.spacious) print_str += " ";
					if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIDOT;
					else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_SMALLCIRCLE, po.can_display_unicode_string_arg))) print_str += SIGN_SMALLCIRCLE;
					else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIPLICATION;
					else print_str += "*";
					if(po.spacious) print_str += " ";
				} else if(i > 0) {
					switch(CHILD(i).neededMultiplicationSign(po, ips_n, *this, i + 1, ips_n.wrap || (CHILD(i).isPower() && CHILD(i)[0].needsParenthesis(po, ips_n, CHILD(i), 1, true, true)), par_prev, true, true)) {
						case MULTIPLICATION_SIGN_SPACE: {print_str += " "; break;}
						case MULTIPLICATION_SIGN_OPERATOR: {
							if(po.spacious) {
								if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += " " SIGN_MULTIDOT " ";
								else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_SMALLCIRCLE, po.can_display_unicode_string_arg))) print_str += " " SIGN_SMALLCIRCLE " ";
								else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += " " SIGN_MULTIPLICATION " ";
								else print_str += " * ";
								break;
							}
						}
						case MULTIPLICATION_SIGN_OPERATOR_SHORT: {
							if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIDOT, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIDOT;
							else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_DOT && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_SMALLCIRCLE, po.can_display_unicode_string_arg))) print_str += SIGN_SMALLCIRCLE;
							else if(po.use_unicode_signs && po.multiplication_sign == MULTIPLICATION_SIGN_X && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_MULTIPLICATION, po.can_display_unicode_string_arg))) print_str += SIGN_MULTIPLICATION;
							else print_str += "*";
							break;
						}
					}
				}
				print_str += CHILD(i).print(po, ips_n);
				par_prev = ips_n.wrap;
			}
			break;
		}
		case STRUCT_INVERSE: {
			print_str = "1";
			if(po.spacious) print_str += " ";
			if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
				print_str += SIGN_DIVISION;
			} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
				print_str += SIGN_DIVISION_SLASH;
			} else {
				print_str += "/";
			}
			if(po.spacious) print_str += " ";
			ips_n.depth++;
			ips_n.division_depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, ips_n);
			break;
		}
		case STRUCT_DIVISION: {
			ips_n.depth++;
			ips_n.division_depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str = CHILD(0).print(po, ips_n);
			if(po.spacious) print_str += " ";
			if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION, po.can_display_unicode_string_arg))) {
				print_str += SIGN_DIVISION;
			} else if(po.use_unicode_signs && po.division_sign == DIVISION_SIGN_DIVISION_SLASH && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_DIVISION_SLASH, po.can_display_unicode_string_arg))) {
				print_str += SIGN_DIVISION_SLASH;
			} else {
				print_str += "/";
			}
			if(po.spacious) print_str += " ";
			ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
			print_str += CHILD(1).print(po, ips_n);
			break;
		}
		case STRUCT_POWER: {
			ips_n.depth++;
			ips_n.power_depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str = CHILD(0).print(po, ips_n);
			print_str += "^";
			ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
			PrintOptions po2 = po;
			po2.show_ending_zeroes = false;
			print_str += CHILD(1).print(po2, ips_n);
			break;
		}
		case STRUCT_COMPARISON: {
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str = CHILD(0).print(po, ips_n);
			if(po.spacious) print_str += " ";
			switch(ct_comp) {
				case COMPARISON_EQUALS: {print_str += "="; break;}
				case COMPARISON_NOT_EQUALS: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_NOT_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_NOT_EQUAL;
					else print_str += "!="; 
					break;
				}
				case COMPARISON_GREATER: {print_str += ">"; break;}
				case COMPARISON_LESS: {print_str += "<"; break;}
				case COMPARISON_EQUALS_GREATER: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
					else print_str += ">="; 
					break;
				}
				case COMPARISON_EQUALS_LESS: {
					if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
					else print_str += "<="; 
					break;
				}
			}
			if(po.spacious) print_str += " ";
			ips_n.wrap = CHILD(1).needsParenthesis(po, ips_n, *this, 2, true, true);
			print_str += CHILD(1).print(po, ips_n);
			break;
		}
		case STRUCT_BITWISE_AND: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					if(po.spacious) print_str += " ";
					print_str += "&";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_OR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					if(po.spacious) print_str += " ";
					print_str += "|";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_XOR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					if(po.spacious) print_str += " ";
					print_str += "XOR";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_BITWISE_NOT: {
			print_str = "~";
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, ips_n);
			break;
		}
		case STRUCT_LOGICAL_AND: {
			ips_n.depth++;
			if(!po.preserve_format && SIZE == 2 && CHILD(0).isComparison() && CHILD(1).isComparison() && CHILD(0).comparisonType() != COMPARISON_EQUALS && CHILD(0).comparisonType() != COMPARISON_NOT_EQUALS && CHILD(1).comparisonType() != COMPARISON_EQUALS && CHILD(1).comparisonType() != COMPARISON_NOT_EQUALS && CHILD(0)[0] == CHILD(1)[0]) {
				ips_n.wrap = CHILD(0)[1].needsParenthesis(po, ips_n, CHILD(0), 2, true, true);
				print_str += CHILD(0)[1].print(po, ips_n);
				if(po.spacious) print_str += " ";
				switch(CHILD(0).comparisonType()) {
					case COMPARISON_LESS: {print_str += ">"; break;}
					case COMPARISON_GREATER: {print_str += "<"; break;}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
						else print_str += ">="; 
						break;
					}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
						else print_str += "<="; 
						break;
					}
					default: {}
				}
				if(po.spacious) print_str += " ";
				
				ips_n.wrap = CHILD(0)[0].needsParenthesis(po, ips_n, CHILD(0), 1, true, true);
				print_str += CHILD(0)[0].print(po, ips_n);
				
				if(po.spacious) print_str += " ";
				switch(CHILD(1).comparisonType()) {
					case COMPARISON_GREATER: {print_str += ">"; break;}
					case COMPARISON_LESS: {print_str += "<"; break;}
					case COMPARISON_EQUALS_GREATER: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_GREATER_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_GREATER_OR_EQUAL;
						else print_str += ">="; 
						break;
					}
					case COMPARISON_EQUALS_LESS: {
						if(po.use_unicode_signs && (!po.can_display_unicode_string_function || (*po.can_display_unicode_string_function) (SIGN_LESS_OR_EQUAL, po.can_display_unicode_string_arg))) print_str += SIGN_LESS_OR_EQUAL;
						else print_str += "<="; 
						break;
					}
					default: {}
				}
				if(po.spacious) print_str += " ";
				
				ips_n.wrap = CHILD(1)[1].needsParenthesis(po, ips_n, CHILD(1), 2, true, true);
				print_str += CHILD(1)[1].print(po, ips_n);
				
				break;
			}
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					if(po.spacious) print_str += " ";
					if(po.spell_out_logical_operators) print_str += _("and");
					else print_str += "&&";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_OR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					if(po.spacious) print_str += " ";
					if(po.spell_out_logical_operators) print_str += _("or");
					else print_str += "||";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_XOR: {
			ips_n.depth++;
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					if(po.spacious) print_str += " ";
					print_str += "XOR";
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			break;
		}
		case STRUCT_LOGICAL_NOT: {
			print_str = "!";
			ips_n.depth++;
			ips_n.wrap = CHILD(0).needsParenthesis(po, ips_n, *this, 1, true, true);
			print_str += CHILD(0).print(po, ips_n);
			break;
		}
		case STRUCT_VECTOR: {
			ips_n.depth++;
			print_str = "[";
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					print_str += po.comma();
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			print_str += "]";
			break;
		}
		case STRUCT_UNIT: {
			const ExpressionName *ename = &o_unit->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, b_plural, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			if(o_prefix) print_str += o_prefix->name(po.abbreviate_names && ename->abbreviation, po.use_unicode_signs, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			break;
		}
		case STRUCT_VARIABLE: {
			const ExpressionName *ename = &o_variable->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			break;
		}
		case STRUCT_FUNCTION: {
			ips_n.depth++;
			const ExpressionName *ename = &o_function->preferredDisplayName(po.abbreviate_names, po.use_unicode_signs, false, po.use_reference_names, po.can_display_unicode_string_function, po.can_display_unicode_string_arg);
			print_str += ename->name;
			if(po.hide_underscore_spaces && !ename->suffix) {
				gsub("_", " ", print_str);
			}
			print_str += "(";
			for(size_t i = 0; i < SIZE; i++) {
				if(i > 0) {
					print_str += po.comma();
					if(po.spacious) print_str += " ";
				}
				ips_n.wrap = CHILD(i).needsParenthesis(po, ips_n, *this, i + 1, true, true);
				print_str += CHILD(i).print(po, ips_n);
			}
			print_str += ")";
			break;
		}
		case STRUCT_UNDEFINED: {
			print_str = _("undefined");
			break;
		}
	}
	if(o_uncertainty) {
		if(SIZE > 0) {
			print_str.insert(0, "(");
			print_str += ")";
		}
		print_str += SIGN_PLUSMINUS;
		if(o_uncertainty->size() > 0) print_str += "(";
		print_str += o_uncertainty->print(po);
		if(o_uncertainty->size() > 0) print_str += ")";
	}
	if(ips.wrap) {
		print_str.insert(0, "(");
		print_str += ")";
	}
	return print_str;
}

MathStructure &MathStructure::flattenVector(MathStructure &mstruct) const {
	if(!isVector()) {
		mstruct = *this;
		return mstruct;
	}
	MathStructure mstruct2;
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			CHILD(i).flattenVector(mstruct2);
			for(size_t i2 = 0; i2 < mstruct2.size(); i2++) {
				mstruct.addChild(mstruct2[i2]);
			}
		} else {
			mstruct.addChild(CHILD(i));
		}
	}
	return mstruct;
}
bool MathStructure::rankVector(bool ascending) {
	vector<int> ranked;
	vector<bool> ranked_equals_prev;	
	bool b;
	for(size_t index = 0; index < SIZE; index++) {
		b = false;
		for(size_t i = 0; i < ranked.size(); i++) {
			ComparisonResult cmp = CHILD(index).compare(CHILD(ranked[i]));
			if(COMPARISON_NOT_FULLY_KNOWN(cmp)) {
				CALCULATOR->error(true, _("Unsolvable comparison at element %s when trying to rank vector."), i2s(index).c_str(), NULL);
				return false;
			}
			if((ascending && cmp == COMPARISON_RESULT_GREATER) || cmp == COMPARISON_RESULT_EQUAL || (!ascending && cmp == COMPARISON_RESULT_LESS)) {
				if(cmp == COMPARISON_RESULT_EQUAL) {
					ranked.insert(ranked.begin() + i + 1, index);
					ranked_equals_prev.insert(ranked_equals_prev.begin() + i + 1, true);
				} else {
					ranked.insert(ranked.begin() + i, index);
					ranked_equals_prev.insert(ranked_equals_prev.begin() + i, false);
				}
				b = true;
				break;
			}
		}
		if(!b) {
			ranked.push_back(index);
			ranked_equals_prev.push_back(false);
		}
	}	
	int n_rep = 0;
	for(int i = (int) ranked.size() - 1; i >= 0; i--) {
		if(ranked_equals_prev[i]) {
			n_rep++;
		} else {
			if(n_rep) {
				MathStructure v(i + 1 + n_rep, 1);
				v += i + 1;
				v *= MathStructure(1, 2);
				for(; n_rep >= 0; n_rep--) {
					CHILD(ranked[i + n_rep]) = v;
				}
			} else {
				CHILD(ranked[i]).set(i + 1, 1);
			}
			n_rep = 0;
		}
	}
	return true;
}
bool MathStructure::sortVector(bool ascending) {
	vector<size_t> ranked_mstructs;
	bool b;
	for(size_t index = 0; index < SIZE; index++) {
		b = false;
		for(size_t i = 0; i < ranked_mstructs.size(); i++) {
			ComparisonResult cmp = CHILD(index).compare(*v_subs[ranked_mstructs[i]]);
			if(COMPARISON_MIGHT_BE_LESS_OR_GREATER(cmp)) {
				CALCULATOR->error(true, _("Unsolvable comparison at element %s when trying to sort vector."), i2s(index).c_str(), NULL);
				return false;
			}
			if((ascending && COMPARISON_IS_EQUAL_OR_GREATER(cmp)) || (!ascending && COMPARISON_IS_EQUAL_OR_LESS(cmp))) {
				ranked_mstructs.insert(ranked_mstructs.begin() + i, v_order[index]);
				b = true;
				break;
			}
		}
		if(!b) {
			ranked_mstructs.push_back(v_order[index]);
		}
	}	
	v_order = ranked_mstructs;
	return true;
}
MathStructure &MathStructure::getRange(int start, int end, MathStructure &mstruct) const {
	if(!isVector()) {
		if(start > 1) {
			mstruct.clear();
			return mstruct;
		} else {
			mstruct = *this;
			return mstruct;
		}
	}
	if(start < 1) start = 1;
	else if(start > (int) SIZE) {
		mstruct.clear();
		return mstruct;
	}
	if(end < (int) 1 || end > (int) SIZE) end = SIZE;
	else if(end < start) end = start;	
	mstruct.clearVector();
	for(; start <= end; start++) {
		mstruct.addChild(CHILD(start - 1));
	}
	return mstruct;
}

void MathStructure::resizeVector(size_t i, const MathStructure &mfill) {
	if(i > SIZE) {
		while(i > SIZE) {
			APPEND(mfill);
		}
	} else if(i > SIZE) {
		REDUCE(i)
	}
}

size_t MathStructure::rows() const {
	if(m_type != STRUCT_VECTOR || SIZE == 0 || (SIZE == 1 && (!CHILD(0).isVector() || CHILD(0).size() == 0))) return 0;
	return SIZE;
}
size_t MathStructure::columns() const {
	if(m_type != STRUCT_VECTOR || SIZE == 0 || !CHILD(0).isVector()) return 0;
	return CHILD(0).size();
}
const MathStructure *MathStructure::getElement(size_t row, size_t column) const {
	if(row == 0 || column == 0 || row > rows() || column > columns()) return NULL;
	if(CHILD(row - 1).size() < column) return NULL;
	return &CHILD(row - 1)[column - 1];
}
MathStructure *MathStructure::getElement(size_t row, size_t column) {
	if(row == 0 || column == 0 || row > rows() || column > columns()) return NULL;
	if(CHILD(row - 1).size() < column) return NULL;
	return &CHILD(row - 1)[column - 1];
}
MathStructure &MathStructure::getArea(size_t r1, size_t c1, size_t r2, size_t c2, MathStructure &mstruct) const {
	size_t r = rows();
	size_t c = columns();
	if(r1 < 1) r1 = 1;
	else if(r1 > r) r1 = r;
	if(c1 < 1) c1 = 1;
	else if(c1 > c) c1 = c;
	if(r2 < 1 || r2 > r) r2 = r;
	else if(r2 < r1) r2 = r1;
	if(c2 < 1 || c2 > c) c2 = c;
	else if(c2 < c1) c2 = c1;
	mstruct.clearMatrix(); mstruct.resizeMatrix(r2 - r1 + 1, c2 - c1 + 1, m_undefined);
	for(size_t index_r = r1; index_r <= r2; index_r++) {
		for(size_t index_c = c1; index_c <= c2; index_c++) {
			mstruct[index_r - r1][index_c - c1] = CHILD(index_r - 1)[index_c - 1];
		}			
	}
	return mstruct;
}
MathStructure &MathStructure::rowToVector(size_t r, MathStructure &mstruct) const {
	if(r > rows()) {
		mstruct = m_undefined;
		return mstruct;
	}
	if(r < 1) r = 1;
	mstruct = CHILD(r - 1);
	return mstruct;
}
MathStructure &MathStructure::columnToVector(size_t c, MathStructure &mstruct) const {
	if(c > columns()) {
		mstruct = m_undefined;
		return mstruct;
	}
	if(c < 1) c = 1;
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		mstruct.addChild(CHILD(i)[c - 1]);
	}
	return mstruct;
}
MathStructure &MathStructure::matrixToVector(MathStructure &mstruct) const {
	if(!isVector()) {
		mstruct = *this;
		return mstruct;
	}
	mstruct.clearVector();
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			for(size_t i2 = 0; i2 < CHILD(i).size(); i2++) {
				mstruct.addChild(CHILD(i)[i2]);
			}
		} else {
			mstruct.addChild(CHILD(i));
		}
	}
	return mstruct;
}
void MathStructure::setElement(const MathStructure &mstruct, size_t row, size_t column) {
	if(row > rows() || column > columns() || row < 1 || column < 1) return;
	CHILD(row - 1)[column - 1] = mstruct;
	CHILD(row - 1).childUpdated(column);
	CHILD_UPDATED(row - 1);
}
void MathStructure::addRows(size_t r, const MathStructure &mfill) {
	if(r == 0) return;
	size_t cols = columns();
	MathStructure mstruct; mstruct.clearVector();
	mstruct.resizeVector(cols, mfill);
	for(size_t i = 0; i < r; i++) {
		APPEND(mstruct);
	}
}
void MathStructure::addColumns(size_t c, const MathStructure &mfill) {
	if(c == 0) return;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).isVector()) {
			for(size_t i2 = 0; i2 < c; i2++) {
				CHILD(i).addChild(mfill);
			}
		}
	}
	CHILDREN_UPDATED;
}
void MathStructure::addRow(const MathStructure &mfill) {
	addRows(1, mfill);
}
void MathStructure::addColumn(const MathStructure &mfill) {
	addColumns(1, mfill);
}
void MathStructure::resizeMatrix(size_t r, size_t c, const MathStructure &mfill) {
	if(r > SIZE) {
		addRows(r - SIZE, mfill);
	} else if(r != SIZE) {
		REDUCE(r);
	}
	size_t cols = columns();
	if(c > cols) {
		addColumns(c - cols, mfill);
	} else if(c != cols) {
		for(size_t i = 0; i < SIZE; i++) {
			CHILD(i).resizeVector(c, mfill);
		}
	}
}
bool MathStructure::matrixIsSquare() const {
	return rows() == columns();
}

bool MathStructure::isNumericMatrix() const {
	if(!isMatrix()) return false;
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(index_r).size(); index_c++) {
			if(!CHILD(index_r)[index_c].isNumber() || CHILD(index_r)[index_c].isInfinity()) return false;
		}
	}
	return true;
}

//from GiNaC
int MathStructure::pivot(size_t ro, size_t co, bool symbolic) {

	size_t k = ro;
	if(symbolic) {
		while((k < SIZE) && (CHILD(k)[co].isZero())) ++k;
	} else {
		size_t kmax = k + 1;
		Number mmax(CHILD(kmax)[co].number());
		mmax.abs();
		while(kmax < SIZE) {
			if(CHILD(kmax)[co].number().isNegative()) {
				Number ntmp(CHILD(kmax)[co].number());
				ntmp.negate();
				if(ntmp.isGreaterThan(mmax)) {
					mmax = ntmp;
					k = kmax;
				}
			} else if(CHILD(kmax)[co].number().isGreaterThan(mmax)) {
				mmax = CHILD(kmax)[co].number();
				k = kmax;
			}
			++kmax;
		}
		if(!mmax.isZero()) k = kmax;
	}
	if(k == SIZE) return -1;
	if(k == ro) return 0;
	
	SWAP_CHILDREN(ro, k)
	
	return k;
	
}


//from GiNaC
void determinant_minor(const MathStructure &mtrx, MathStructure &mdet, const EvaluationOptions &eo) {
	size_t n = mtrx.size();
	if(n == 1) {
		mdet = mtrx[0][0];
		return;
	}
	if(n == 2) {
		mdet = mtrx[0][0];
		mdet.calculateMultiply(mtrx[1][1], eo);
		mdet.add(mtrx[1][0], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[0][1], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		return;
	}
	if(n == 3) {
		mdet = mtrx[0][0];
		mdet.calculateMultiply(mtrx[1][1], eo);
		mdet.calculateMultiply(mtrx[2][2], eo);
		mdet.add(mtrx[0][0], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][2], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][1], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][1], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][0], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][2], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][2], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][0], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][1], eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][1], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][2], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][0], eo);
		mdet.calculateAddLast(eo);
		mdet.add(mtrx[0][2], true);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[1][1], eo);
		mdet[mdet.size() - 1].calculateMultiply(mtrx[2][0], eo);
		mdet[mdet.size() - 1].calculateNegate(eo);
		mdet.calculateAddLast(eo);		
		return;
	}
	
	std::vector<size_t> Pkey;
	Pkey.reserve(n);
	std::vector<size_t> Mkey;
	Mkey.reserve(n - 1);
	typedef std::map<std::vector<size_t>, class MathStructure> Rmap;
	typedef std::map<std::vector<size_t>, class MathStructure>::value_type Rmap_value;
	Rmap A;
	Rmap B;
	for(size_t r = 0; r < n; ++r) {
		Pkey.erase(Pkey.begin(), Pkey.end());
		Pkey.push_back(r);
		A.insert(Rmap_value(Pkey, mtrx[r][n - 1]));
	}
	for(int c = n - 2; c >= 0; --c) {
		Pkey.erase(Pkey.begin(), Pkey.end()); 
		Mkey.erase(Mkey.begin(), Mkey.end());
		for(size_t i = 0; i < n - c; ++i) Pkey.push_back(i);
		size_t fc = 0;
		do {
			mdet.clear();
			for(size_t r = 0; r < n - c; ++r) {
				if (mtrx[Pkey[r]][c].isZero()) continue;
				Mkey.erase(Mkey.begin(), Mkey.end());
				for(size_t i = 0; i < n - c; ++i) {
					if(i != r) Mkey.push_back(Pkey[i]);
				}
				mdet.add(mtrx[Pkey[r]][c], true);
				mdet[mdet.size() - 1].calculateMultiply(A[Mkey], eo);
				if(r % 2) mdet[mdet.size() - 1].calculateNegate(eo);
				mdet.calculateAddLast(eo);
			}
			if(!mdet.isZero()) B.insert(Rmap_value(Pkey, mdet));
			for(fc = n - c; fc > 0; --fc) {
				++Pkey[fc-1];
				if(Pkey[fc - 1] < fc + c) break;
			}
			if(fc < n - c && fc > 0) {
				for(size_t j = fc; j < n - c; ++j) Pkey[j] = Pkey[j - 1] + 1;
			}
		} while(fc);
		A = B;
		B.clear();
	}	
	return;
}

//from GiNaC
int MathStructure::gaussianElimination(const EvaluationOptions &eo, bool det) {

	if(!isMatrix()) return 0;
	bool isnumeric = isNumericMatrix();

	size_t m = rows();
	size_t n = columns();
	int sign = 1;
	
	size_t r0 = 0;
	for(size_t c0 = 0; c0 < n && r0 < m - 1; ++c0) {
		int indx = pivot(r0, c0, true);
		if(indx == -1) {
			sign = 0;
			if(det) return 0;
		}
		if(indx >= 0) {
			if(indx > 0) sign = -sign;
			for(size_t r2 = r0 + 1; r2 < m; ++r2) {
				if(!CHILD(r2)[c0].isZero()) {
					if(isnumeric) {
						Number piv(CHILD(r2)[c0].number());
						piv /= CHILD(r0)[c0].number();
						for(size_t c = c0 + 1; c < n; ++c) {
							CHILD(r2)[c].number() -= piv * CHILD(r0)[c].number();
						}
					} else {
						MathStructure piv(CHILD(r2)[c0]);
						piv.calculateDivide(CHILD(r0)[c0], eo);
						for(size_t c = c0 + 1; c < n; ++c) {
							CHILD(r2)[c].add(piv, true);
							CHILD(r2)[c][CHILD(r2)[c].size() - 1].calculateMultiply(CHILD(r0)[c], eo);
							CHILD(r2)[c][CHILD(r2)[c].size() - 1].calculateNegate(eo);
							CHILD(r2)[c].calculateAddLast(eo);
						}
					}
				}
				for(size_t c = r0; c <= c0; ++c) CHILD(r2)[c].clear();
			}
			if(det) {
				for(size_t c = r0 + 1; c < n; ++c) CHILD(r0)[c].clear();
			}
			++r0;
		}
	}
	for(size_t r = r0 + 1; r < m; ++r) {
		for(size_t c = 0; c < n; ++c) CHILD(r)[c].clear();
	}

	return sign;
}

//from GiNaC
template <class It>
int permutation_sign(It first, It last)
{
	if (first == last)
		return 0;
	--last;
	if (first == last)
		return 0;
	It flag = first;
	int sign = 1;

	do {
		It i = last, other = last;
		--other;
		bool swapped = false;
		while (i != first) {
			if (*i < *other) {
				std::iter_swap(other, i);
				flag = other;
				swapped = true;
				sign = -sign;
			} else if (!(*other < *i))
				return 0;
			--i; --other;
		}
		if (!swapped)
			return sign;
		++flag;
		if (flag == last)
			return sign;
		first = flag;
		i = first; other = first;
		++other;
		swapped = false;
		while (i != last) {
			if (*other < *i) {
				std::iter_swap(i, other);
				flag = other;
				swapped = true;
				sign = -sign;
			} else if (!(*i < *other))
				return 0;
			++i; ++other;
		}
		if (!swapped)
			return sign;
		last = flag;
		--last;
	} while (first != last);

	return sign;
}

//from GiNaC
MathStructure &MathStructure::determinant(MathStructure &mstruct, const EvaluationOptions &eo) const {

	if(!matrixIsSquare()) {
		CALCULATOR->error(true, _("The determinant can only be calculated for square matrices."), NULL);
		mstruct = m_undefined;
		return mstruct;
	}

	if(SIZE == 1) {
		mstruct = CHILD(0)[0];
	} else if(isNumericMatrix()) {
	
		mstruct.set(1, 1);
		MathStructure mtmp(*this);
		int sign = mtmp.gaussianElimination(eo, true);
		for(size_t d = 0; d < SIZE; ++d) {
			mstruct.number() *= mtmp[d][d].number();
		}
		mstruct.number() *= sign;
		
	} else {

		typedef std::pair<size_t, size_t> sizet_pair;
		std::vector<sizet_pair> c_zeros;
		for(size_t c = 0; c < CHILD(0).size(); ++c) {
			size_t acc = 0;
			for(size_t r = 0; r < SIZE; ++r) {
				if(CHILD(r)[c].isZero()) ++acc;
			}
			c_zeros.push_back(sizet_pair(acc, c));
		}

		std::sort(c_zeros.begin(), c_zeros.end());
		std::vector<size_t> pre_sort;
		for(std::vector<sizet_pair>::const_iterator i = c_zeros.begin(); i != c_zeros.end(); ++i) {
			pre_sort.push_back(i->second);
		}
		std::vector<size_t> pre_sort_test(pre_sort);

		int sign = permutation_sign(pre_sort_test.begin(), pre_sort_test.end());

		MathStructure result;
		result.clearMatrix();
		result.resizeMatrix(SIZE, CHILD(0).size(), m_zero);

		size_t c = 0;
		for(std::vector<size_t>::const_iterator i = pre_sort.begin(); i != pre_sort.end(); ++i,++c) {
			for(size_t r = 0; r < SIZE; ++r) result[r][c] = CHILD(r)[(*i)];
		}
		mstruct.clear();

		determinant_minor(result, mstruct, eo);

		if(sign != 1) {
			mstruct.calculateMultiply(sign, eo);
		}
		
	}
	
	mstruct.mergePrecision(*this);
	return mstruct;
	
}

MathStructure &MathStructure::permanent(MathStructure &mstruct, const EvaluationOptions &eo) const {
	if(!matrixIsSquare()) {
		CALCULATOR->error(true, _("The permanent can only be calculated for square matrices."), NULL);
		mstruct = m_undefined;
		return mstruct;
	}
	if(b_approx) mstruct.setApproximate();
	mstruct.setPrecision(i_precision);
	if(SIZE == 1) {
		if(CHILD(0).size() >= 1) {	
			mstruct == CHILD(0)[0];
		}
	} else if(SIZE == 2) {
		mstruct = CHILD(0)[0];
		if(IS_REAL(mstruct) && IS_REAL(CHILD(1)[1])) {
			mstruct.number() *= CHILD(1)[1].number();
		} else {
			mstruct.calculateMultiply(CHILD(1)[1], eo);
		}
		if(IS_REAL(mstruct) && IS_REAL(CHILD(1)[0]) && IS_REAL(CHILD(0)[1])) {
			mstruct.number() += CHILD(1)[0].number() * CHILD(0)[1].number();
		} else {
			MathStructure mtmp = CHILD(1)[0];
			mtmp.calculateMultiply(CHILD(0)[1], eo);
			mstruct.calculateAdd(mtmp, eo);
		}
	} else {
		MathStructure mtrx;
		mtrx.clearMatrix();
		mtrx.resizeMatrix(SIZE - 1, CHILD(0).size() - 1, m_undefined);
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			for(size_t index_r2 = 1; index_r2 < SIZE; index_r2++) {
				for(size_t index_c2 = 0; index_c2 < CHILD(index_r2).size(); index_c2++) {
					if(index_c2 > index_c) {
						mtrx.setElement(CHILD(index_r2)[index_c2], index_r2, index_c2);
					} else if(index_c2 < index_c) {
						mtrx.setElement(CHILD(index_r2)[index_c2], index_r2, index_c2 + 1);
					}
				}
			}
			MathStructure mdet;
			mtrx.permanent(mdet, eo);
			if(IS_REAL(mdet) && IS_REAL(CHILD(0)[index_c])) {
				mdet.number() *= CHILD(0)[index_c].number();
			} else {
				mdet.calculateMultiply(CHILD(0)[index_c], eo);
			}
			if(IS_REAL(mdet) && IS_REAL(mstruct)) {
				mstruct.number() += mdet.number();
			} else {
				mstruct.calculateAdd(mdet, eo);
			}
		}
	}
	return mstruct;
}
void MathStructure::setToIdentityMatrix(size_t n) {
	clearMatrix();
	resizeMatrix(n, n, m_zero);
	for(size_t i = 0; i < n; i++) {
		CHILD(i)[i] = m_one;
	}
}
MathStructure &MathStructure::getIdentityMatrix(MathStructure &mstruct) const {
	mstruct.setToIdentityMatrix(columns());
	return mstruct;
}

//modified algorithm from eigenmath
bool MathStructure::invertMatrix(const EvaluationOptions &eo) {

	if(!matrixIsSquare()) return false;
	
	if(isNumericMatrix()) {
	
		int d, i, j, n = SIZE;

		MathStructure idstruct;
		Number mtmp;
		idstruct.setToIdentityMatrix(n);
		MathStructure mtrx(*this);

		for(d = 0; d < n; d++) {

			if(mtrx[d][d].isZero()) {

				for (i = d + 1; i < n; i++) {
					if(!mtrx[i][d].isZero()) break;
				}

				if(i == n) {
					CALCULATOR->error(true, _("Inverse of singular matrix."), NULL);
					return false;
				}

				mtrx[i].ref();
				mtrx[d].ref();
				MathStructure *mptr = &mtrx[d];
				mtrx.setChild_nocopy(&mtrx[i], d + 1);
				mtrx.setChild_nocopy(mptr, i + 1);
			
				idstruct[i].ref();
				idstruct[d].ref();
				mptr = &idstruct[d];
				idstruct.setChild_nocopy(&idstruct[i], d + 1);
				idstruct.setChild_nocopy(mptr, i + 1);
				
			}

			mtmp = mtrx[d][d].number();
			mtmp.recip();

			for(j = 0; j < n; j++) {

				if(j > d) {
					mtrx[d][j].number() *= mtmp;
				}

				idstruct[d][j].number() *= mtmp;
			
			}

			for(i = 0; i < n; i++) {

				if(i == d) continue;

				mtmp = mtrx[i][d].number();
				mtmp.negate();

				for(j = 0; j < n; j++) {
				
					if(j > d) {		
						mtrx[i][j].number() += mtrx[d][j].number() * mtmp;
					}
				
					idstruct[i][j].number() += idstruct[d][j].number() * mtmp;

				}
			}
		}
		set_nocopy(idstruct);
		MERGE_APPROX_AND_PREC(idstruct)
	} else {
		MathStructure *mstruct = new MathStructure();
		determinant(*mstruct, eo);
		mstruct->calculateInverse(eo);
		adjointMatrix(eo);
		multiply_nocopy(mstruct, true);
		calculateMultiplyLast(eo);
	}
	return true;
}

bool MathStructure::adjointMatrix(const EvaluationOptions &eo) {
	if(!matrixIsSquare()) return false;
	MathStructure msave(*this);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			msave.cofactor(index_r + 1, index_c + 1, CHILD(index_r)[index_c], eo);
		}
	}
	transposeMatrix();
	return true;
}
bool MathStructure::transposeMatrix() {
	MathStructure msave(*this);
	resizeMatrix(CHILD(0).size(), SIZE, m_undefined);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
			CHILD(index_r)[index_c] = msave[index_c][index_r];
		}
	}
	return true;
}	
MathStructure &MathStructure::cofactor(size_t r, size_t c, MathStructure &mstruct, const EvaluationOptions &eo) const {
	if(r < 1) r = 1;
	if(c < 1) c = 1;
	if(r > SIZE || c > CHILD(0).size()) {
		mstruct = m_undefined;
		return mstruct;
	}
	r--; c--;
	mstruct.clearMatrix(); mstruct.resizeMatrix(SIZE - 1, CHILD(0).size() - 1, m_undefined);
	for(size_t index_r = 0; index_r < SIZE; index_r++) {
		if(index_r != r) {
			for(size_t index_c = 0; index_c < CHILD(0).size(); index_c++) {
				if(index_c > c) {
					if(index_r > r) {
						mstruct[index_r - 1][index_c - 1] = CHILD(index_r)[index_c];
					} else {
						mstruct[index_r][index_c - 1] = CHILD(index_r)[index_c];
					}
				} else if(index_c < c) {
					if(index_r > r) {
						mstruct[index_r - 1][index_c] = CHILD(index_r)[index_c];
					} else {
						mstruct[index_r][index_c] = CHILD(index_r)[index_c];
					}
				}
			}
		}
	}	
	MathStructure mstruct2;
	mstruct = mstruct.determinant(mstruct2, eo);
	if((r + c) % 2 == 1) {
		mstruct.calculateNegate(eo);
	}
	return mstruct;
}

void gatherInformation(const MathStructure &mstruct, vector<Unit*> &base_units, vector<AliasUnit*> &alias_units) {
	switch(mstruct.type()) {
		case STRUCT_UNIT: {
			switch(mstruct.unit()->subtype()) {
				case SUBTYPE_BASE_UNIT: {
					for(size_t i = 0; i < base_units.size(); i++) {
						if(base_units[i] == mstruct.unit()) {
							return;
						}
					}
					base_units.push_back(mstruct.unit());
					break;
				}
				case SUBTYPE_ALIAS_UNIT: {
					for(size_t i = 0; i < alias_units.size(); i++) {
						if(alias_units[i] == mstruct.unit()) {
							return;
						}
					}
					alias_units.push_back((AliasUnit*) (mstruct.unit()));
					break;
				}
				case SUBTYPE_COMPOSITE_UNIT: {
					gatherInformation(((CompositeUnit*) (mstruct.unit()))->generateMathStructure(), base_units, alias_units);
					break;
				}				
			}
			break;
		}
		case STRUCT_FUNCTION: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(!mstruct.function()->getArgumentDefinition(i + 1) || mstruct.function()->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) {
					gatherInformation(mstruct[i], base_units, alias_units);
				}
			}
		}
		default: {
			for(size_t i = 0; i < mstruct.size(); i++) {
				gatherInformation(mstruct[i], base_units, alias_units);
			}
			break;
		}
	}

}

int MathStructure::isUnitCompatible(const MathStructure &mstruct) {
	int b1 = mstruct.containsRepresentativeOfType(STRUCT_UNIT, true, true);
	int b2 = containsRepresentativeOfType(STRUCT_UNIT, true, true);
	if(b1 < 0 || b2 < 0) return -1;
	if(b1 != b2) return false;
	if(!b1) return true;
	if(mstruct.isMultiplication()) {
		size_t i2 = 0;
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).containsType(STRUCT_UNIT)) {
				bool b = false;
				for(; i2 < mstruct.size(); i2++) {
					if(mstruct[i2].containsType(STRUCT_UNIT)) {
						if(!CHILD(i).isUnitCompatible(mstruct[i2])) {
							return false;
						}
						i2++;
						b = true;
						break;
					}
				}
				if(!b) return false;
			}
		}
		for(; i2 < mstruct.size(); i2++) {
			if(mstruct[i2].containsType(STRUCT_UNIT)) {
				return false;
			}	
		}
	}
	if(isUnit()) {
		return equals(mstruct);
	} else if(isPower()) {
		return equals(mstruct);
	}
	return true;
}

bool MathStructure::syncUnits(bool sync_complex_relations, bool *found_complex_relations, bool calculate_new_functions, const EvaluationOptions &feo) {
	vector<Unit*> base_units;
	vector<AliasUnit*> alias_units;
	vector<CompositeUnit*> composite_units;	
	gatherInformation(*this, base_units, alias_units);
	CompositeUnit *cu;
	bool b = false, do_erase = false;
	for(size_t i = 0; i < alias_units.size(); ) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			b = false;
			cu = (CompositeUnit*) alias_units[i]->baseUnit();
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(cu->containsRelativeTo(base_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);					
					do_erase = true;
					break;
				}
			}
			for(size_t i2 = 0; !do_erase && i2 < alias_units.size(); i2++) {
				if(i != i2 && cu->containsRelativeTo(alias_units[i2])) {
					for(size_t i3 = 0; i3 < composite_units.size(); i3++) {
						if(composite_units[i3] == cu) {
							b = true;
							break;
						}
					}
					if(!b) composite_units.push_back(cu);
					do_erase = true;
					break;
				}
			}					
		}
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			for(int i2 = 1; i2 <= (int) cu->countUnits(); i2++) {
				b = false;
				Unit *cub = cu->get(i2);
				switch(cub->subtype()) {
					case SUBTYPE_BASE_UNIT: {
						for(size_t i3 = 0; i3 < base_units.size(); i3++) {
							if(base_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) base_units.push_back(cub);
						break;
					}
					case SUBTYPE_ALIAS_UNIT: {
						for(size_t i3 = 0; i3 < alias_units.size(); i3++) {
							if(alias_units[i3] == cub) {
								b = true;
								break;
							}
						}
						if(!b) alias_units.push_back((AliasUnit*) cub);
						break;
					}
					case SUBTYPE_COMPOSITE_UNIT: {
						gatherInformation(((CompositeUnit*) cub)->generateMathStructure(), base_units, alias_units);
						break;
					}
				}
			}
			i = 0;
		} else {
			i++;
		}
	}
	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		for(size_t i2 = 0; i2 < alias_units.size(); i2++) {
			if(i != i2 && alias_units[i]->baseUnit() == alias_units[i2]->baseUnit()) { 
				if(alias_units[i2]->isParentOf(alias_units[i])) {
					do_erase = true;
					break;
				} else if(!alias_units[i]->isParentOf(alias_units[i2])) {
					b = false;
					for(size_t i3 = 0; i3 < base_units.size(); i3++) {
						if(base_units[i3] == alias_units[i2]->firstBaseUnit()) {
							b = true;
							break;
						}
					}
					if(!b) base_units.push_back((Unit*) alias_units[i]->baseUnit());
					do_erase = true;
					break;
				}
			}
		} 
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}	
	for(size_t i = 0; i < alias_units.size();) {
		do_erase = false;
		if(alias_units[i]->baseUnit()->subtype() == SUBTYPE_BASE_UNIT) {
			for(size_t i2 = 0; i2 < base_units.size(); i2++) {
				if(alias_units[i]->baseUnit() == base_units[i2]) {
					do_erase = true;
					break;
				}
			}
		} 
		if(do_erase) {
			alias_units.erase(alias_units.begin() + i);
			i = 0;
		} else {
			i++;
		}
	}
	b = false;
	for(size_t i = 0; i < composite_units.size(); i++) {	
		if(convert(composite_units[i], sync_complex_relations, found_complex_relations, calculate_new_functions, feo)) b = true;
	}	
	if(dissolveAllCompositeUnits()) b = true;
	for(size_t i = 0; i < base_units.size(); i++) {	
		if(convert(base_units[i], sync_complex_relations, found_complex_relations, calculate_new_functions, feo)) b = true;
	}
	for(size_t i = 0; i < alias_units.size(); i++) {	
		if(convert(alias_units[i], sync_complex_relations, found_complex_relations, calculate_new_functions, feo)) b = true;
	}
	return b;
}
bool MathStructure::testDissolveCompositeUnit(Unit *u) {
	if(m_type == STRUCT_UNIT) {
		if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) o_unit)->containsRelativeTo(u)) {
				set(((CompositeUnit*) o_unit)->generateMathStructure());
				return true;
			}
		} else if(o_unit->subtype() == SUBTYPE_ALIAS_UNIT && o_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (o_unit->baseUnit()))->containsRelativeTo(u)) {
				convert(o_unit->baseUnit());
				convert(u);
				return true;
			}		
		}
	}
	return false; 
}
bool MathStructure::testCompositeUnit(Unit *u) {
	if(m_type == STRUCT_UNIT) {
		if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) o_unit)->containsRelativeTo(u)) {
				return true;
			}
		} else if(o_unit->subtype() == SUBTYPE_ALIAS_UNIT && o_unit->baseUnit()->subtype() == SUBTYPE_COMPOSITE_UNIT) {
			if(((CompositeUnit*) (o_unit->baseUnit()))->containsRelativeTo(u)) {
				return true;
			}		
		}
	}
	return false; 
}
bool MathStructure::dissolveAllCompositeUnits() {
	switch(m_type) {
		case STRUCT_UNIT: {
			if(o_unit->subtype() == SUBTYPE_COMPOSITE_UNIT) {
				set(((CompositeUnit*) o_unit)->generateMathStructure());
				return true;
			}
			break;
		}
		default: {
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).dissolveAllCompositeUnits()) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}		
	}		
	return false;
}
bool MathStructure::convert(Unit *u, bool convert_complex_relations, bool *found_complex_relations, bool calculate_new_functions, const EvaluationOptions &feo) {
	bool b = false;	
	if(m_type == STRUCT_UNIT && o_unit == u) return false;
	if(u->subtype() == SUBTYPE_COMPOSITE_UNIT && !(m_type == STRUCT_UNIT && o_unit->baseUnit() == u)) {
		return convert(((CompositeUnit*) u)->generateMathStructure(), convert_complex_relations, found_complex_relations, calculate_new_functions, feo);
	}
	if(m_type == STRUCT_UNIT) {
		if(!convert_complex_relations && u->hasComplexRelationTo(o_unit)) {
			if(found_complex_relations) *found_complex_relations = true;
			return false;
		}
		if(testDissolveCompositeUnit(u)) {
			convert(u, convert_complex_relations, found_complex_relations, calculate_new_functions, feo);
			return true;
		}
		MathStructure *exp = new MathStructure(1, 1);
		MathStructure *mstruct = new MathStructure(1, 1);
		;
		if(u->convert(o_unit, *mstruct, *exp)) {
			o_unit = u;
			if(!exp->isOne()) {
				if(calculate_new_functions) exp->calculateFunctions(feo, true);
				raise_nocopy(exp);
			} else {
				exp->unref();
			}
			if(!mstruct->isOne()) {
				if(calculate_new_functions) mstruct->calculateFunctions(feo, true);
				multiply_nocopy(mstruct);
			} else {
				mstruct->unref();
			}
			return true;
		} else {
			exp->unref();
			mstruct->unref();
			return false;
		}
	} else {
		if(convert_complex_relations) {
			if(m_type == STRUCT_MULTIPLICATION) {
				int b_c = -1;
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).isUnit_exp()) {
						if((CHILD(i).isUnit() && u->hasComplexRelationTo(CHILD(i).unit())) || (CHILD(i).isPower() && u->hasComplexRelationTo(CHILD(i)[0].unit()))) {
							if(found_complex_relations) *found_complex_relations = true;
							if(b_c >= 0) {
								b_c = -1;
								break;
							}
							b_c = i;
						}
					}
				}
				if(b_c >= 0) {
					MathStructure mstruct(1, 1);
					if(SIZE == 2) {
						if(b_c == 0) mstruct = CHILD(1);
						else mstruct = CHILD(0);
					} else if(SIZE > 2) {
						mstruct = *this;
						mstruct.delChild(b_c + 1);
					}
					MathStructure exp(1, 1);
					Unit *u2;
					if(CHILD(b_c).isPower()) {
						if(CHILD(b_c)[0].testDissolveCompositeUnit(u)) {
							convert(u, convert_complex_relations, found_complex_relations, calculate_new_functions, feo);
							return true;
						}	
						exp = CHILD(b_c)[1];
						u2 = CHILD(b_c)[0].unit();
					} else {
						if(CHILD(b_c).testDissolveCompositeUnit(u)) {
							convert(u, convert_complex_relations, found_complex_relations, calculate_new_functions, feo);
							return true;
						}
						u2 = CHILD(b_c).unit();
					}
					size_t efc = 0, mfc = 0;
					if(calculate_new_functions) {
						efc = exp.countFunctions();
						mfc = mstruct.countFunctions();
					}
					if(u->convert(u2, mstruct, exp)) {
						set(u);
						if(!exp.isOne()) {
							if(calculate_new_functions && exp.countFunctions() > efc) exp.calculateFunctions(feo, true);
							raise(exp);
						}
						if(!mstruct.isOne()) {
							if(calculate_new_functions && mstruct.countFunctions() > mfc) mstruct.calculateFunctions(feo, true);
							multiply(mstruct);
						}
						return true;
					}
					return false;
				}
			} else if(m_type == STRUCT_POWER) {
				if(CHILD(0).isUnit() && u->hasComplexRelationTo(CHILD(0).unit())) {
					if(found_complex_relations) *found_complex_relations = true;
					if(CHILD(0).testDissolveCompositeUnit(u)) {
						convert(u, convert_complex_relations, found_complex_relations, calculate_new_functions, feo);
						return true;
					}	
					MathStructure exp(CHILD(1));
					MathStructure mstruct(1, 1);
					size_t efc = 0;
					if(calculate_new_functions) {
						efc = exp.countFunctions();
					}
					if(u->convert(CHILD(0).unit(), mstruct, exp)) {
						set(u);
						if(!exp.isOne()) {
							if(calculate_new_functions && exp.countFunctions() > efc) exp.calculateFunctions(feo, true);
							raise(exp);
						}
						if(!mstruct.isOne()) {
							if(calculate_new_functions) mstruct.calculateFunctions(feo, true);
							multiply(mstruct);
						}
						return true;
					}
					return false;
				}
			}
			if(m_type == STRUCT_MULTIPLICATION || m_type == STRUCT_POWER) {
				for(size_t i = 0; i < SIZE; i++) {
					if(CHILD(i).convert(u, false, found_complex_relations, calculate_new_functions, feo)) {
						CHILD_UPDATED(i);
						b = true;
					}
				}
				return b;
			}
		}
		if(m_type == STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				if((!o_function->getArgumentDefinition(i + 1) || o_function->getArgumentDefinition(i + 1)->type() != ARGUMENT_TYPE_ANGLE) && CHILD(i).convert(u, convert_complex_relations, found_complex_relations, calculate_new_functions, feo)) {
					CHILD_UPDATED(i);
					b = true;
				}
			}
			return b;
		}
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).convert(u, convert_complex_relations, found_complex_relations, calculate_new_functions, feo)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
		return b;
	}	
	return b;
}
bool MathStructure::convert(const MathStructure unit_mstruct, bool convert_complex_relations, bool *found_complex_relations, bool calculate_new_functions, const EvaluationOptions &feo) {
	bool b = false;
	if(unit_mstruct.type() == STRUCT_UNIT) {
		if(convert(unit_mstruct.unit(), convert_complex_relations, found_complex_relations, calculate_new_functions, feo)) b = true;
	} else {
		for(size_t i = 0; i < unit_mstruct.size(); i++) {
			if(convert(unit_mstruct[i], convert_complex_relations, found_complex_relations, calculate_new_functions, feo)) b = true;
		}
	}	
	return b;
}

int MathStructure::contains(const MathStructure &mstruct, bool structural_only, bool check_variables, bool check_functions) const {
	if(equals(mstruct)) return 1;
	if(structural_only) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).contains(mstruct)) return 1;
		}
	} else {
		int ret = 0;
		if(m_type != STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).contains(mstruct, structural_only, check_variables, check_functions);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().contains(mstruct, structural_only, check_variables, check_functions);
		} else if(m_type == STRUCT_FUNCTION && check_functions) {
			if(function_value) {
				return function_value->contains(mstruct, structural_only, check_variables, check_functions);
			}
			return -1;
		}
		return ret;
	}
	return 0;
}
int MathStructure::containsRepresentativeOf(const MathStructure &mstruct, bool check_variables, bool check_functions) const {
	if(equals(mstruct)) return 1;
	int ret = 0;
	if(m_type != STRUCT_FUNCTION) {
		for(size_t i = 0; i < SIZE; i++) {
			int retval = CHILD(i).containsRepresentativeOf(mstruct, check_variables, check_functions);
			if(retval == 1) return 1;
			else if(retval < 0) ret = retval;
		}
	}
	if(m_type == STRUCT_VARIABLE && check_variables && o_variable->isKnown()) {
		return ((KnownVariable*) o_variable)->get().containsRepresentativeOf(mstruct, check_variables, check_functions);
	} else if(m_type == STRUCT_FUNCTION && check_functions) {
		if(function_value) {
			return function_value->containsRepresentativeOf(mstruct, check_variables, check_functions);
		}
		return -1;
	}
	return ret;
}

int MathStructure::containsType(StructureType mtype, bool structural_only, bool check_variables, bool check_functions) const {
	if(m_type == mtype) return 1;
	if(structural_only) {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).containsType(mtype, true, check_variables, check_functions)) return 1;
		}
		return 0;
	} else {
		int ret = 0;
		if(m_type != STRUCT_FUNCTION) {
			for(size_t i = 0; i < SIZE; i++) {
				int retval = CHILD(i).containsType(mtype, false, check_variables, check_functions);
				if(retval == 1) return 1;
				else if(retval < 0) ret = retval;
			}
		}
		if(check_variables && m_type == STRUCT_VARIABLE && o_variable->isKnown()) {
			return ((KnownVariable*) o_variable)->get().containsType(mtype, false, check_variables, check_functions);
		} else if(check_functions && m_type == STRUCT_FUNCTION) {
			if(function_value) {
				return function_value->containsType(mtype, false, check_variables, check_functions);
			}
			return -1;
		}
		return ret;
	}
}
int MathStructure::containsRepresentativeOfType(StructureType mtype, bool check_variables, bool check_functions) const {
	if(m_type == mtype) return 1;
	int ret = 0;
	if(m_type != STRUCT_FUNCTION) {
		for(size_t i = 0; i < SIZE; i++) {
			int retval = CHILD(i).containsRepresentativeOfType(mtype, check_variables, check_functions);
			if(retval == 1) return 1;
			else if(retval < 0) ret = retval;
		}
	}
	if(check_variables && m_type == STRUCT_VARIABLE && o_variable->isKnown()) {
		return ((KnownVariable*) o_variable)->get().containsRepresentativeOfType(mtype, check_variables, check_functions);
	} else if(check_functions && m_type == STRUCT_FUNCTION) {
		if(function_value) {
			return function_value->containsRepresentativeOfType(mtype, check_variables, check_functions);
		}
	}
	if(m_type == STRUCT_SYMBOLIC || m_type == STRUCT_VARIABLE || m_type == STRUCT_FUNCTION) {
		if(representsNumber(false)) {
			return m_type == STRUCT_NUMBER;
		} else {
			return -1;
		}
	}
	return ret;
}
bool MathStructure::containsUnknowns() const {
	if(m_type == STRUCT_SYMBOLIC || (m_type == STRUCT_VARIABLE && !o_variable->isKnown())) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).containsUnknowns()) return true;
	}
	return false;
}
bool MathStructure::containsDivision() const {
	if(m_type == STRUCT_DIVISION || m_type == STRUCT_INVERSE || (m_type == STRUCT_POWER && CHILD(1).hasNegativeSign())) return true;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).containsDivision()) return true;
	}
	return false;
}
size_t MathStructure::countFunctions(bool count_subfunctions) const {
	size_t c = 0;
	if(isFunction()) {
		if(!count_subfunctions) return 1;
		c = 1;
	}
	for(size_t i = 0; i < SIZE; i++) {
		c += CHILD(i).countFunctions();
	}
	return c;
}
void MathStructure::findAllUnknowns(MathStructure &unknowns_vector) {
	if(!unknowns_vector.isVector()) unknowns_vector.clearVector();
	switch(m_type) {
		case STRUCT_VARIABLE: {
			if(o_variable->isKnown()) {
				break;
			}
		}
		case STRUCT_SYMBOLIC: {
			bool b = false;
			for(size_t i = 0; i < unknowns_vector.size(); i++) {
				if(equals(unknowns_vector[i])) {
					b = true;
					break;
				}
			}
			if(!b) unknowns_vector.addChild(*this);
			break;
		}
		default: {
			for(size_t i = 0; i < SIZE; i++) {
				CHILD(i).findAllUnknowns(unknowns_vector);
			}
		}
	}
}
bool MathStructure::replace(const MathStructure &mfrom, const MathStructure &mto) {
	if(b_protected) b_protected = false;
	if(equals(mfrom)) {
		set(mto);
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).replace(mfrom, mto)) {
			b = true;
			CHILD_UPDATED(i);
		}
	}
	return b;
}
bool MathStructure::calculateReplace(const MathStructure &mfrom, const MathStructure &mto, const EvaluationOptions &eo) {
	if(equals(mfrom)) {
		set(mto);
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).calculateReplace(mfrom, mto, eo)) {
			b = true;
			CHILD_UPDATED(i);
		}
	}
	if(b) {
		calculatesub(eo, eo, false);
	}
	return b;
}

bool MathStructure::replace(const MathStructure &mfrom1, const MathStructure &mto1, const MathStructure &mfrom2, const MathStructure &mto2) {
	if(equals(mfrom1)) {
		set(mto1);
		return true;
	}
	if(equals(mfrom2)) {
		set(mto2);
		return true;
	}
	bool b = false;
	for(size_t i = 0; i < SIZE; i++) {
		if(CHILD(i).replace(mfrom1, mto1, mfrom2, mto2)) {
			b = true;
			CHILD_UPDATED(i);
		}
	}
	return b;
}
bool MathStructure::removeType(StructureType mtype) {
	if(m_type == mtype || (m_type == STRUCT_POWER && CHILD(0).type() == mtype)) {
		set(1);
		return true;
	}
	bool b = false;
	if(m_type == STRUCT_MULTIPLICATION) {
		for(int i = 0; i < (int) SIZE; i++) {
			if(CHILD(i).removeType(mtype)) {
				if(CHILD(i).isOne()) {
					ERASE(i);
					i--;
				} else {
					CHILD_UPDATED(i);
				}
				b = true;
			}
		}
		if(SIZE == 0) {
			set(1);
		} else if(SIZE == 1) {
			setToChild(1, true);
		}
	} else {
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).removeType(mtype)) {
				b = true;
				CHILD_UPDATED(i);
			}
		}
	}
	return b;
}

MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, int steps, MathStructure *x_vector, const EvaluationOptions &eo) const {
	if(steps < 1) {
		steps = 1;
	}
	MathStructure x_value(min);
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	MathStructure step(max);
	step.calculateSubtract(min, eo);
	step.calculateDivide(steps-1, eo);
	if(!step.isNumber() || step.number().isNegative()) {
		return y_vector;
	}
	for(int i = 0; i < steps; i++) {
		if(x_vector) {
			x_vector->addChild(x_value);
		}
		y_value = *this;
		y_value.replace(x_mstruct, x_value);
		y_value.eval(eo);
		y_vector.addChild(y_value);
		x_value.calculateAdd(step, eo);
	}
	return y_vector;
}
MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &min, const MathStructure &max, const MathStructure &step, MathStructure *x_vector, const EvaluationOptions &eo) const {
	MathStructure x_value(min);
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	if(min != max) {
		MathStructure mtest(max);
		mtest.calculateSubtract(min, eo);
		if(!min.isZero()) mtest.calculateDivide(min, eo);
		if(!mtest.isNumber() || mtest.number().isNegative()) {
			return y_vector;
		}
	}
	ComparisonResult cr = max.compare(x_value);
	while(COMPARISON_IS_EQUAL_OR_LESS(cr)) {
		if(x_vector) {
			x_vector->addChild(x_value);
		}
		y_value = *this;
		y_value.replace(x_mstruct, x_value);
		y_value.eval(eo);
		y_vector.addChild(y_value);
		x_value.calculateAdd(step, eo);
		if(cr == COMPARISON_RESULT_EQUAL) break;
		cr = max.compare(x_value);
	}
	return y_vector;
}
MathStructure MathStructure::generateVector(MathStructure x_mstruct, const MathStructure &x_vector, const EvaluationOptions &eo) const {
	MathStructure y_value;
	MathStructure y_vector;
	y_vector.clearVector();
	for(size_t i = 1; i <= x_vector.countChildren(); i++) {
		y_value = *this;
		y_value.replace(x_mstruct, x_vector.getChild(i));
		y_value.eval(eo);
		y_vector.addChild(y_value);
	}
	return y_vector;
}

bool MathStructure::differentiate(const MathStructure &x_var, const EvaluationOptions &eo) {
	if(equals(x_var)) {
		set(m_one);
		return true;
	}
	switch(m_type) {
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).differentiate(x_var, eo)) {
					CHILD_UPDATED(i);
				}
			}
			break;
		}
		case STRUCT_BITWISE_AND: {}
		case STRUCT_BITWISE_OR: {}
		case STRUCT_BITWISE_XOR: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_COMPARISON: {}
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			clear(true);
			break;
		}
		case STRUCT_POWER: {
			if(SIZE < 1) {
				clear(true);
				break;
			} else if(SIZE < 2) {
				setToChild(1, true);
				return differentiate(x_var, eo);
			}
			bool x_in_base = CHILD(0).containsRepresentativeOf(x_var, true, true) != 0;
			bool x_in_exp = CHILD(1).containsRepresentativeOf(x_var, true, true) != 0;
			if(x_in_base && !x_in_exp) {
				MathStructure exp_mstruct(CHILD(1));
				MathStructure base_mstruct(CHILD(0));
				CHILD(1) += m_minus_one;
				multiply(exp_mstruct);
				base_mstruct.differentiate(x_var, eo);
				multiply(base_mstruct);
			} else if(!x_in_base && x_in_exp) {
				MathStructure exp_mstruct(CHILD(1));
				MathStructure mstruct(CALCULATOR->f_ln, &CHILD(0), NULL);
				multiply(mstruct);
				exp_mstruct.differentiate(x_var, eo);
				multiply(exp_mstruct);
			} else if(x_in_base && x_in_exp) {
				MathStructure exp_mstruct(CHILD(1));
				MathStructure base_mstruct(CHILD(0));
				exp_mstruct.differentiate(x_var, eo);
				base_mstruct.differentiate(x_var, eo);
				base_mstruct /= CHILD(0);
				base_mstruct *= CHILD(1);
				MathStructure mstruct(CALCULATOR->f_ln, &CHILD(0), NULL);
				mstruct *= exp_mstruct;
				mstruct += base_mstruct;
				multiply(mstruct);
			} else {
				clear(true);
			}
			break;
		}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_ln && SIZE == 1) {
				MathStructure mstruct(CHILD(0));
				setToChild(1, true);
				inverse();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
			} else if(o_function == CALCULATOR->f_lambert_w && SIZE == 1 && CHILD(0) == x_var) {
				MathStructure *mstruct = new MathStructure(*this);
				mstruct->add(m_one);
				mstruct->multiply(CHILD(0));
				divide_nocopy(mstruct);
			} else if(o_function == CALCULATOR->f_sin && SIZE == 1) {
				o_function = CALCULATOR->f_cos;
				MathStructure mstruct(CHILD(0));
				mstruct.differentiate(x_var, eo);
				multiply(mstruct);
				if(CALCULATOR->getRadUnit()) divide(CALCULATOR->getRadUnit());
			} else if(o_function == CALCULATOR->f_cos && SIZE == 1) {
				o_function = CALCULATOR->f_sin;
				MathStructure mstruct(CHILD(0));
				negate();
				mstruct.differentiate(x_var, eo);
				multiply(mstruct, true);
				if(CALCULATOR->getRadUnit()) divide(CALCULATOR->getRadUnit());
			} else if(o_function == CALCULATOR->f_integrate && SIZE == 2 && CHILD(1) == x_var) {
				setToChild(1, true);
			} else if(o_function == CALCULATOR->f_diff && SIZE == 3 && CHILD(1) == x_var) {
				CHILD(2) += m_one;
			} else if(o_function == CALCULATOR->f_diff && SIZE == 2 && CHILD(1) == x_var) {
				APPEND(MathStructure(2, 1));
			} else if(o_function == CALCULATOR->f_diff && SIZE == 1 && x_var == (Variable*) CALCULATOR->v_x) {
				APPEND(x_var);
				APPEND(MathStructure(2, 1));
			} else {
				if(!eo.calculate_functions || !calculateFunctions(eo)) {
					MathStructure mstruct(CALCULATOR->f_diff, this, &x_var, &m_one, NULL);
					set(mstruct);
					return false;
				} else {
					EvaluationOptions eo2 = eo;
					eo2.calculate_functions = false;
					return differentiate(x_var, eo2);
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			MathStructure mstruct, vstruct;
			size_t idiv = 0;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isPower() && CHILD(i)[1].isNumber() && CHILD(i)[1].number().isNegative()) {
					if(idiv == 0) {
						if(CHILD(i)[1].isMinusOne()) {
							vstruct = CHILD(i)[0];
						} else {
							vstruct = CHILD(i);
							vstruct[1].number().negate();
						}
					} else {
						if(CHILD(i)[1].isMinusOne()) {
							vstruct.multiply(CHILD(i)[0], true);
						} else {
							vstruct.multiply(CHILD(i), true);
							vstruct[vstruct.size() - 1][1].number().negate();
						}
					}
					idiv++;
				}
			}
			if(idiv == SIZE) {
				set(-1, 1);
				MathStructure mdiv(vstruct);
				mdiv ^= MathStructure(2, 1);
				mdiv.inverse();
				multiply(mdiv);
				MathStructure diff_u(vstruct);
				diff_u.differentiate(x_var, eo);
				multiply(diff_u, true);
				break;
			} else if(idiv > 0) {
				idiv = 0;
				for(size_t i = 0; i < SIZE; i++) {
					if(!CHILD(i).isPower() || !CHILD(i)[1].isNumber() || !CHILD(i)[1].number().isNegative()) {
						if(idiv == 0) {
							mstruct = CHILD(i);
						} else {
							mstruct.multiply(CHILD(i), true);
						}
						idiv++;
					}
				}
				MathStructure mdiv(vstruct);
				mdiv ^= MathStructure(2, 1);
				MathStructure diff_v(vstruct);
				diff_v.differentiate(x_var, eo);
				MathStructure diff_u(mstruct);
				diff_u.differentiate(x_var, eo);
				vstruct *= diff_u;
				mstruct *= diff_v;
				set_nocopy(vstruct, true);
				subtract(mstruct);
				divide(mdiv);
				break;
			}
			if(SIZE > 2) {
				mstruct.set(*this);
				mstruct.delChild(mstruct.size());
				MathStructure mstruct2(LAST);
				MathStructure mstruct3(mstruct);
				mstruct.differentiate(x_var, eo);
				mstruct2.differentiate(x_var, eo);			
				mstruct *= LAST;
				mstruct2 *= mstruct3;
				set(mstruct, true);
				add(mstruct2);
			} else {
				mstruct = CHILD(0);
				MathStructure mstruct2(CHILD(1));
				mstruct.differentiate(x_var, eo);
				mstruct2.differentiate(x_var, eo);	
				mstruct *= CHILD(1);
				mstruct2 *= CHILD(0);
				set(mstruct, true);
				add(mstruct2);
			}
			break;
		}
		case STRUCT_SYMBOLIC: {
			if(representsNumber()) {
				clear(true);
			} else {
				MathStructure mstruct(CALCULATOR->f_diff, this, &x_var, &m_one, NULL);
				set(mstruct);
				return false;
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(eo.calculate_variables && o_variable->isKnown()) {
				if(eo.approximation != APPROXIMATION_EXACT || !o_variable->isApproximate()) {
					set(((KnownVariable*) o_variable)->get(), true);
					return differentiate(x_var, eo);
				} else if(containsRepresentativeOf(x_var, true, true) != 0) {
					MathStructure mstruct(CALCULATOR->f_diff, this, &x_var, &m_one, NULL);
					set(mstruct);
					return false;
				}
			}
			if(representsNumber(true)) {
				clear(true);
				break;
			}
		}		
		default: {
			MathStructure mstruct(CALCULATOR->f_diff, this, &x_var, &m_one, NULL);
			set(mstruct);
			return false;
		}	
	}
	return true;
}

bool MathStructure::integrate(const MathStructure &x_var, const EvaluationOptions &eo) {
	if(equals(x_var)) {
		raise(2);
		multiply(MathStructure(1, 2));
		return true;
	}
	if(containsRepresentativeOf(x_var, true, true) == 0) {
		multiply(x_var);
		return true;
	}
	switch(m_type) {
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).integrate(x_var, eo)) {
					CHILD_UPDATED(i);
				}
			}
			break;
		}
		case STRUCT_BITWISE_AND: {}
		case STRUCT_BITWISE_OR: {}
		case STRUCT_BITWISE_XOR: {}
		case STRUCT_LOGICAL_AND: {}
		case STRUCT_LOGICAL_OR: {}
		case STRUCT_LOGICAL_XOR: {}
		case STRUCT_COMPARISON: {}
		case STRUCT_UNIT: {}
		case STRUCT_NUMBER: {
			multiply(x_var);
			break;
		}
		case STRUCT_POWER: {
			if(CHILD(0).equals(x_var)) {
				if(CHILD(1).isNumber() && CHILD(1).number().isMinusOne()) {
					MathStructure mstruct(CALCULATOR->f_abs, &x_var, NULL);
					set(CALCULATOR->f_ln, &mstruct, NULL);
					break;
				} else if(CHILD(1).isNumber() || (CHILD(1).containsRepresentativeOf(x_var, true, true) == 0 && CHILD(1).representsNonNegative(false))) {
					CHILD(1) += m_one;
					MathStructure mstruct(CHILD(1));
					divide(mstruct);
					break;
				}
			} else if(CHILD(0).type() == STRUCT_POWER) {
			       this->simplify(eo);
			       return integrate(x_var, eo);
			} else if(CHILD(0).isVariable() && CHILD(0).variable() == CALCULATOR->v_e) {
				if(CHILD(1).equals(x_var)) {
					break;
				} else if(CHILD(1).isMultiplication()) {
					bool b = false;
					size_t i = 0;
					for(; i < CHILD(1).size(); i++) {
						if(CHILD(1)[i].equals(x_var)) {
							b = true;
							break;
						}
					}
					if(b) {
						MathStructure mstruct;
						if(CHILD(1).size() == 2) {
							if(i == 0) {
								mstruct = CHILD(1)[1];
							} else {
								mstruct = CHILD(1)[0];
							}
						} else {
							mstruct = CHILD(1);
							mstruct.delChild(i + 1);
						}
						if(mstruct.representsNonZero(false) && mstruct.containsRepresentativeOf(x_var, true, true) == 0) {
							divide(mstruct);
							break;
						}
					}
				}
			} else if(CHILD(1).equals(x_var) && CHILD(0).containsRepresentativeOf(x_var, true, true) == 0 && CHILD(0).representsPositive(false)) {
				MathStructure mstruct(CALCULATOR->f_ln, &CHILD(0), NULL);
				CHILD(1) *= mstruct;
				CHILD(0) = CALCULATOR->v_e;
				CHILDREN_UPDATED;
				return integrate(x_var, eo);
			}
			MathStructure mstruct(CALCULATOR->f_integrate, this, &x_var, NULL);
			set(mstruct);
			return false;
		}
		case STRUCT_FUNCTION: {
			if(o_function == CALCULATOR->f_ln && SIZE == 1 && CHILD(0) == x_var) {
				multiply(x_var);
				subtract(x_var);
			} else if(o_function == CALCULATOR->f_lambert_w && SIZE == 1 && CHILD(0) == x_var) {
				MathStructure *mthis = new MathStructure(*this);
				subtract(m_one);
				mthis->inverse();
				add_nocopy(mthis);
				multiply(x_var);
			} else if(o_function == CALCULATOR->f_sin && SIZE == 1 && CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0] == x_var && CHILD(0)[1] == CALCULATOR->getRadUnit()) {
				o_function = CALCULATOR->f_cos;
				multiply(m_minus_one);
			} else if(o_function == CALCULATOR->f_cos && SIZE == 1 && CHILD(0).isMultiplication() && CHILD(0).size() == 2 && CHILD(0)[0] == x_var && CHILD(0)[1] == CALCULATOR->getRadUnit()) {
				o_function = CALCULATOR->f_sin;
			} else if(o_function == CALCULATOR->f_diff && SIZE == 3 && CHILD(1) == x_var) {
				if(CHILD(2).isOne()) {
					setToChild(1, true);
				} else {
					CHILD(2) += m_minus_one;
				}
			} else if(o_function == CALCULATOR->f_diff && SIZE == 2 && CHILD(1) == x_var) {
				setToChild(1, true);
			} else {
				if(!eo.calculate_functions || !calculateFunctions(eo)) {
					MathStructure mstruct(CALCULATOR->f_integrate, this, &x_var, NULL);
					set(mstruct);
					return false;
				} else {
					EvaluationOptions eo2 = eo;
					eo2.calculate_functions = false;
					return integrate(x_var, eo2);
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			MathStructure mstruct;
			bool b = false;
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).containsRepresentativeOf(x_var, true, true) == 0) {
					if(b) {
						mstruct *= CHILD(i);
					} else {
						mstruct = CHILD(i);
						b = true;
					}
					ERASE(i);
				}
			}
			if(b) {
				if(SIZE == 1) {
					setToChild(1, true);
				} else if(SIZE == 0) {
					set(mstruct, true);
					break;	
				}
				integrate(x_var, eo);
				multiply(mstruct);
			} else {
				MathStructure mstruct(CALCULATOR->f_integrate, this, &x_var, NULL);
				set(mstruct);
				return false;
			}
			break;
		}
		case STRUCT_SYMBOLIC: {
			if(representsNumber()) {
				multiply(x_var);
			} else {
				MathStructure mstruct(CALCULATOR->f_integrate, this, &x_var, NULL);
				set(mstruct);
				return false;
			}
			break;
		}
		case STRUCT_VARIABLE: {
			if(eo.calculate_variables && o_variable->isKnown()) {
				if(eo.approximation != APPROXIMATION_EXACT || !o_variable->isApproximate()) {
					set(((KnownVariable*) o_variable)->get(), true);
					return integrate(x_var, eo);
				} else if(containsRepresentativeOf(x_var, true, true) != 0) {
					MathStructure mstruct3(1);
					MathStructure mstruct(CALCULATOR->f_integrate, this, &x_var, &mstruct3, NULL);
					set(mstruct);
					return false;
				}
			}
			if(representsNumber()) {
				multiply(x_var);
				break;
			}
		}		
		default: {
			MathStructure mstruct(CALCULATOR->f_integrate, this, &x_var, NULL);
			set(mstruct);
			return false;
		}	
	}
	return true;
}


const MathStructure &MathStructure::find_x_var() const {
	if(isSymbolic()) {
		return *this;
	} else if(isVariable()) {
		if(o_variable->isKnown()) return m_undefined;
		return *this;
	}
	const MathStructure *mstruct;
	const MathStructure *x_mstruct = &m_undefined;
	for(size_t i = 0; i < SIZE; i++) {
		mstruct = &CHILD(i).find_x_var();
		if(mstruct->isVariable()) {
			if(mstruct->variable() == CALCULATOR->v_x) {
				return *mstruct;
			} else if(!x_mstruct->isVariable()) {
				x_mstruct = mstruct;
			} else if(mstruct->variable() == CALCULATOR->v_y) {
				x_mstruct = mstruct;
			} else if(mstruct->variable() == CALCULATOR->v_z && x_mstruct->variable() != CALCULATOR->v_y) {
				x_mstruct = mstruct;
			}
		} else if(mstruct->isSymbolic()) {
			if(!x_mstruct->isVariable() && !x_mstruct->isSymbolic()) {
				x_mstruct = mstruct;
			}
		}
	}
	return *x_mstruct;
}

bool test_comparisons(const MathStructure &msave, MathStructure &mthis, const MathStructure &x_var, const EvaluationOptions &eo, bool sub) {
	if(mthis.isComparison() && mthis[0] == x_var) {
		MathStructure mtest;
		EvaluationOptions eo2 = eo;
		eo2.calculate_functions = false;
		eo2.isolate_x = false;
		eo2.test_comparisons = true;
		if(eo2.approximation == APPROXIMATION_EXACT) eo2.approximation = APPROXIMATION_TRY_EXACT;
		mtest = mthis;
		mtest.eval(eo2);
		if(mtest.isComparison()) {
			mtest = msave;
			mtest.replace(x_var, mthis[1]);
			CALCULATOR->beginTemporaryStopMessages();
			mtest.eval(eo2);
			if(CALCULATOR->endTemporaryStopMessages() == 0 && mtest.isNumber() && ((mthis.comparisonType() != COMPARISON_NOT_EQUALS && mtest.number().getBoolean() > 0) || (mthis.comparisonType() == COMPARISON_NOT_EQUALS && mtest.number().getBoolean() < 1))) {
				return true;	
			}
		}
		if(!sub) mthis = msave;
		return false;
	}
	if(mthis.isLogicalOr() || mthis.isLogicalAnd()) {
		MathStructure mtest;
		EvaluationOptions eo2 = eo;
		eo2.calculate_functions = false;
		eo2.isolate_x = false;
		eo2.test_comparisons = true;
		if(eo2.approximation == APPROXIMATION_EXACT) eo2.approximation = APPROXIMATION_TRY_EXACT;
		for(size_t i = 0; i < mthis.size();) {
			if(!test_comparisons(msave, mthis[i], x_var, eo, true)) {
				if(mthis.isLogicalAnd()) {
					if(!sub) mthis = msave;
					return false;
				}
				mthis.delChild(i + 1);
			} else {
				i++;
			}
		}
		if(mthis.size() > 0) {
			if(mthis.size() == 1) mthis.setToChild(1);
			return true;
		}
		if(!sub) mthis = msave;
		return false;
	}
	return sub;
}

bool isx_deabsify(MathStructure &mstruct);
bool isx_deabsify(MathStructure &mstruct) {
	switch(mstruct.type()) {
		case STRUCT_FUNCTION: {
			if(mstruct.function() == CALCULATOR->f_abs && mstruct.size() == 1 && mstruct[0].representsReal(true)) {
				mstruct.setToChild(1, true);
				return true;
			}
			break;
		}
		case STRUCT_POWER: {
			if(mstruct[1].isMinusOne()) {
				return isx_deabsify(mstruct[0]);
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			bool b = false;
			for(size_t i = 0; i < mstruct.size(); i++) {
				if(isx_deabsify(mstruct[i])) b = true;
			}
			return b;
		}
		default: {}
	}
	return false;
}

bool MathStructure::isolate_x_sub(const EvaluationOptions &eo, EvaluationOptions &eo2, const MathStructure &x_var, MathStructure *morig) {
	if(!isComparison()) {
		printf("isolate_x_sub: not comparison\n");
		return false;
	}
	if(CHILD(0) == x_var) return false;	
	switch(CHILD(0).type()) {
		case STRUCT_ADDITION: {
			bool b = false;
			for(size_t i = 0; i < CHILD(0).size(); i++) {
				if(!CHILD(0)[i].contains(x_var)) {
					CHILD(0)[i].calculateNegate(eo2);
					CHILD(0)[i].ref();
					CHILD(1).add_nocopy(&CHILD(0)[i], true);
					CHILD(1).calculateAddLast(eo2);
					CHILD(0).delChild(i + 1);
					b = true;
				}
			}
			if(b) {
				CHILD_UPDATED(0);
				CHILD_UPDATED(1);
				if(CHILD(0).size() == 1) {
					CHILD(0).setToChild(1, true);
				} else if(CHILD(0).size() == 0) {
					CHILD(0).clear(true);
				}
				isolate_x_sub(eo, eo2, x_var, morig);
				return true;
			}
			if(CHILD(0).size() == 2 && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
				bool sqpow = false;
				bool nopow = false;
				MathStructure mstruct_a, mstruct_b;
				for(size_t i = 0; i < CHILD(0).size(); i++) {
					if(!sqpow && CHILD(0)[i].isPower() && CHILD(0)[i][0] == x_var && CHILD(0)[i][1].isNumber() && CHILD(0)[i][1].number() == 2) {
						sqpow = true;
						mstruct_a.set(m_one);
					} else if(!nopow && CHILD(0)[i] == x_var) {
						nopow = true;
						mstruct_b.set(m_one);
					} else if(CHILD(0)[i].isMultiplication()) {
						for(size_t i2 = 0; i2 < CHILD(0)[i].size(); i2++) {
							if(CHILD(0)[i][i2] == x_var) {
								if(nopow) break;
								nopow = true;
								mstruct_b = CHILD(0)[i];
								mstruct_b.delChild(i2 + 1);
								if(mstruct_b.size() == 1) {
									mstruct_b.setToChild(1, true);
								}
							} else if(CHILD(0)[i][i2].isPower() && CHILD(0)[i][i2][0] == x_var && CHILD(0)[i][i2][1].isNumber() && CHILD(0)[i][i2][1].number() == 2) {
								if(sqpow) break;
								sqpow = true;
								mstruct_a = CHILD(0)[i];
								mstruct_a.delChild(i2 + 1);
								if(mstruct_a.size() == 1) {
									mstruct_a.setToChild(1, true);
								}
							} else if(CHILD(0)[i][i2].contains(x_var)) {
								sqpow = false;
								nopow = false;
								break;
							}
						}
					}
				}	
				if(sqpow && nopow && mstruct_a.representsNonZero(true)) {
					MathStructure b2(mstruct_b);
					b2.calculateRaise(MathStructure(2, 1), eo2);
					MathStructure ac(4, 1);
					ac.calculateMultiply(mstruct_a, eo2);
					ac.calculateMultiply(CHILD(1), eo2);
					b2.calculateAdd(ac, eo2);
					if(eo.allow_complex || b2.representsNonNegative()) {
						b2.calculateRaise(MathStructure(1, 2), eo2);	
						mstruct_b.calculateNegate(eo2);
						MathStructure mstruct_1(mstruct_b);
						mstruct_1.calculateAdd(b2, eo2);
						MathStructure mstruct_2(mstruct_b);
						mstruct_2.calculateSubtract(b2, eo2);
						mstruct_a.calculateMultiply(MathStructure(2, 1), eo2);
						mstruct_a.calculateInverse(eo2);
						mstruct_1.calculateMultiply(mstruct_a, eo2);
						mstruct_2.calculateMultiply(mstruct_a, eo2);
						CHILD(0) = x_var;
						if(mstruct_1 != mstruct_2) {
							CHILD(1) = mstruct_1;
							MathStructure *mchild2 = new MathStructure(CHILD(0));
							mchild2->transform(STRUCT_COMPARISON, mstruct_2);
							mchild2->setComparisonType(ct_comp);
							if(ct_comp == COMPARISON_NOT_EQUALS) {
								transform_nocopy(STRUCT_LOGICAL_AND, mchild2);
							} else {
								transform_nocopy(STRUCT_LOGICAL_OR, mchild2);
							}
							calculatesub(eo2, eo, false);
						} else {
							CHILD(1) = mstruct_1;
						}
						CHILDREN_UPDATED;
						return true;
					}
				} 
			}
			/*if(!CHILD(1).representsNonZero(true)) {
				int index1 = -1;
				int index2 = -1;
				for(size_t i2 = 0; i2 < CHILD(0).size(); i2++) {
					if(index1 < 0 && CHILD(0)[i2].isMultiplication()) {
						for(size_t i = 0; i < CHILD(0)[i2].size(); i++) {
							if(CHILD(0)[i2][i].isPower() && CHILD(0)[i2][i][1].isNumber() && CHILD(0)[i2][i][1].number().isMinusOne() && CHILD(0)[i2][i][0].contains(x_var) && CHILD(0)[i2][i][0].representsNonZero(true)) {
								index1 = i2;
								index2 = i;
								break;
							}
						}
					} else if(!CHILD(0)[i2].representsNonZero(true)) {
						index1 = -1;
						break;
					}
				}
				if(index1 >= 0) {
					MathStructure msave(CHILD(0)[index1]);
					CHILD(0).delChild(index1 + 1);
					if(CHILD(0).size() == 0) {
						CHILD(0).clear(true);
					} else if(CHILD(0).size() == 1) {
						CHILD(0).setToChild(1, true);
					}
					CHILD(0).calculateSubtract(CHILD(1), eo2);
					CHILD(1).clear();
					CHILD(0).calculateMultiply(msave[index2][0], eo2);
					msave.delChild(index2 + 1);
					if(msave.size() == 1) {
						msave.setToChild(1, true);
					}
					CHILD(0).calculateAdd(msave, eo2);
					CHILDREN_UPDATED;
					isolate_x_sub(eo, eo2, x_var, morig);
					return true;
				}
			}*/
			if(!eo2.expand) break;
			MathStructure mtest(*this);
			if(!CHILD(1).isZero()) {
				mtest[0].calculateSubtract(CHILD(1), eo2);
				mtest[1].clear();
				mtest.childrenUpdated();
			}
			if(!morig || !equals(morig)) {
				if(mtest[0].factorize(eo2) && !(mtest[0].isMultiplication() && mtest[0].size() == 2 && (mtest[0][0].isNumber() || mtest[0][0] == CHILD(1) || mtest[0][1] == CHILD(1)))) {
					mtest.childUpdated(1);
					if(mtest.isolate_x_sub(eo, eo2, x_var, this)) {
						set_nocopy(mtest);
						return true;
					}
				}
				if(!CHILD(1).isZero()) {
					mtest.set(*this);
					if(mtest[0].factorize(eo2) && !(mtest[0].isMultiplication() && mtest[0].size() == 2 && (mtest[0][0].isNumber() || mtest[0][0] == CHILD(1) || mtest[0][1] == CHILD(1)))) {
						mtest.childUpdated(1);
						if(mtest.isolate_x_sub(eo, eo2, x_var, this)) {
							set_nocopy(mtest);
							return true;
						}
					}
				}
			}
			break;
		}
		case STRUCT_MULTIPLICATION: {
			if(!representsNonMatrix()) return false;
			bool b = false;						
			int zero1;
			if(CHILD(1).isZero()) zero1 = 1;
			else if(CHILD(1).representsNonZero(true)) zero1 = 0;
			else zero1 = 2;
			MathStructure *mcheckmulti = NULL, *mtryzero = NULL, *mchecknegative = NULL, *mchecknonzeropow = NULL;
			MathStructure mchild2(CHILD(1));
			ComparisonType ct_orig = ct_comp;
			for(size_t i = 0; i < CHILD(0).size(); i++) {			
				if(!CHILD(0)[i].contains(x_var)) {
					bool nonzero = false;
					if(CHILD(0)[i].isPower() && !CHILD(0)[i][1].representsNonNegative(true) && !CHILD(0)[i][0].representsNonZero(true)) {
						MathStructure *mcheck = new MathStructure(CHILD(0)[i][0]);
						if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add(m_zero, OPERATION_EQUALS);
						else mcheck->add(m_zero, OPERATION_NOT_EQUALS);
						mcheck->isolate_x(eo2, eo);
						if(CHILD(0)[i][1].representsNegative(true)) {
							nonzero = true;
						} else {
							MathStructure *mcheckor = new MathStructure(CHILD(0)[i][1]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheckor->add(m_zero, OPERATION_EQUALS_LESS);
							else mcheckor->add(m_zero, OPERATION_EQUALS_GREATER);
							mcheckor->isolate_x(eo2, eo);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add_nocopy(mcheckor, OPERATION_LOGICAL_AND);
							else mcheck->add_nocopy(mcheckor, OPERATION_LOGICAL_OR);
							mcheck->calculatesub(eo2, eo, false);
						}
						if(mchecknonzeropow) {
							if(ct_comp == COMPARISON_NOT_EQUALS) mchecknonzeropow->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
							else mchecknonzeropow->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
						} else {
							mchecknonzeropow = mcheck;
						}
					}
					if(!nonzero && !CHILD(0)[i].representsNonZero(true)) {
						if(zero1 != 1 && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
							MathStructure *mcheck = new MathStructure(CHILD(0)[i]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add(m_zero, OPERATION_EQUALS);
							else mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							mcheck->isolate_x(eo2, eo);
							if(mcheckmulti) {
								if(ct_comp == COMPARISON_NOT_EQUALS) mcheckmulti->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
								else mcheckmulti->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
							} else {
								mcheckmulti = mcheck;	
							}
						}
						if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
							if(!mtryzero) {
								mtryzero = new MathStructure(CHILD(0)[i]);
								mtryzero->add(m_zero, OPERATION_EQUALS);
								mtryzero->isolate_x(eo2, eo);								
								MathStructure *mcheck = new MathStructure(mchild2);
								switch(ct_orig) {
									case COMPARISON_LESS: {mcheck->add(m_zero, OPERATION_GREATER); break;}
									case COMPARISON_GREATER: {mcheck->add(m_zero, OPERATION_LESS); break;}
									case COMPARISON_EQUALS_LESS: {mcheck->add(m_zero, OPERATION_EQUALS_GREATER); break;}
									case COMPARISON_EQUALS_GREATER: {mcheck->add(m_zero, OPERATION_EQUALS_LESS); break;}
									default: {}
								}
								mcheck->isolate_x(eo2, eo);
								mtryzero->add_nocopy(mcheck, OPERATION_LOGICAL_AND);		
							} else {
								MathStructure *mcheck = new MathStructure(CHILD(0)[i]);
								mcheck->add(m_zero, OPERATION_EQUALS);
								mcheck->isolate_x(eo2, eo);
								(*mtryzero)[0].add_nocopy(mcheck, OPERATION_LOGICAL_OR);
								mtryzero[0].calculateLogicalOrLast(eo2);
							}
						} else if(zero1 > 0) {
							MathStructure *mcheck = new MathStructure(CHILD(0)[i]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							else mcheck->add(m_zero, OPERATION_EQUALS);
							mcheck->isolate_x(eo2, eo);
							if(zero1 == 2) {
								MathStructure *mcheck2 = new MathStructure(mchild2);
								if(ct_comp == COMPARISON_NOT_EQUALS) mcheck2->add(m_zero, OPERATION_NOT_EQUALS);
								else mcheck2->add(m_zero, OPERATION_EQUALS);
								mcheck2->isolate_x(eo2, eo);
								if(ct_comp == COMPARISON_NOT_EQUALS) mcheck2->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
								else mcheck2->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
								mcheck2->calculatesub(eo2, eo, false);
								mcheck = mcheck2;
							}							
							if(mtryzero) {
								if(ct_comp == COMPARISON_NOT_EQUALS) mtryzero->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
								else mtryzero->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
							} else {
								mtryzero = mcheck;
							}
						}
					}
					if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
						if(CHILD(0)[i].representsNegative()) {
							switch(ct_comp) {
								case COMPARISON_LESS: {ct_comp = COMPARISON_GREATER; break;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_LESS; break;}
								case COMPARISON_EQUALS_LESS: {ct_comp = COMPARISON_EQUALS_GREATER; break;}
								case COMPARISON_EQUALS_GREATER: {ct_comp = COMPARISON_EQUALS_LESS; break;}
								default: {}
							}
						} else if(!CHILD(0)[i].representsNonNegative()) {
							if(mchecknegative) {
								mchecknegative->multiply(CHILD(0)[i]);
							} else {
								mchecknegative = new MathStructure(CHILD(0)[i]);
							}
						}
					}
					if(zero1 != 1) {
						CHILD(1).calculateDivide(CHILD(0)[i], eo2);
						CHILD_UPDATED(1);
					}
					CHILD(0).delChild(i + 1);
					b = true;
				}
			}
			if(!b && ct_comp == COMPARISON_EQUALS && eo2.approximation != APPROXIMATION_EXACT && CHILD(1).isNumber() && CHILD(0).size() <= 3 && CHILD(0).size() >= 2) {
				//Check for x*e^x and solve with Lambert W function
				size_t e_index = CHILD(0).size();
				bool e_power = false;
				bool had_x_var = false;
				bool pow_num = false;
				bool multi_num = false;
				const MathStructure *x_struct = NULL;
				for(size_t i = 0; i < CHILD(0).size(); i++) {
					if(x_struct && CHILD(0)[i] == *x_struct) {
						if(had_x_var) {
							had_x_var = false; 
							break;
						}
						had_x_var = true;
					} else if(CHILD(0)[i].isPower()) {
						if(!x_struct && CHILD(0)[i][0] == x_var) {
							x_struct = &CHILD(0)[i];
							if(had_x_var) {
								had_x_var = false;
								break;
							}
							had_x_var = true;
						} else {
							if(e_index < CHILD(0).size()) {
								e_index = CHILD(0).size();
								break;
							}
							if((CHILD(0)[i][0].isVariable() && CHILD(0)[i][0].variable() == CALCULATOR->v_e) || (CHILD(0)[i][0].isNumber() && CHILD(0)[i][0].number().isPositive())) {
								if(x_struct && CHILD(0)[i][1] == *x_struct) {
									e_index = i;
									e_power = CHILD(0)[i][0].isVariable();
								} else if(!x_struct && (CHILD(0)[i][1] == x_var || (CHILD(0)[i][1].isPower() && CHILD(0)[i][1][0] == x_var))) {
									x_struct = &CHILD(0)[i][1];
									e_index = i;
									e_power = CHILD(0)[i][0].isVariable();
								} else if(CHILD(0)[i][1].isMultiplication() && CHILD(0)[i][1].size() == 2 && CHILD(0)[i][1][0].isNumber() && !CHILD(0)[i][1][0].isZero()) {
									if(x_struct && CHILD(0)[i][1][1] == *x_struct) {
										e_index = i;
										e_power = CHILD(0)[i][0].isVariable();
										pow_num = true;
									} else if(!x_struct && (CHILD(0)[i][1][1] == x_var || (CHILD(0)[i][1][1].isPower() && CHILD(0)[i][1][1][0] == x_var))) {
										x_struct = &CHILD(0)[i][1][1];
										e_index = i;
										e_power = CHILD(0)[i][0].isVariable();
										pow_num = true;
									} else {
										break;
									}
								} else {
									break;
								}							
							} else {
								break;
							}
						}
					} else if(!x_struct && CHILD(0)[i] == x_var) {
						if(had_x_var) {
							had_x_var = false;
							break;
						}
						x_struct = &x_var;
						had_x_var = true;
					} else if(CHILD(0)[i].isNumber() && !CHILD(0)[i].isZero() && i == 0) {
						multi_num = true;
					} else {
						had_x_var = false;						
						break;
					}
				}				
				if(had_x_var && e_index < CHILD(0).size()) {
					Number num(CHILD(1).number());
					Number nr_pow(1, 1);
					if(pow_num) nr_pow = CHILD(0)[e_index][1][0].number();
					if(!e_power) {
						Number n_ln(CHILD(0)[e_index][0].number());
						n_ln.ln();
						nr_pow *= n_ln;						
						pow_num = true;						
					}
					if(multi_num) num /= CHILD(0)[0].number();
					if(pow_num) num *= nr_pow;					
					if(num.lambertW()) {
						CHILD(1) = num;
						if(e_power) {
							CHILD(0)[e_index].setToChild(2, true);
							CHILD(0).setToChild(e_index + 1, true);
						} else {
							MathStructure x_struct_new(*x_struct);
							CHILD(0).set(x_struct_new);
							CHILD(0).calculateMultiply(nr_pow, eo2);
						}						
						CHILDREN_UPDATED;
						isolate_x_sub(eo, eo2, x_var, morig);
						return true;
					}
				}
			}
			if(b) {
				if(CHILD(0).size() == 1) {
					CHILD(0).setToChild(1);
				}
				if((ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) && CHILD(1).contains(x_var)) {
					CHILD(0).calculateSubtract(CHILD(1), eo2);
					CHILD(1).clear();
				}
				if(mchecknegative) {
					MathStructure *mneg = new MathStructure(*this);
					switch(ct_comp) {
						case COMPARISON_LESS: {mneg->setComparisonType(COMPARISON_GREATER); break;}
						case COMPARISON_GREATER: {mneg->setComparisonType(COMPARISON_LESS); break;}
						case COMPARISON_EQUALS_LESS: {mneg->setComparisonType(COMPARISON_EQUALS_GREATER); break;}
						case COMPARISON_EQUALS_GREATER: {mneg->setComparisonType(COMPARISON_EQUALS_LESS); break;}
						default: {}
					}
					isolate_x_sub(eo, eo2, x_var, morig);
					mneg->isolate_x_sub(eo, eo2, x_var);
					mchecknegative->add(m_zero, OPERATION_GREATER);
					add(*mchecknegative, OPERATION_LOGICAL_AND, true);
					SWAP_CHILDREN(0, SIZE - 1)
					calculatesub(eo2, eo, false);
					LAST.isolate_x(eo2, eo);
					mchecknegative->setComparisonType(COMPARISON_LESS);
					mchecknegative->isolate_x(eo2, eo);
					mneg->add_nocopy(mchecknegative, OPERATION_LOGICAL_AND, true);
					mneg->calculatesub(eo2, eo, false);
					add_nocopy(mneg, OPERATION_LOGICAL_OR, true);
					calculatesub(eo2, eo, false);
				} else {
					isolate_x_sub(eo, eo2, x_var, morig);
				}
				if(mcheckmulti) {
					mcheckmulti->calculatesub(eo2, eo, false);
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						add_nocopy(mcheckmulti, OPERATION_LOGICAL_OR, true);
					} else {
						add_nocopy(mcheckmulti, OPERATION_LOGICAL_AND, true);
						SWAP_CHILDREN(0, SIZE - 1)
					}
					calculatesub(eo2, eo, false);
				}
				if(mchecknonzeropow) {
					mchecknonzeropow->calculatesub(eo2, eo, false);
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						add_nocopy(mchecknonzeropow, OPERATION_LOGICAL_OR, true);
					} else {
						add_nocopy(mchecknonzeropow, OPERATION_LOGICAL_AND, true);
						SWAP_CHILDREN(0, SIZE - 1)
					}
					calculatesub(eo2, eo, false);
				}
				if(mtryzero) {					
					mtryzero->calculatesub(eo2, eo, false);
					if(ct_comp == COMPARISON_NOT_EQUALS) {
						add_nocopy(mtryzero, OPERATION_LOGICAL_AND, true);
						SWAP_CHILDREN(0, SIZE - 1)
					} else {
						add_nocopy(mtryzero, OPERATION_LOGICAL_OR, true);
					}					
					calculatesub(eo2, eo, false);
				}												
				return true;
			} else if(CHILD(1).isZero()) {
				if(ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS) {
					vector<int> checktype;
					bool bdoit = false;					
					for(size_t i = 0; i < CHILD(0).size(); i++) {
						if(CHILD(0)[i].isPower() && !CHILD(0)[i][1].representsNonNegative()) {
							if(CHILD(0)[i][1].representsNegative()) {
								checktype.push_back(1);
							} else {
								checktype.push_back(2);
								bdoit = true;
							}
						} else {
							checktype.push_back(0);
							bdoit = true;
						}
					}
					if(!bdoit) break;
					MathStructure *mcheckpowers = NULL;
					ComparisonType ct = ct_comp;
					setToChild(1);
					if(ct == COMPARISON_NOT_EQUALS) {
						setType(STRUCT_LOGICAL_AND);
					} else {
						setType(STRUCT_LOGICAL_OR);
					}
					for(size_t i = 0; i < SIZE;) {
						if(checktype[i] > 0) {
							MathStructure *mcheck = new MathStructure(CHILD(i)[0]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mcheck->add(m_zero, OPERATION_EQUALS);
							else mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							mcheck->isolate_x_sub(eo, eo2, x_var);
							if(checktype[i] == 2) {
								MathStructure *mexpcheck = new MathStructure(CHILD(i)[1]);
								if(ct_comp == COMPARISON_NOT_EQUALS) mexpcheck->add(m_zero, OPERATION_LESS);
								else mexpcheck->add(m_zero, OPERATION_EQUALS_GREATER);
								mexpcheck->isolate_x_sub(eo, eo2, x_var);
								if(ct_comp == COMPARISON_NOT_EQUALS) mexpcheck->add_nocopy(mcheck, OPERATION_LOGICAL_AND, true);
								else mexpcheck->add_nocopy(mcheck, OPERATION_LOGICAL_OR, true);
								mexpcheck->calculatesub(eo2, eo, false);
								mcheck = mexpcheck;
							}
							if(mcheckpowers) {
								if(ct_comp == COMPARISON_NOT_EQUALS) mcheckpowers->add_nocopy(mcheck, OPERATION_LOGICAL_OR, true);
								else mcheckpowers->add_nocopy(mcheck, OPERATION_LOGICAL_AND, true);
							} else {
								mcheckpowers = mcheck;
							}
						}
						if(checktype[i] == 1) {
							ERASE(i)
							checktype.erase(checktype.begin() + i);
						} else {
							CHILD(i).transform(STRUCT_COMPARISON, m_zero);
							CHILD(i).setComparisonType(ct);
							CHILD(i).isolate_x_sub(eo, eo2, x_var);
							i++;
						}
					}					
					if(SIZE == 1) setToChild(1);
					else if(SIZE == 0) clear(true);
					else calculatesub(eo2, eo, false);
					if(mcheckpowers) {
						mcheckpowers->calculatesub(eo2, eo, false);
						if(ct_comp == COMPARISON_NOT_EQUALS) add_nocopy(mcheckpowers, OPERATION_LOGICAL_OR, true);
						else add_nocopy(mcheckpowers, OPERATION_LOGICAL_AND, true);
						SWAP_CHILDREN(0, SIZE - 1)
						calculatesub(eo2, eo, false);
					}
					return true;
				} else if(CHILD(0).size() >= 2) {
					MathStructure mless1(CHILD(0)[0]);
					MathStructure mless2;
					if(CHILD(0).size() == 2) {
						mless2 = CHILD(0)[1];
					} else {
						mless2 = CHILD(0);
						mless2.delChild(1);
					}
					
					int checktype1 = 0, checktype2 = 0;
					MathStructure *mcheck1 = NULL, *mcheck2 = NULL;
					if(mless1.isPower() && !mless1[1].representsNonNegative()) {
						if(mless1[1].representsNegative()) {
							checktype1 = 1;
						} else if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER) {
							checktype1 = 2;
							mcheck1 = new MathStructure(mless1[1]);
							mcheck1->add(m_zero, OPERATION_EQUALS_GREATER);
							mcheck1->isolate_x_sub(eo, eo2, x_var);
							MathStructure *mcheck = new MathStructure(mless1[0]);
							mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							mcheck1->isolate_x_sub(eo, eo2, x_var);
							mcheck1->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
							mcheck1->calculatesub(eo2, eo, false);
						}
					}
					if(mless2.isPower() && !mless2[1].representsNonNegative()) {
						if(mless2[1].representsNegative()) {
							checktype2 = 1;
						} else if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER) {
							checktype2 = 2;
							mcheck2 = new MathStructure(mless2[1]);
							mcheck2->add(m_zero, OPERATION_EQUALS_GREATER);
							mcheck2->isolate_x_sub(eo, eo2, x_var);
							MathStructure *mcheck = new MathStructure(mless2[0]);
							mcheck->add(m_zero, OPERATION_NOT_EQUALS);
							mcheck2->isolate_x_sub(eo, eo2, x_var);
							mcheck2->add_nocopy(mcheck, OPERATION_LOGICAL_OR);
							mcheck2->calculatesub(eo2, eo, false);
						}
					}
					
					mless1.transform(STRUCT_COMPARISON, m_zero);
					if(checktype1 != 1 && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
						mless1.setComparisonType(COMPARISON_EQUALS_LESS);
					} else {
						mless1.setComparisonType(COMPARISON_LESS);
					}
					mless1.isolate_x_sub(eo, eo2, x_var);	
					mless2.transform(STRUCT_COMPARISON, m_zero);
					mless2.setComparisonType(COMPARISON_LESS);
					if(checktype2 != 1 && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
						mless2.setComparisonType(COMPARISON_EQUALS_LESS);
					} else {
						mless2.setComparisonType(COMPARISON_LESS);
					}
					mless2.isolate_x_sub(eo, eo2, x_var);
					MathStructure mgreater1(CHILD(0)[0]);
					mgreater1.transform(STRUCT_COMPARISON, m_zero);
					if(checktype1 != 1 && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
						mgreater1.setComparisonType(COMPARISON_EQUALS_GREATER);
					} else {
						mgreater1.setComparisonType(COMPARISON_GREATER);
					}
					mgreater1.isolate_x_sub(eo, eo2, x_var);
					MathStructure mgreater2;
					if(CHILD(0).size() == 2) {
						mgreater2 = CHILD(0)[1];
					} else {
						mgreater2 = CHILD(0);
						mgreater2.delChild(1);
					}
					mgreater2.transform(STRUCT_COMPARISON, m_zero);
					if(checktype2 != 1 && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
						mgreater2.setComparisonType(COMPARISON_EQUALS_GREATER);
					} else {
						mgreater2.setComparisonType(COMPARISON_GREATER);
					}
					mgreater2.isolate_x_sub(eo, eo2, x_var);
					clear();
					
					if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
						set(mless1);
						transform(STRUCT_LOGICAL_AND, mgreater2);
						calculatesub(eo2, eo, false);
						transform(STRUCT_LOGICAL_OR, mless2);
						CHILD(1).transform(STRUCT_LOGICAL_AND, mgreater1);
						CHILD(1).calculatesub(eo2, eo, false);
						CHILD_UPDATED(1)
						calculatesub(eo2, eo, false);
					} else {
						set(mless1);
						transform(STRUCT_LOGICAL_AND, mless2);
						calculatesub(eo2, eo, false);
						transform(STRUCT_LOGICAL_OR, mgreater1);
						CHILD(1).transform(STRUCT_LOGICAL_AND, mgreater2);
						CHILD(1).calculatesub(eo2, eo, false);
						CHILD_UPDATED(1)
						calculatesub(eo2, eo, false);
					}					
					if(checktype1 == 2) {
						add_nocopy(mcheck1, OPERATION_LOGICAL_AND, true);
						calculatesub(eo2, eo, false);
					}
					if(checktype2 == 2) {
						add_nocopy(mcheck2, OPERATION_LOGICAL_AND, true);
						calculatesub(eo2, eo, false);
					}
					return true;
					
				}
			}
			if(!eo2.expand) break;
			if(!CHILD(1).isZero() && (!morig || !equals(*morig))) {
				MathStructure mtest(*this);
				mtest[0].calculateSubtract(CHILD(1), eo2);
				mtest[1].clear();
				mtest.childrenUpdated();
				if(mtest[0].factorize(eo2) && !(mtest[0].isMultiplication() && mtest[0].size() == 2 && (mtest[0][0].isNumber() || mtest[0][0] == CHILD(1) || mtest[0][1] == CHILD(1)))) {
					mtest.childUpdated(1);
					if(mtest.isolate_x_sub(eo, eo2, x_var, this)) {
						set_nocopy(mtest);
						return true;
					}
				}
			}
			break;
		} 
		case STRUCT_POWER: {
			if(CHILD(0)[0].contains(x_var)) {
				if(ct_comp == COMPARISON_EQUALS && eo2.approximation != APPROXIMATION_EXACT && CHILD(1).isNumber() && CHILD(1).number().isPositive() && CHILD(0)[1] == x_var && CHILD(0)[0] == x_var) {
					Number n_ln_z(CHILD(1).number());
					if(n_ln_z.ln()) {
						Number n_ln_z_w(n_ln_z);
						if(n_ln_z_w.lambertW()) {
							CHILD(1).number() = n_ln_z;
							CHILD(1).number() /= n_ln_z_w;
							CHILD(0) = x_var;
							CHILDREN_UPDATED
							return true;
						}
					}
				} else if(!CHILD(0)[1].contains(x_var)) {									
					MathStructure *mbasecheck = NULL, *mcheckeven = NULL, *malternative = NULL, *malternative2 = NULL, *mcheck1 = NULL;
					bool b_zero = CHILD(1).isZero();					
					bool b_posexp = CHILD(0)[1].representsNonNegative(true);
					bool b_negexp = !b_posexp && CHILD(0)[1].representsNegative();
					bool b_nonzero = !b_zero && CHILD(1).representsNonZero(true);

					if(b_zero && (b_negexp || CHILD(0)[1].isZero()) && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
						if(ct_comp == COMPARISON_EQUALS) {
							clear(true);
						} else if(ct_comp == COMPARISON_NOT_EQUALS) {
							set(1, 1, 0, true);
						}
						return true;
					}

					bool b_pos1 = true;
					bool b_neg1 = false;
					if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
						if(!b_negexp && !b_posexp) {
							return false;
						}
						b_pos1 = b_zero || CHILD(1).representsNonNegative(true);
						b_neg1 = !b_pos1 && CHILD(1).representsNegative(true);
					}
					bool pow_even = false;
					if(CHILD(0)[1].isNumber()) {
						if(!CHILD(0)[1].number().isRational()) {
							if(!b_pos1) {
								return false;
							}
						} else {
							pow_even = CHILD(0)[1].number().numeratorIsEven();
						}
					} else {
						pow_even = CHILD(0)[1].representsEven();					
						if(!pow_even && !CHILD(0)[1].representsOdd()) {
							pow_even = true;
							MathStructure mnum(CALCULATOR->f_numerator, &CHILD(0)[1], NULL);
							if(b_neg1) mcheckeven = new MathStructure(CALCULATOR->f_odd, &mnum, NULL);
							else mcheckeven = new MathStructure(CALCULATOR->f_even, &mnum, NULL);
							mcheckeven->calculateFunctions(eo);
							if(mcheckeven->isOne()) {
								delete mcheckeven;
								mcheckeven = NULL;
								if(b_neg1) pow_even = false;
							} else if(mcheckeven->isZero()) {
								delete mcheckeven;								
								mcheckeven = NULL;
								if(!b_neg1) pow_even = false;
							} else if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
								return false;
							} else if(!CHILD(0)[1].representsRational()) {
								delete mcheckeven;		
								mcheckeven = NULL;
								pow_even = false;
							}
						}
					}
					if(pow_even && b_neg1) {
						if(ct_comp == COMPARISON_LESS) {
							clear(true);
							return true;
						} else if(ct_comp == COMPARISON_EQUALS_LESS) {
							ct_comp = COMPARISON_EQUALS;
							b_pos1 = false;
							b_neg1 = false;
						} else if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {
							ct_comp = COMPARISON_EQUALS_GREATER;
							CHILD(1).clear(true);
							CHILD_UPDATED(1)
							b_pos1 = true;
							b_neg1 = false;
							b_zero = true;
							b_nonzero = false;
						}
					}
					if(!b_posexp && !b_nonzero && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
						if(!b_zero) {
							mbasecheck = new MathStructure(CHILD(0)[0]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mbasecheck->add(m_zero, OPERATION_EQUALS);
							else mbasecheck->add(m_zero, OPERATION_NOT_EQUALS);
							mbasecheck->isolate_x_sub(eo, eo2, x_var);
						}
						if(!b_negexp) {
							MathStructure *mexpcheck = new MathStructure(CHILD(0)[1]);
							if(ct_comp == COMPARISON_NOT_EQUALS) mexpcheck->add(m_zero, OPERATION_LESS);
							else mexpcheck->add(m_zero, OPERATION_EQUALS_GREATER);
							mexpcheck->isolate_x(eo2, eo);
							if(!b_zero) {
								if(ct_comp == COMPARISON_NOT_EQUALS) mexpcheck->add_nocopy(mbasecheck, OPERATION_LOGICAL_AND, true);
								else mexpcheck->add_nocopy(mbasecheck, OPERATION_LOGICAL_OR, true);
								mexpcheck->calculatesub(eo2, eo, false);
							}
							mbasecheck = mexpcheck;
						} else if(b_zero) {
							if(ct_comp == COMPARISON_EQUALS) {
								clear(true);
							} else if(ct_comp == COMPARISON_NOT_EQUALS) {
								set(1, 1, 0, true);
							}
							if(mcheckeven) delete mcheckeven;
							return true;
						}
					} else if(b_negexp && ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
						if(!b_zero && !b_nonzero) {
							mbasecheck = new MathStructure(CHILD(1));
							mbasecheck->add(m_zero, OPERATION_NOT_EQUALS);
							mbasecheck->isolate_x(eo2, eo);
							malternative2 = new MathStructure(CHILD(1));
							malternative2->add(m_zero, OPERATION_EQUALS);
							malternative2->isolate_x(eo2, eo);
							MathStructure *mcheck = new MathStructure(CHILD(0)[0]);
							if(ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_LESS) {
								mcheck->add(m_zero, OPERATION_LESS);
							} else if(ct_comp == COMPARISON_EQUALS_GREATER || ct_comp == COMPARISON_GREATER) {
								mcheck->add(m_zero, OPERATION_GREATER);
							}
							mcheck->isolate_x_sub(eo, eo2, x_var);
							malternative2->add_nocopy(mcheck, OPERATION_LOGICAL_AND, true);
							malternative2->calculatesub(eo2, eo, false);
						}
						if(b_zero && (ct_comp == COMPARISON_EQUALS_LESS || ct_comp == COMPARISON_EQUALS_GREATER)) {
							if(ct_comp == COMPARISON_EQUALS_LESS) {
								ct_comp = COMPARISON_LESS;
							} else if(ct_comp == COMPARISON_EQUALS_GREATER) {
								ct_comp = COMPARISON_GREATER;
							}
						} else if((b_pos1 || b_neg1) && !b_zero) {							
							if((b_pos1 && (ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS)) || (b_neg1 && (ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER))) {
								malternative = new MathStructure(CHILD(0)[0]);
								if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) malternative->add(m_zero, OPERATION_LESS);
								else malternative->add(m_zero, OPERATION_GREATER);
								malternative->isolate_x_sub(eo, eo2, x_var);								
								if(mbasecheck) {
									malternative->add(*mbasecheck, OPERATION_LOGICAL_AND, true);
									malternative->childToFront(malternative->size());
									malternative->calculatesub(eo2, eo, false);
								}
								if(ct_comp == COMPARISON_LESS) ct_comp = COMPARISON_GREATER;
								else if(ct_comp == COMPARISON_EQUALS_LESS) ct_comp = COMPARISON_EQUALS_GREATER;
								else if(ct_comp == COMPARISON_GREATER) ct_comp = COMPARISON_LESS;
								else ct_comp = COMPARISON_EQUALS_LESS;
							} else {
								if(ct_comp == COMPARISON_LESS) ct_comp = COMPARISON_GREATER;
								else if(ct_comp == COMPARISON_EQUALS_LESS) ct_comp = COMPARISON_EQUALS_GREATER;
								else if(ct_comp == COMPARISON_GREATER) ct_comp = COMPARISON_LESS;
								else ct_comp = COMPARISON_EQUALS_LESS;
								mcheck1 = new MathStructure(CHILD(0)[0]);
								if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) mcheck1->add(m_zero, OPERATION_LESS);
								else mcheck1->add(m_zero, OPERATION_GREATER);
								mcheck1->isolate_x_sub(eo, eo2, x_var);
							}
						} else if(!b_zero) {
							malternative = new MathStructure(CHILD(1));
							if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
								if(b_nonzero) malternative->add(m_zero, OPERATION_GREATER);
								else malternative->add(m_zero, OPERATION_EQUALS_GREATER);
							} else {
								if(b_nonzero) malternative->add(m_zero, OPERATION_LESS);
								else malternative->add(m_zero, OPERATION_EQUALS_LESS);
							}
							malternative->isolate_x(eo2, eo);								
							MathStructure *malt2 = new MathStructure(CHILD(0)[0]);
							if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
								malt2->add(m_zero, OPERATION_LESS);
							} else {
								malt2->add(m_zero, OPERATION_GREATER);
							}
							malt2->isolate_x_sub(eo, eo2, x_var);
							malternative->add_nocopy(malt2, OPERATION_LOGICAL_AND, true);
							if(mbasecheck) {
								malternative->add(*mbasecheck, OPERATION_LOGICAL_AND, true);
								malternative->childToFront(malternative->size());
							}
							malternative->calculatesub(eo2, eo, false);
							if(ct_comp == COMPARISON_LESS) ct_comp = COMPARISON_GREATER;
							else if(ct_comp == COMPARISON_EQUALS_LESS) ct_comp = COMPARISON_EQUALS_GREATER;
							else if(ct_comp == COMPARISON_GREATER) ct_comp = COMPARISON_LESS;
							else ct_comp = COMPARISON_EQUALS_LESS;
							mcheck1 = new MathStructure(CHILD(1));
							if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
								mcheck1->add(m_zero, OPERATION_LESS);
							} else {
								mcheck1->add(m_zero, OPERATION_GREATER);
							}
							mcheck1->isolate_x(eo2, eo);
							MathStructure *mcheck = new MathStructure(CHILD(0)[0]);
							if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
								mcheck->add(m_zero, OPERATION_GREATER);
							} else {
								mcheck->add(m_zero, OPERATION_LESS);
							}
							mcheck->isolate_x_sub(eo, eo2, x_var);
							mcheck1->add_nocopy(mcheck, OPERATION_LOGICAL_OR, true);
							mcheck1->calculatesub(eo2, eo, false);
						}
					}

					if(!b_zero) {
						MathStructure exp(1, 1);
						exp.calculateDivide(CHILD(0)[1], eo2);
						CHILD(1).calculateRaise(exp, eo2);
					}
					
					CHILD(0).setToChild(1);					
					CHILDREN_UPDATED
					if(pow_even && !b_neg1) {
						MathStructure *mcheck = NULL, *malt1 = NULL;
						if(!b_pos1) {
							malt1 = NULL;
							if(ct_comp == COMPARISON_EQUALS_LESS) {
								malt1 = new MathStructure(*this);
								malt1->setComparisonType(COMPARISON_EQUALS);
							} else if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {
								malt1 = new MathStructure(*this);
								malt1->setComparisonType(COMPARISON_EQUALS_GREATER);
								(*malt1)[1].clear(true);
							}
							if(malt1) {
								malt1->isolate_x_sub(eo, eo2, x_var);
								mcheck = new MathStructure(CHILD(1));
								mcheck->add(m_zero, OPERATION_LESS);
								mcheck->isolate_x(eo2, eo);
								malt1->add_nocopy(mcheck, OPERATION_LOGICAL_AND);
								malt1->swapChildren(2, 1);
								malt1->calculatesub(eo2, eo, false);
							}
							mcheck = new MathStructure(CHILD(1));
							mcheck->add(m_zero, OPERATION_EQUALS_GREATER);
							mcheck->isolate_x(eo2, eo);							
						}						
						if(!mcheckeven) isx_deabsify(CHILD(1));
						MathStructure *mchild2 = new MathStructure(*this);
						isolate_x_sub(eo, eo2, x_var, morig);
						(*mchild2)[0].calculateNegate(eo2);						
						mchild2->childUpdated(1);
						mchild2->isolate_x_sub(eo, eo2, x_var);
						if(mcheckeven) {
							mchild2->add_nocopy(mcheckeven, OPERATION_LOGICAL_AND, true);
							mchild2->calculatesub(eo2, eo, false);
						}
						if(ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
							transform_nocopy(STRUCT_LOGICAL_AND, mchild2);
						} else {
							transform_nocopy(STRUCT_LOGICAL_OR, mchild2);
						}
						calculatesub(eo2, eo, false);
						if(mcheck) {
							add_nocopy(mcheck, OPERATION_LOGICAL_AND);
							SWAP_CHILDREN(1, 0)
							calculatesub(eo2, eo, false);
						}
						if(malt1) {
							add_nocopy(malt1, OPERATION_LOGICAL_OR);
							calculatesub(eo2, eo, false);
						}
					} else if(b_neg1 && pow_even && mcheckeven) {
						MathStructure *malt1 = NULL;
						if(ct_comp == COMPARISON_EQUALS_LESS) {
							malt1 = new MathStructure(*this);
							malt1->setComparisonType(COMPARISON_EQUALS);
						} else if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {
							malt1 = new MathStructure(*this);
							malt1->setComparisonType(COMPARISON_EQUALS_GREATER);
							(*malt1)[1].clear(true);
						}
						isolate_x_sub(eo, eo2, x_var, morig);
						add_nocopy(mcheckeven, OPERATION_LOGICAL_AND);
						{SWAP_CHILDREN(1, 0)}
						calculatesub(eo2, eo, false);
						if(malt1) {
							malt1->isolate_x_sub(eo, eo2, x_var);
							add_nocopy(malt1, OPERATION_LOGICAL_OR);
							{SWAP_CHILDREN(1, 0)}
							calculatesub(eo2, eo, false);
						}
					} else {
						isolate_x_sub(eo, eo2, x_var, morig);
					}
					if(mbasecheck) {
						if(ct_comp == COMPARISON_NOT_EQUALS) add_nocopy(mbasecheck, OPERATION_LOGICAL_OR);
						else add_nocopy(mbasecheck, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(1, 0)
						calculatesub(eo2, eo, false);
					}
					if(mcheck1) {
						if(ct_comp == COMPARISON_NOT_EQUALS) add_nocopy(mcheck1, OPERATION_LOGICAL_OR);
						else add_nocopy(mcheck1, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(1, 0)
						calculatesub(eo2, eo, false);
					}
					if(malternative) {
						add_nocopy(malternative, OPERATION_LOGICAL_OR);
						if(!malternative2) calculatesub(eo2, eo, false);
					}
					if(malternative2) {
						add_nocopy(malternative2, OPERATION_LOGICAL_OR);
						calculatesub(eo2, eo, false);
					}
					return true;
				}
			} else if(CHILD(0)[1].contains(x_var)) {
				if(CHILD(1).isOne()) {
					if(!CHILD(0)[0].representsUndefined(true, true)) {
						CHILD(0).setToChild(2, true);
						CHILD(1).clear(true);
						isolate_x_sub(eo, eo2, x_var, morig);
						return true;
					}
				}
				if(ct_comp != COMPARISON_EQUALS && ct_comp != COMPARISON_NOT_EQUALS) {
					if(CHILD(0)[0].isNumber() && CHILD(0)[0].number().isReal() && CHILD(0)[0].number().isPositive()) {
						if(CHILD(1).representsNegative()) {
							if(ct_comp == COMPARISON_GREATER || ct_comp == COMPARISON_EQUALS_GREATER) {
								set(1, 1, 0, true);
							} else {
								clear(true);
							}
							return true;
						}
						if(CHILD(0)[0].number().isFraction()) {
							switch(ct_comp) {
								case COMPARISON_LESS: {ct_comp = COMPARISON_GREATER; break;}
								case COMPARISON_GREATER: {ct_comp = COMPARISON_LESS; break;}
								case COMPARISON_EQUALS_LESS: {ct_comp = COMPARISON_EQUALS_GREATER; break;}
								case COMPARISON_EQUALS_GREATER: {ct_comp = COMPARISON_EQUALS_LESS; break;}
								default: {}
							}
						}
					} else {
						return false;
					}
				}
				MathStructure msave(CHILD(1));
				CHILD(1).set(CALCULATOR->f_logn, &msave, &CHILD(0)[0], NULL);
				bool b = CHILD(1).calculateFunctions(eo);
				CHILD(1).unformat(eo);
				if(b) CHILD(1).calculatesub(eo2, eo, true);
				CHILD(0).setToChild(2, true);
				CHILDREN_UPDATED;				
				isolate_x_sub(eo, eo2, x_var, morig);
				return true;
			}
			break;
		}
		case STRUCT_FUNCTION: {
			if(CHILD(0).function() == CALCULATOR->f_ln && CHILD(0).size() == 1) {
				if(CHILD(0)[0].contains(x_var)) {					
					MathStructure msave(CHILD(1));
					CHILD(1).set(CALCULATOR->v_e);
					CHILD(1).calculateRaise(msave, eo2);
					CHILD(0).setToChild(1, true);
					CHILDREN_UPDATED;
					if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
						MathStructure *mand = new MathStructure(CHILD(0));
						mand->add(m_zero, OPERATION_GREATER);
						mand->isolate_x_sub(eo, eo2, x_var);
						isolate_x_sub(eo, eo2, x_var, morig);
						add_nocopy(mand, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(0, 1);
						calculatesub(eo2, eo, false);
					} else {
						isolate_x_sub(eo, eo2, x_var, morig);
					}
					return true;
				}
			} else if(CHILD(0).function() == CALCULATOR->f_lambert_w && CHILD(0).size() == 1 && (ct_comp == COMPARISON_EQUALS || ct_comp == COMPARISON_NOT_EQUALS)) {
				if(CHILD(0)[0].contains(x_var)) {					
					MathStructure msave(CHILD(1));
					CHILD(1).set(CALCULATOR->v_e);
					CHILD(1).calculateRaise(msave, eo2);
					CHILD(1).calculateMultiply(msave, eo2);
					CHILD(0).setToChild(1, true);
					CHILDREN_UPDATED;
					if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
						MathStructure *mand = new MathStructure(CHILD(0));
						mand->add(m_zero, OPERATION_GREATER);
						mand->isolate_x_sub(eo, eo2, x_var);
						isolate_x_sub(eo, eo2, x_var, morig);
						add_nocopy(mand, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(0, 1);
						calculatesub(eo2, eo, false);
					} else {
						isolate_x_sub(eo, eo2, x_var, morig);
					}
					return true;
				}
			} else if(CHILD(0).function() == CALCULATOR->f_logn && CHILD(0).size() == 2) {
				if(CHILD(0)[0].contains(x_var)) {
					MathStructure msave(CHILD(1));
					CHILD(1) = CHILD(0)[1];
					CHILD(1).calculateRaise(msave, eo2);
					CHILD(0).setToChild(1, true);
					CHILDREN_UPDATED;
					if(ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS) {
						MathStructure *mand = new MathStructure(CHILD(0));
						mand->add(m_zero, OPERATION_GREATER);
						mand->isolate_x_sub(eo, eo2, x_var);
						isolate_x_sub(eo, eo2, x_var, morig);
						add_nocopy(mand, OPERATION_LOGICAL_AND);
						SWAP_CHILDREN(0, 1);
						calculatesub(eo2, eo, false);
					} else {
						isolate_x_sub(eo, eo2, x_var, morig);
					}
					return true;
				}
			} else if(CHILD(0).function() == CALCULATOR->f_abs && CHILD(0).size() == 1) {
				if(CHILD(0)[0].contains(x_var)) {
					bool b_and = ct_comp == COMPARISON_NOT_EQUALS || ct_comp == COMPARISON_LESS || ct_comp == COMPARISON_EQUALS_LESS;
					CHILD(0).setToChild(1);
					CHILD_UPDATED(0)					
					MathStructure *mchild2 = new MathStructure(*this);
					isolate_x_sub(eo, eo2, x_var, morig);
					(*mchild2)[0].calculateNegate(eo2);
					mchild2->childUpdated(1);
					mchild2->isolate_x_sub(eo, eo2, x_var);
					if(b_and) {
						add_nocopy(mchild2, OPERATION_LOGICAL_AND);
					} else {
						add_nocopy(mchild2, OPERATION_LOGICAL_OR);
					}
					calculatesub(eo2, eo, false);
					return true;
				}
			}
			break;
		}
		default: {}
	}
	return false;
}

bool MathStructure::isolate_x(const EvaluationOptions &eo, const MathStructure &x_varp, bool check_result) {
	return isolate_x(eo, eo, x_varp, check_result);
}
bool MathStructure::isolate_x(const EvaluationOptions &eo, const EvaluationOptions &feo, const MathStructure &x_varp, bool check_result) {
	if(isProtected()) return false;
	if(!isComparison()) {		
		bool b = false;
		for(size_t i = 0; i < SIZE; i++) {
			if(CHILD(i).isolate_x(eo, feo, x_varp, check_result)) {
				CHILD_UPDATED(i);
				b = true;
			}
		}
		return b;
	}
	MathStructure x_var(x_varp);
	if(x_var.isUndefined()) {
		const MathStructure *x_var2;
		if(eo.isolate_var && contains(*eo.isolate_var)) x_var2 = eo.isolate_var;
		else x_var2 = &find_x_var();
		if(x_var2->isUndefined()) return false;
		x_var = *x_var2;
	}
	if(CHILD(0) == x_var && !CHILD(1).contains(x_var)) return true;
	if(!CHILD(1).isZero()) {
		CHILD(0).calculateSubtract(CHILD(1), eo);
		CHILD(1).clear(true);
		CHILDREN_UPDATED
	}
	EvaluationOptions eo2 = eo;
	eo2.calculate_functions = false;
	eo2.test_comparisons = false;
	eo2.isolate_x = false;
	if(!check_result) {
		return isolate_x_sub(feo, eo2, x_var);
	}
	if(CHILD(1).isZero() && CHILD(0).isAddition()) {
		bool found_1x = false;
		for(size_t i = 0; i < CHILD(0).size(); i++) {
			if(CHILD(0)[i] == x_var) {
				found_1x = true;
			} else if(CHILD(0)[i].contains(x_var)) {
				found_1x = false;
				break;
			}
		}
		if(found_1x) return isolate_x_sub(feo, eo2, x_var);
	}
	MathStructure msave(*this);
	if(isolate_x_sub(feo, eo2, x_var)) {
		return test_comparisons(msave, *this, x_var, eo);
	}
	return false;
	
}

bool MathStructure::isRationalPolynomial() const {
	switch(m_type) {
		case STRUCT_NUMBER: {
			return o_number.isRational() && !o_number.isZero();
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isAddition() || CHILD(i).isMultiplication() || !CHILD(i).isRationalPolynomial()) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isAddition() || !CHILD(i).isRationalPolynomial()) {
					return false;
				}
			}
			return true;
		}
		case STRUCT_POWER: {
			return CHILD(1).isInteger() && CHILD(1).number().isNonNegative() && !CHILD(0).isMultiplication() && !CHILD(0).isAddition() && !CHILD(0).isPower() && CHILD(0).isRationalPolynomial();
		}
		case STRUCT_FUNCTION: {}
		case STRUCT_UNIT: {}		
		case STRUCT_VARIABLE: {}
		case STRUCT_SYMBOLIC: {			
			return representsNonMatrix() && !representsUndefined(true, true);
		}
		default: {}
	}
	return false;
}
const Number &MathStructure::overallCoefficient() const {
	switch(m_type) {
		case STRUCT_NUMBER: {
			return o_number;
		}
		case STRUCT_MULTIPLICATION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isNumber()) {
					return CHILD(i).number();
				}
			}
			return nr_one;
		}
		case STRUCT_ADDITION: {
			for(size_t i = 0; i < SIZE; i++) {
				if(CHILD(i).isNumber()) {
					return CHILD(i).number();
				}
			}
			return nr_zero;
		}
		case STRUCT_POWER: {
			return nr_zero;
		}
		default: {}
	}
	return nr_zero;
}

