//
// Created by Muham on 7/14/2024.
//

#ifndef COMPILATION_HW3_SCOPESTABLE_H
#define COMPILATION_HW3_SCOPESTABLE_H

#include <vector>
#include <string>

using std::string;
using std::vector;
class SingleScopeTable;

class ScopeTableEntry {
public:
    string name;
    string type;
    int offset;
    ScopeTableEntry(const string& name, const string& type, int offset);
    virtual void printEntry() const;
};

class ScopeTableFunctionEntry : public ScopeTableEntry {
public:
    string paramType;
    ScopeTableFunctionEntry(const string& name, const string& type, int offset, const string& paramType);
    void printEntry() const override;
};


class ScopesTable {
private:
    vector<SingleScopeTable*> scopesTable;
    vector<int> offsetsTable;
    static ScopesTable* instance;
    ScopesTable();

public:
    static ScopesTable* getInstance();
    ScopesTable(ScopesTable const&) = delete; // disable copy ctor
    void operator=(ScopesTable const&) = delete; // disable = operator
    void addScope(bool isWhile = false);
    void removeScope();
    void addVarToLastScope(string name, string type);
    bool symbolExists(const string& symbol) const;
    ScopeTableEntry* getSymbol(const string& symbol) const;
    bool hasWhileScope() const;
    ScopeTableFunctionEntry* getFunction(const string& name) const;
    ScopeTableEntry* getVar(const string& symbol) const;
};


class SingleScopeTable {
private:
    vector<ScopeTableEntry*> table;
    bool isWhile;

public:
    SingleScopeTable(bool isWhile = false);
    void addEntry(string name, string type, int offset);
    bool symbolExists(const string& symbol) const;
    ScopeTableEntry* getSymbol(const string& symbol) const;
    bool isWhileScope() const;
    void addFuncEntry(const string& name, const string& type, int offset, const string& paramType);
    void printSymbols() const;
};

#endif //COMPILATION_HW3_SCOPESTABLE_H
