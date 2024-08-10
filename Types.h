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
//Node Methods
    Node(const string& type = "");
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
    Statement() = default;
    Statement(Node* node);
    Statement(Call* call);
    Statement(Statements* statements);
    Statement(Type* type, Node* node, Exp * exp = nullptr);
    Statement(Node* node, Exp* exp);
    Statement(Exp* exp, Statement* statement1, Statement* statement2 = nullptr);
};

class Call : public Node {
public:
    Call(Exp* exp);
    Call(Node* node, Exp* exp);
};

class Type : public Node {
public:
    Type(const string& type);
};

class Exp : public Node {
public:
    Exp(Node* node);
    Exp(Exp* exp);
    Exp(Call* call);
    Exp(Node* node, const string& op);
    Exp(Exp* exp, const string& op);
    Exp(Exp* exp1, Node* op, Exp* exp2);
    Exp(Type* type, Exp* exp);
};

template <typename T>
T* nodeTo(Node* ptr)
{
    return dynamic_cast<T*>(ptr);
}

void CheckConditionIsBool(Node* node);

#endif //COMPILATION_HW3_TYPES_H
