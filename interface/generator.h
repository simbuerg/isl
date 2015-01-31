#ifndef ISL_INTERFACE_GENERATOR_H
#define ISL_INTERFACE_GENERATOR_H

#include <clang/AST/Decl.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include <cerrno>
#include <sys/stat.h>
#include <sys/types.h>

using namespace clang;
using namespace std;

/* isl_class collects all constructors and methods for an isl "class".
 * "name" is the name of the class.
 * "type" is the declaration that introduces the type.
 */
struct isl_class {
	string name;
	RecordDecl *type;
	set<FunctionDecl *> constructors;
	set<FunctionDecl *> methods;
};

struct isl_enum {
	string name;
	map<string,int> values;

	string name_without_enum(const string &valname) const;
};

/* Base class for interface generators.
 */
class generator {
private:
	/* Mapping from file names (for generated files) to
	 * string output streams for the file contents.
	 */
	map<string,ostringstream*> files;

protected:
	map<string,isl_class> classes;
	map<string,isl_enum>  enums;

public:
	generator(set<RecordDecl *> &types, set<FunctionDecl *> &functions,
		  set<EnumDecl *> &enums);
	virtual ~generator();

	virtual void generate() = 0;

	/* Write generated files to the given directory. */
	void write_generated_files(const string &directory);

protected:
	/* Obtain the output stream for a given file name. */
	ostream &outputfile(const string &name);

	bool is_subclass(RecordDecl *decl, string &super);
	bool is_constructor(Decl *decl);
	void validate_constructor(const isl_class &clazz, FunctionDecl *fdecl);
	void validate_this(const isl_class &clazz, FunctionDecl *fdecl);
	bool takes(Decl *decl);
	bool gives(Decl *decl);
	isl_class &method2class(map<string, isl_class> &classes,
				FunctionDecl *fd);
	bool is_isl_ctx(QualType type);
	bool first_arg_is_isl_ctx(FunctionDecl *fd);
	bool is_isl_type(QualType type);
	bool is_callback(QualType type);
	bool is_string(QualType type);
	bool is_isl_class(QualType type);
	bool is_isl_enum(QualType type);
	string extract_type(QualType type);

	const isl_enum &find_enum(QualType type);

	bool can_be_printed(const isl_class &clazz) const;
};

#endif
