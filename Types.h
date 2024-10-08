//
// Created by Muham on 7/14/2024.
//

#ifndef COMPILATION_HW3_TYPES_H
#define COMPILATION_HW3_TYPES_H

#include "hw3_output.hpp"
#include "ScopesTable.h"
#define YYSTYPE Node*
using namespace output;

class Node;
class Program;
class Statements;
class Statement;
class Call;
class Type;
class Exp;

class Node {
public:
    string m_type;
    string m_text;
    string m_var;
    string m_trueLabel;
    string m_falseLabel;
    string m_nextLabel;

    explicit Node(const string& type = "");
    bool checkNumber() const;
    bool checkInt() const;
    bool checkBool() const;
    virtual ~Node() = default;
};

class Program : public Node {
public:
    Program();
};

class Statements : public Node {
public:
    Statements() = default;
};

class Statement : public Node {
public:
    Statement();
    explicit Statement(Node* node);
    explicit Statement(Call* call);
    explicit Statement(Statements* statements);
    Statement(Type* type, Node* node, Exp * exp = nullptr);
    Statement(Node* node, Exp* exp);
    Statement(Exp* exp, Statement* statement1, Statement* statement2 = nullptr);
};

class Call : public Node {
public:
    Call(Node* node, Exp* exp);
    void callFunction(Node *node, Exp *exp, ScopeTableFunctionEntry* entry);
};

class Type : public Node {
public:
    explicit Type(const string& type);
};

class Exp : public Node {
public:
    explicit Exp(Node* node);
    explicit Exp(Call* call);
    Exp(Node* node, const string& op);
    Exp(Exp* exp1, Node* op, Exp* exp2);
    Exp(Type* type, Exp* exp);
    void processArithmetic(Exp *exp1, Node *op, Exp *exp2);
    void processAndOr(Exp *exp1, Node *op, Exp *exp2);
    void processRelop(Exp *exp1, Node *op, Exp *exp2);
};

template <typename T>
T* nodeTo(Node* ptr)
{
    return dynamic_cast<T*>(ptr);
}

void CheckConditionIsBool(Node* node);
void initProgram();

#endif //COMPILATION_HW3_TYPES_H
