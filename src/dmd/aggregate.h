
/* Compiler implementation of the D programming language
 * Copyright (C) 1999-2020 by The D Language Foundation, All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/dlang/dmd/blob/master/src/dmd/aggregate.h
 */

#pragma once

#include "dsymbol.h"
#include "objc.h"

class AliasThis;
class Identifier;
class Type;
class TypeFunction;
class Expression;
class FuncDeclaration;
class CtorDeclaration;
class DtorDeclaration;
class NewDeclaration;
class InterfaceDeclaration;
class TypeInfoClassDeclaration;
class VarDeclaration;

enum class Sizeok : uint8_t
{
    none,         // size of aggregate is not yet able to compute
    fwd,          // size of aggregate is ready to compute
    inProcess,    // in the midst of computing the size
    done          // size of aggregate is set correctly
};

enum class Baseok : uint8_t
{
    none,         // base classes not computed yet
    in,           // in process of resolving base classes
    done,         // all base classes are resolved
    semanticdone  // all base classes semantic done
};

enum StructPOD
{
    ISPODno,            // struct is not POD
    ISPODyes,           // struct is POD
    ISPODfwd            // POD not yet computed
};

enum class Abstract : uint8_t
{
    fwdref = 0,      // whether an abstract class is not yet computed
    yes,             // is abstract class
    no               // is not abstract class
};

FuncDeclaration *search_toString(StructDeclaration *sd);

enum class ClassKind : uint8_t
{
  /// the aggregate is a d(efault) struct/class/interface
  d,
  /// the aggregate is a C++ struct/class/interface
  cpp,
  /// the aggregate is an Objective-C class/interface
  objc
};

class AggregateDeclaration : public ScopeDsymbol
{
public:
    Type *type;
    StorageClass storage_class;
    unsigned structsize;        // size of struct
    unsigned alignsize;         // size of struct for alignment purposes
    VarDeclarations fields;     // VarDeclaration fields
    Dsymbol *deferred;          // any deferred semantic2() or semantic3() symbol

    ClassKind classKind;        // specifies the linkage type
    CPPMANGLE cppmangle;

    /* !=NULL if is nested
     * pointing to the dsymbol that directly enclosing it.
     * 1. The function that enclosing it (nested struct and class)
     * 2. The class that enclosing it (nested class only)
     * 3. If enclosing aggregate is template, its enclosing dsymbol.
     * See AggregateDeclaraton::makeNested for the details.
     */
    Dsymbol *enclosing;
    VarDeclaration *vthis;      // 'this' parameter if this aggregate is nested
    VarDeclaration *vthis2;     // 'this' parameter if this aggregate is a template and is nested
    // Special member functions
    FuncDeclarations invs;              // Array of invariants
    FuncDeclaration *inv;               // invariant
    NewDeclaration *aggNew;             // allocator

    Dsymbol *ctor;                      // CtorDeclaration or TemplateDeclaration

    // default constructor - should have no arguments, because
    // it would be stored in TypeInfo_Class.defaultConstructor
    CtorDeclaration *defaultCtor;

    AliasThis *aliasthis;       // forward unresolved lookups to aliasthis

    DtorDeclarations dtors;     // Array of destructors
    DtorDeclaration *dtor;      // aggregate destructor
    DtorDeclaration *primaryDtor; // non-deleting C++ destructor, same as dtor for D
    DtorDeclaration *tidtor;    // aggregate destructor used in TypeInfo (must have extern(D) ABI)
    FuncDeclaration *fieldDtor;   // aggregate destructor for just the fields

    Expression *getRTInfo;      // pointer to GC info generated by object.RTInfo(this)

    Prot protection;
    bool noDefaultCtor;         // no default construction
    Sizeok sizeok;              // set when structsize contains valid data

    virtual Scope *newScope(Scope *sc);
    void setScope(Scope *sc);
    bool determineFields();
    size_t nonHiddenFields();
    bool determineSize(Loc loc);
    virtual void finalizeSize() = 0;
    d_uns64 size(const Loc &loc);
    bool fill(Loc loc, Expressions *elements, bool ctorinit);
    Type *getType();
    bool isDeprecated() const;         // is aggregate deprecated?
    void setDeprecated();
    bool isNested() const;
    bool isExport() const;
    Dsymbol *searchCtor();

    Prot prot();

    // 'this' type
    Type *handleType() { return type; }

    // Back end
    Symbol *stag;               // tag symbol for debug data
    Symbol *sinit;

    AggregateDeclaration *isAggregateDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

struct StructFlags
{
    enum Type
    {
        none = 0x0,
        hasPointers = 0x1  // NB: should use noPointers as in ClassFlags
    };
};

class StructDeclaration : public AggregateDeclaration
{
public:
    bool zeroInit;              // !=0 if initialize with 0 fill
    bool hasIdentityAssign;     // true if has identity opAssign
    bool hasBlitAssign;         // true if opAssign is a blit
    bool hasIdentityEquals;     // true if has identity opEquals
    bool hasNoFields;           // has no fields
    bool hasCopyCtor;           // copy constructor
    // Even if struct is defined as non-root symbol, some built-in operations
    // (e.g. TypeidExp, NewExp, ArrayLiteralExp, etc) request its TypeInfo.
    // For those, today TypeInfo_Struct is generated in COMDAT.
    bool requestTypeInfo;

    FuncDeclarations postblits; // Array of postblit functions
    FuncDeclaration *postblit;  // aggregate postblit

    FuncDeclaration *xeq;       // TypeInfo_Struct.xopEquals
    FuncDeclaration *xcmp;      // TypeInfo_Struct.xopCmp
    FuncDeclaration *xhash;     // TypeInfo_Struct.xtoHash
    static FuncDeclaration *xerreq;      // object.xopEquals
    static FuncDeclaration *xerrcmp;     // object.xopCmp

