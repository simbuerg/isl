#ifndef ISL_INTERFACE_GENERATOR_H
#define ISL_INTERFACE_GENERATOR_H

#include <clang/AST/Decl.h>
#include <map>
#include <set>
#include <string>

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

	void print(map<string, isl_class> &classes, set<string> &done);
	void print_constructor(FunctionDecl *method);
	void print_method(FunctionDecl *method, bool subclass, string super);
};

bool is_subclass(RecordDecl *decl, string &super);
bool is_constructor(Decl *decl);
bool takes(Decl *decl);
isl_class &method2class(map<string, isl_class> &classes, FunctionDecl *fd);
bool is_isl_ctx(QualType type);
bool first_arg_is_isl_ctx(FunctionDecl *fd);
bool is_isl_type(QualType type);
bool is_callback(QualType type);
bool is_string(QualType type);
string extract_type(QualType type);

#endif
