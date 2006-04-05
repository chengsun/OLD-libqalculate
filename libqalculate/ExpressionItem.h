/*
    Qalculate    

    Copyright (C) 2003  Niklas Knutsson (nq@altern.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef EXPRESSIONITEM_H
#define EXPRESSIONITEM_H

#include <libqalculate/includes.h>


/// A name for an expression item (function, variable or unit)
/** An expression name has a text string representing a name and boolean values describing the names properties.
*/
struct ExpressionName {
	
	/// If the name is an abbreviation.
	bool abbreviation;
	/// If the name has a suffix. If set to true, the part of the name after an underscore should be treated as a suffix.
	bool suffix;
	/// If the name contains unicode characters.
	bool unicode;
	/// If the name is in plural form.
	bool plural;
	/// If the name shall be used as a fixed reference. If this is set to true, the name will kept as it is in addition to translations of it.
	bool reference;
	/// If the name is unsuitable for user input.
	bool avoid_input;
	/// If the name is case sensitive. The default behavior is that abbreviations are case sensitive and other names are not.
	bool case_sensitive;
	/// The name.
	string name;
	
	/** Create an empty expression name. All properties are set to false.
	*/
	ExpressionName();
	/** Create an expression name. All properties are set to false, unless the name only has one character in which case abbreviation and case_sesnsitive is set to true.
	*
	* @param sname The name.
	*/
	ExpressionName(string sname);
	
	void operator = (const ExpressionName &ename);
	bool operator == (const ExpressionName &ename) const;
	bool operator != (const ExpressionName &ename) const;
	
};

/// Abstract base class for functions, variables and units.
/** 
*/
class ExpressionItem {

  protected:

	string scat, stitle, sdescr;
	bool b_local, b_changed, b_builtin, b_approx, b_active, b_registered, b_hidden, b_destroyed;
	int i_ref, i_precision;
	vector<ExpressionItem*> v_refs;
	vector<ExpressionName> names;

  public:

	ExpressionItem(string cat_, string name_, string title_ = "", string descr_ = "", bool is_local = true, bool is_builtin = false, bool is_active = true);
	ExpressionItem();
	virtual ~ExpressionItem();
	
	virtual ExpressionItem *copy() const = 0;
	virtual void set(const ExpressionItem *item);
	
	virtual bool destroy();

	bool isRegistered() const;
	void setRegistered(bool is_registered);

