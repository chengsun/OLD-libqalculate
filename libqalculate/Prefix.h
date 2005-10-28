/*
    Qalculate    

    Copyright (C) 2003  Niklas Knutsson (nq@altern.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PREFIX_H
#define PREFIX_H


#include <libqalculate/includes.h>
#include <libqalculate/Number.h>

enum {
	PREFIX_DECIMAL,
	PREFIX_BINARY,
	PREFIX_NUMBER
};

class Prefix {
  protected:
	string l_name, s_name, u_name;
  public:
  	Prefix(string long_name, string short_name = "", string unicode_name = "");
	virtual ~Prefix();
	const string &shortName(bool return_long_if_no_short = true, bool use_unicode = false) const;
	const string &longName(bool return_short_if_no_long = true, bool use_unicode = false) const;
	const string &unicodeName(bool return_short_if_no_uni = true) const;
	void setShortName(string short_name);
	void setLongName(string long_name);
	void setUnicodeName(string unicode_name);
	const string &name(bool short_default = true, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	virtual Number value(const Number &nexp) const = 0;
	virtual Number value(int iexp) const = 0;
	virtual Number value() const = 0;
	virtual int type() const = 0;
	
};

class DecimalPrefix : public Prefix {
  protected:
	int exp;
  public:
  	DecimalPrefix(int exp10, string long_name, string short_name = "", string unicode_name = "");
	~DecimalPrefix();
	int exponent(int iexp = 1) const;
	Number exponent(const Number &nexp) const;	
	void setExponent(int iexp);	
	Number value(const Number &nexp) const;
	Number value(int iexp) const;
	Number value() const;
	int type() const;
};

class BinaryPrefix : public Prefix {
  protected:
	int exp;
  public:
  	BinaryPrefix(int exp2, string long_name, string short_name = "", string unicode_name = "");
	~BinaryPrefix();
	int exponent(int iexp = 1) const;
	Number exponent(const Number &nexp) const;	
	void setExponent(int iexp);	
	Number value(const Number &nexp) const;
	Number value(int iexp) const;
	Number value() const;
	int type() const;
};

class NumberPrefix : public Prefix {
  protected:
	Number o_number;
  public:
  	NumberPrefix(const Number &nr, string long_name, string short_name = "", string unicode_name = "");
	~NumberPrefix();
	Number setValue(const Number &nr);
	Number value(const Number &nexp) const;
	Number value(int iexp) const;
	Number value() const;
	int type() const;
};

#endif
