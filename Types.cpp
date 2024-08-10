//
// Created by Muham on 7/14/2024.
//

#include "Types.h"
#include <iostream>

extern int yylineno;
extern char* yytext;

const int MAX_BYTE_VALUE = 255;

Exp::Exp(Exp *exp1, Node *op, Exp *exp2) : Node()
{
    m_text = "";
    string operation = op->m_text;
    bool areNums = exp1->checkNumber() && exp2->checkNumber();
    bool oneIsInt = exp1->checkInt() || exp2->checkInt();
    bool areBools = exp1->checkBool() && exp2->checkBool();
    if(operation == "+" || operation == "-" || operation == "*" || operation == "/")
    {
        if(!areNums)
        {
            errorMismatch(yylineno);
            exit(1);
        }
        m_type = oneIsInt ? "INT" : "BYTE";
    } else if(operation == "and" || operation == "or") {
        if(!areBools)
        {
            errorMismatch(yylineno);
            exit(1);
        }
        m_type = "BOOL";
    } else if(operation == "<" || operation == ">" || operation == "<=" || operation == ">=" || operation == "==" || operation == "!=") {
        if(!areNums)
        {
            errorMismatch(yylineno);
            exit(1);
        }
        m_type = "BOOL";
    }
}

Exp::Exp(Node* node) : Node()
{
    ScopeTableEntry *entry = ScopesTable::getInstance()->getVar(node->m_text);
    if(entry == nullptr)
    {
        errorUndef(yylineno, node->m_text);
        exit(1);
    }
    m_text = "";
    m_type = entry->type;
}

Exp::Exp(Node *node, const string& op) : Node()
{
    m_text = "";
    if(op == "INT") {
        m_type = "INT";
    } else if(op == "BYTE") {
        m_type = "BYTE";
        int value;
        bool outOfByteRange = false;
        try {
            value = stoi(node->m_text);
            if (value > MAX_BYTE_VALUE)
                outOfByteRange = true;
        } catch (std::out_of_range& e) {
            outOfByteRange = true;
        }
        if(outOfByteRange)
        {
            errorByteTooLarge(yylineno, node->m_text);
            exit(1);
        }
    } else if(op == "NOT") {
        m_type = "BOOL";
        bool isBool = node->checkBool();
        if(!isBool)
        {
            errorMismatch(yylineno);
            exit(1);
        }
    } else if(op == "TRUE" || op == "FALSE") {
        m_type = "BOOL";
    } else if(op == "STRING") {
        m_type = "STRING";
    } else if(op == "SAME") {
        m_type = node->m_type;
    }
}

Exp::Exp(Type *type, Exp *exp) : Node()
{
    bool areNums = type->checkNumber() && exp->checkNumber();
    if(!areNums)
    {
        errorMismatch(yylineno);
        exit(1);
    }

    m_type = type->m_type;
}

Exp::Exp(Call *call) : Node()
{
    m_text = "";
    m_type = call->m_type;
}

Node::Node(const string& type) : m_text(string(yytext)), m_type(type) {}

bool Node::checkNumber() const
{
    if(m_type == "INT" || m_type == "BYTE")
        return true;
    return false;
}
bool Node::checkBool() const
{
    if(m_type == "BOOL")
        return true;
    return false;
}

bool Node::checkInt() const
{
    if(m_type == "INT")
        return true;
    return false;
}

Type::Type(const string& type) : Node()
{
    m_type = type;
}

Statement::Statement(Type *type, Node *node, Exp* exp) : Node()
{
    m_text = "";
    m_type = type->m_type;
    ScopesTable* scopesTable = ScopesTable::getInstance();
    string symbol = node->m_text;
    if(scopesTable->symbolExists(symbol)) {
        errorDef(yylineno, symbol); exit(1);
    }
    if(exp != nullptr && exp->m_type != type->m_type && (type->m_type != "INT" || exp->m_type != "BYTE")) {
        errorMismatch(yylineno);exit(1);
    }
    scopesTable->addVarToLastScope(symbol, m_type);
}

Statement::Statement(Node *node, Exp *exp) : Node()
{
    ScopeTableEntry* entry = ScopesTable::getInstance()->getVar(node->m_text);
    if(entry == nullptr) {
        errorUndef(yylineno, node->m_text); exit(1);
    }
    string type = entry->type;
    if(type != exp->m_type && (type != "INT" || exp->m_type != "BYTE")) {
        errorMismatch(yylineno); exit(1);
    }
}

Statement::Statement(Node *node) : Node()
{
    m_type = "";
    m_text = "";
    if(!ScopesTable::getInstance()->hasWhileScope())
    {
        if(node->m_text == "break")
            errorUnexpectedBreak(yylineno);
        else if(node->m_text == "continue")
            errorUnexpectedContinue(yylineno);
        exit(1);
    }
}

Statement::Statement(Statements *statements) : Node() {}

Statement::Statement(Call *call) : Node()
{
    m_text = "";
    m_type = call->m_type;
}

Statement::Statement(Exp *exp, Statement *statement1, Statement *statement2) : Node()
{
    m_text = "";
    m_type = "";
    if(exp->m_type != "BOOL")
    {
        errorMismatch(yylineno);
        exit(1);
    }
}

Call::Call(Node *node, Exp *exp) : Node()
{
    ScopeTableFunctionEntry* entry = ScopesTable::getInstance()->getFunction(node->m_text);
    if(entry == nullptr)
    {
        errorUndefFunc(yylineno, node->m_text);
        exit(1);
    }
    if(entry->paramType != exp->m_type)
    {
        bool flag1 = entry->paramType == "INT";
        bool flag2 = exp->checkNumber();
        if(!flag1 || !flag2)
        {
            errorPrototypeMismatch(yylineno, node->m_text, entry->paramType);
            exit(1);
        }
    }
    m_text = "";
    m_type = entry->type;
}

Program::Program() : Node()
{
    //ScopesTable::getInstance()->removeScope();
}

void CheckConditionIsBool(Node* node)
{
    if(node->m_type != "BOOL")
    {
        errorMismatch(yylineno);
        exit(1);
    }
}