	virtual const string &name(bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	virtual const string &referenceName() const;

	/** Returns the name that best fulfils provided criterias. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param abbreviation If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param abbreviation If a name in plural form is preferred.
	* @param abbreviation If a reference name is preferred (ignored if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	virtual const ExpressionName &preferredName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns the name that best fulfils provided criterias and is suitable for user input. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param abbreviation If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param abbreviation If a name in plural form is preferred.
	* @param abbreviation If a reference name is preferred (ignored if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	virtual const ExpressionName &preferredInputName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns the name that best fulfils provided criterias and is suitable for display. If two names are equally preferred, the one with lowest index is returned.
	*
	* @param abbreviation If an abbreviated name is preferred.
	* @param abbreviation If a name with unicode characters can be displayed/is preferred (prioritized if false).
	* @param abbreviation If a name in plural form is preferred.
	* @param abbreviation If a reference name is preferred (ignored if false).
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The preferred name.
	*/
	virtual const ExpressionName &preferredDisplayName(bool abbreviation = false, bool use_unicode = false, bool plural = false, bool reference = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	/** Returns name for an index (starting at one). All functions can be traversed by starting at index one and increasing the index until empty_expression_name is returned.
	*
	* @param index Index of name.
	* @returns Name for index or empty_expression_name if not found.
	*/
	virtual const ExpressionName &getName(size_t index) const;
	/** Changes a name. If a name for the provided index is not present, it is added (equivalent to addName(ename, index, force)).
	*
	* @param ename The new name.
	* @param index Index of name to change.
	* @param force If true, expression items with conflicting names are replaced, otherwise . Only applies if the item is registered.
	*/
	virtual void setName(const ExpressionName &ename, size_t index = 1, bool force = true);
	/** Changes the text string of a name. If a name for the provided index is not present, it is added (equivalent to addName(sname, index, force)).
	*
	* @param sname The new name text string.
	* @param index Index of name to change.
	* @param force If true, expression items with conflicting names are replaced, otherwise . Only applies if the item is registered.
	*/
	virtual void setName(string sname, size_t index, bool force = true);
	virtual void addName(const ExpressionName &ename, size_t index = 0, bool force = true);
	virtual void addName(string sname, size_t index = 0, bool force = true);
	virtual size_t countNames() const;
	/** Removes all names. */
	virtual void clearNames();
	/** Removes all names that are not used for reference (ExpressionName.reference = true). */
	virtual void clearNonReferenceNames();
	virtual void removeName(size_t index);
	/** Checks if the expression item has a name with a specific text string.
	*
	* @param sname A text string to look for (not case sensitive)
	* @returns true if the item has a name with the given text string.
	*/
	virtual bool hasName(const string &sname) const;
	/** Checks if the expression item has a name with a specific case sensitive text string.
	*
	* @param sname A text string to look for (case sensitive)
	* @returns true if the item has a name with the given text string.
	*/
	virtual bool hasNameCaseSensitive(const string &sname) const;
	/** Searches for a name with specific properties.
	*
	* @param abbreviation If the name must be abbreviated. 1=true, 0=false, -1=ignore.
	* @param use_unicode If the name must have unicode characters. 1=true, 0=false, -1=ignore.
	* @param plural If the name must be in plural form. 1=true, 0=false, -1=ignore.
	* @param can_display_unicode_string_function Function that tests if the unicode characters in a name can be displayed. If the function returns false, the name will be rejected.
	* @param can_display_unicode_string_arg Argument to pass to the above test function.
	* @returns The first found name with the specified properties or empty_expression_name if none found.
	*/
	virtual const ExpressionName &findName(int abbreviation = -1, int use_unicode = -1, int plural = -1, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	
	virtual const string &title(bool return_name_if_no_title = true, bool use_unicode = false, bool (*can_display_unicode_string_function) (const char*, void*) = NULL, void *can_display_unicode_string_arg = NULL) const;
	
	virtual void setTitle(string title_);

	/** Returns the expression items description.
	*
	* @returns Description.
	*/
	virtual const string &description() const;
	/** Sets the expression items description.
	*
	* @param descr_ Description.
	*/
	virtual void setDescription(string descr_);

	/** Returns the category that the expression item belongs to. Subcategories are separated by '/'.
	*
	* @returns Category.
	*/
	virtual const string &category() const;
	/** Sets which category the expression belongs to. Subcategories are separated by '/'.
	*
	* @param cat_ Category.
	*/
	virtual void setCategory(string cat_);

	virtual bool hasChanged() const;
	virtual void setChanged(bool has_changed);
	
	virtual bool isLocal() const;
	virtual bool setLocal(bool is_local = true, int will_be_active = -1);
	
	virtual bool isBuiltin() const;
	
	virtual bool isApproximate() const;
	virtual void setApproximate(bool is_approx = true);
	
	virtual int precision() const;
	virtual void setPrecision(int prec);

	/** Returns if the expression item is active and can be used in expressions.
	*
	* @returns true if active.
	*/
	virtual bool isActive() const;
	virtual void setActive(bool is_active);
	
	virtual bool isHidden() const;
	virtual void setHidden(bool is_hidden);
	
	virtual int refcount() const;
	/** The reference count is not used to delete the expression item when it becomes zero, but to stop from being deleted while it is in use.
	*/
	virtual void ref();
	virtual void unref();
	virtual void ref(ExpressionItem *o);
	virtual void unref(ExpressionItem *o);
	virtual ExpressionItem *getReferencer(size_t index = 1) const;
	virtual bool changeReference(ExpressionItem *o_from, ExpressionItem *o_to);

	/** Returns the type of the expression item, corresponding to which subclass the object belongs to (TYPE_FUNCTION, TYPE_VARIABLE or TYPE_UNIT).
	*
	* @returns Type/subclass.
	*/
	virtual int type() const = 0;
	/** Returns the subtype of the expression item, corresponding to which subsubclass the object belongs to.
	*
	* @returns Subtype/subsubclass.
	*/
	virtual int subtype() const = 0;

};

#endif
