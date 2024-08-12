//
// Created by Muham on 7/14/2024.
//

#include "ScopesTable.h"
#include "hw3_output.hpp"
#include <iostream>
#include "cg.hpp"
using namespace output;

ScopesTable* ScopesTable::instance = nullptr;

ScopeTableEntry::ScopeTableEntry(const string &name, const string &type, int offset)
                : name(name), type(type), offset(offset) {}

//void ScopeTableEntry::printEntry() const
//{
//    printID(name, offset, type);
//}

ScopeTableFunctionEntry::ScopeTableFunctionEntry(const string& name, const string& type, int offset,
                                                 const std::string &paramType) : ScopeTableEntry(name, type, offset),
                                                 paramType(paramType) {}

//void ScopeTableFunctionEntry::printEntry() const
//{
//    printID(name, 0, makeFunctionType(paramType, type));
//}

ScopesTable *ScopesTable::getInstance()
{
    if(instance == nullptr)
        instance = new ScopesTable();
    return instance;
}

ScopesTable::ScopesTable() : scopesTable(), offsetsTable()
{
    offsetsTable.push_back(0);
    addScope();
    offsetsTable.pop_back();
    scopesTable[0]->addFuncEntry("print", "VOID", 0, "STRING");
    scopesTable[0]->addFuncEntry("printi", "VOID", 0, "INT");
    scopesTable[0]->addFuncEntry("readi", "INT", 0, "INT");
}

void ScopesTable::addScope(bool isWhile)
{
    scopesTable.push_back(new SingleScopeTable(isWhile));
    int lastOffset = offsetsTable[offsetsTable.size() - 1];
    offsetsTable.push_back(lastOffset);
}

void ScopesTable::removeScope()
{
    SingleScopeTable *table = scopesTable[scopesTable.size() - 1];
//    table->printSymbols();
    scopesTable.pop_back();
    offsetsTable.pop_back();
}

void ScopesTable::addVarToLastScope(string name, string type)
{
    if(scopesTable.empty())
        return;
    int offset = offsetsTable[offsetsTable.size() - 1]++;
    scopesTable[scopesTable.size() - 1]->addEntry(name, type, offset);
}

bool ScopesTable::symbolExists(const string& symbol) const
{
    for(SingleScopeTable* table : scopesTable)
    {
        if(table->symbolExists(symbol))
            return true;
    }
    return false;
}

ScopeTableEntry *ScopesTable::getSymbol(const string &symbol) const
{
    for(SingleScopeTable* table : scopesTable)
    {
        ScopeTableEntry *entry = table->getSymbol(symbol);
        if(entry != nullptr)
            return entry;
    }
    return nullptr;
}

bool ScopesTable::hasWhileScope() const
{
    for(SingleScopeTable* table : scopesTable)
    {
        if(table->isWhileScope())
            return true;
    }
    return false;
}

SingleScopeTable::SingleScopeTable(bool isWhile) : table(), isWhile(isWhile), beforeWhileCond(), nextLabel()
{
    if(!isWhile)
        return;
    CodeBuffer& buffer = CodeBuffer::instance();
    beforeWhileCond = buffer.freshLabel();
    buffer.emit("br label %" + beforeWhileCond);
    buffer.emit(beforeWhileCond + ":");
}

bool SingleScopeTable::symbolExists(const string& symbol) const
{
    for(ScopeTableEntry* entry : table)
    {
        if(entry->name == symbol)
            return true;
    }
    return false;
}

ScopeTableEntry* SingleScopeTable::getSymbol(const string& symbol) const
{
    for(ScopeTableEntry* entry : table)
    {
        if(entry->name == symbol)
            return entry;
    }
    return nullptr;
}

void SingleScopeTable::addEntry(string name, string type, int offset)
{
    ScopeTableEntry* newEntry = new ScopeTableEntry(name, type, offset);
    table.push_back(newEntry);
}

bool SingleScopeTable::isWhileScope() const
{
    return isWhile;
}

void SingleScopeTable::addFuncEntry(const std::string &name, const std::string &type, int offset,
                                    const std::string &paramType)
{
    ScopeTableFunctionEntry* funcEntry = new ScopeTableFunctionEntry(name, type, offset, paramType);
    table.push_back(funcEntry);
}

//void SingleScopeTable::printSymbols() const
//{
//    endScope();
//    for(ScopeTableEntry* entry : table)
//    {
//        entry->printEntry();
//    }
//}

ScopeTableFunctionEntry *ScopesTable::getFunction(const std::string &name) const
{
    SingleScopeTable* table = scopesTable[0];
    ScopeTableEntry* entry = table->getSymbol(name);
    ScopeTableFunctionEntry* functionEntry;
    try {
        functionEntry = dynamic_cast<ScopeTableFunctionEntry*>(entry);
    } catch (...) {
        return nullptr;
    }
    return functionEntry;
}

ScopeTableEntry *ScopesTable::getVar(const string &symbol) const {
    ScopeTableEntry *var = getSymbol(symbol);
    if(getFunction(symbol) == nullptr)
        return var;
    return nullptr;
}

string ScopesTable::getLastWhileBeforeLabel() const {
    string label;
    for(SingleScopeTable* table : scopesTable)
    {
        if(table->isWhileScope())
            label = table->beforeWhileCond;
    }
    return label;
}

SingleScopeTable *ScopesTable::getLastWhileScope() {
    SingleScopeTable *res = nullptr;
    for(SingleScopeTable* table : scopesTable)
    {
        if(table->isWhileScope())
            res = table;
    }
    return res;
}

string ScopesTable::getLastWhileNextLabel() {
    return ScopesTable::getLastWhileScope()->nextLabel;
}