    structalign_t alignment;    // alignment applied outside of the struct
    StructPOD ispod;            // if struct is POD

    // ABI-specific type(s) if the struct can be passed in registers
    TypeTuple *argTypes;

    static StructDeclaration *create(Loc loc, Identifier *id, bool inObject);
    Dsymbol *syntaxCopy(Dsymbol *s);
    void semanticTypeInfoMembers();
    Dsymbol *search(const Loc &loc, Identifier *ident, int flags = SearchLocalsOnly);
    const char *kind() const;
    void finalizeSize();
    bool fit(const Loc &loc, Scope *sc, Expressions *elements, Type *stype);
    bool isPOD();

    StructDeclaration *isStructDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }

    unsigned numArgTypes() const;
    Type *argType(unsigned index);
};

class UnionDeclaration : public StructDeclaration
{
public:
    Dsymbol *syntaxCopy(Dsymbol *s);
    const char *kind() const;

    UnionDeclaration *isUnionDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};

struct BaseClass
{
    Type *type;                         // (before semantic processing)

    ClassDeclaration *sym;
    unsigned offset;                    // 'this' pointer offset
    // for interfaces: Array of FuncDeclaration's
    // making up the vtbl[]
    FuncDeclarations vtbl;

    DArray<BaseClass> baseInterfaces;   // if BaseClass is an interface, these
                                        // are a copy of the InterfaceDeclaration::interfaces

    bool fillVtbl(ClassDeclaration *cd, FuncDeclarations *vtbl, int newinstance);
};

struct ClassFlags
{
    enum Type
    {
        none = 0x0,
        isCOMclass = 0x1,
        noPointers = 0x2,
        hasOffTi = 0x4,
        hasCtor = 0x8,
        hasGetMembers = 0x10,
        hasTypeInfo = 0x20,
        isAbstract = 0x40,
        isCPPclass = 0x80,
        hasDtor = 0x100
    };
};

class ClassDeclaration : public AggregateDeclaration
{
public:
    static ClassDeclaration *object;
    static ClassDeclaration *throwable;
    static ClassDeclaration *exception;
    static ClassDeclaration *errorException;
    static ClassDeclaration *cpp_type_info_ptr;

    ClassDeclaration *baseClass;        // NULL only if this is Object
    FuncDeclaration *staticCtor;
    FuncDeclaration *staticDtor;
    Dsymbols vtbl;                      // Array of FuncDeclaration's making up the vtbl[]
    Dsymbols vtblFinal;                 // More FuncDeclaration's that aren't in vtbl[]

    BaseClasses *baseclasses;           // Array of BaseClass's; first is super,
                                        // rest are Interface's

    DArray<BaseClass*> interfaces;      // interfaces[interfaces_dim] for this class
                                        // (does not include baseClass)

    BaseClasses *vtblInterfaces;        // array of base interfaces that have
                                        // their own vtbl[]

    TypeInfoClassDeclaration *vclassinfo;       // the ClassInfo object for this ClassDeclaration
    bool com;                           // true if this is a COM class (meaning it derives from IUnknown)
    bool stack;                         // true if this is a scope class
    int cppDtorVtblIndex;               // slot reserved for the virtual destructor [extern(C++)]
    bool inuse;                         // to prevent recursive attempts

    Abstract isabstract;                // 0: fwdref, 1: is abstract class, 2: not abstract
    Baseok baseok;                      // set the progress of base classes resolving
    ObjcClassDeclaration objc;          // Data for a class declaration that is needed for the Objective-C integration
    Symbol *cpp_type_info_ptr_sym;      // cached instance of class Id.cpp_type_info_ptr

    static ClassDeclaration *create(Loc loc, Identifier *id, BaseClasses *baseclasses, Dsymbols *members, bool inObject);
    Dsymbol *syntaxCopy(Dsymbol *s);
    Scope *newScope(Scope *sc);
    bool isBaseOf2(ClassDeclaration *cd);

    #define OFFSET_RUNTIME 0x76543210
    #define OFFSET_FWDREF 0x76543211
    virtual bool isBaseOf(ClassDeclaration *cd, int *poffset);

    bool isBaseInfoComplete();
    Dsymbol *search(const Loc &loc, Identifier *ident, int flags = SearchLocalsOnly);
    ClassDeclaration *searchBase(Identifier *ident);
    void finalizeSize();
    bool hasMonitor();
    bool isFuncHidden(FuncDeclaration *fd);
    FuncDeclaration *findFunc(Identifier *ident, TypeFunction *tf);
    bool isCOMclass() const;
    virtual bool isCOMinterface() const;
    bool isCPPclass() const;
    virtual bool isCPPinterface() const;
    bool isAbstract();
    virtual int vtblOffset() const;
    const char *kind() const;

    void addLocalClass(ClassDeclarations *);
    void addObjcSymbols(ClassDeclarations *classes, ClassDeclarations *categories);

    // Back end
    Dsymbol *vtblsym;
    Dsymbol *vtblSymbol();

    ClassDeclaration *isClassDeclaration() { return (ClassDeclaration *)this; }
    void accept(Visitor *v) { v->visit(this); }
};

class InterfaceDeclaration : public ClassDeclaration
{
public:
    Dsymbol *syntaxCopy(Dsymbol *s);
    Scope *newScope(Scope *sc);
    bool isBaseOf(ClassDeclaration *cd, int *poffset);
    bool isBaseOf(BaseClass *bc, int *poffset);
    const char *kind() const;
    int vtblOffset() const;
    bool isCPPinterface() const;
    bool isCOMinterface() const;

    InterfaceDeclaration *isInterfaceDeclaration() { return this; }
    void accept(Visitor *v) { v->visit(this); }
};
