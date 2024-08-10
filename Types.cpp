//
// Created by Muham on 7/14/2024.
//

#include "Types.h"
#include <iostream>
#include "cg.hpp"

extern int yylineno;
extern char* yytext;

const int MAX_BYTE_VALUE = 255;
CodeBuffer &buffer = CodeBuffer::instance();

void emitCheckZero(const string& var)
{
    string isZero = buffer.freshVar();
    buffer.emit(isZero + " = icmp eq i32 " + var + ", 0");
    string trueLabel = buffer.freshLabel();
    string nextLabel = buffer.freshLabel();
    buffer.emit("br i1 " + isZero + ", label " + trueLabel + ", label " + nextLabel);
    buffer.emit(trueLabel + ":");
    buffer.emitGlobal(R"(@error_msg = private unnamed_addr constant [24 x i8] c"Error division by zero\00", align 1)");
    buffer.emit("%msg_ptr = getelementptr [24 x i8], [24 x i8]* @error_msg, i32 0, i32 0");
    buffer.emit("call void @print(i8* %msg_ptr)");
    buffer.emit("call void @exit(i32 1)");
    buffer.emit(nextLabel + ":");
}

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
        m_var = buffer.freshVar();
        if(operation == "+")
            buffer.emit(m_var + " = add i32 " + exp1->m_var + ", " + exp2->m_var);
        else if (operation == "-")
            buffer.emit(m_var + " = sub i32 " + exp1->m_var + ", " + exp2->m_var);
        else if (operation == "*")
            buffer.emit(m_var + " = mul i32 " + exp1->m_var + ", " + exp2->m_var);
        else if (operation == "/")
        {
            emitCheckZero(exp2->m_var);
            if(oneIsInt)
                buffer.emit(m_var + " = sdiv i32 " + exp1->m_var + ", " + exp2->m_var);
            else
                buffer.emit(m_var + " = udiv i32 " + exp1->m_var + ", " + exp2->m_var);
        }
        if(!oneIsInt)
        {
            string tmpVar = m_var;
            m_var = buffer.freshVar();
            buffer.emit(m_var + " = trunc i32 " + tmpVar + " to i8");
        }
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
        m_var = buffer.freshVar();
        buffer.emit(m_var + " = add i32 0, " + node->m_text);
    } else if(op == "BYTE") {
        m_type = "BYTE";
        string tmpVar = buffer.freshVar();
        buffer.emit(tmpVar + " = add i32 0, " + node->m_text);
        m_var = buffer.freshVar();
        buffer.emit(m_var + " = trunc i32 " + tmpVar + " to i8");
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
        m_trueLabel = buffer.freshLabel();
        m_falseLabel = buffer.freshLabel();
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
    m_var = buffer.freshVar();
    buffer.emit(m_var + " = add i32 0, " + exp->m_var);
    if(m_type == "BYTE")
    {
        string tmpVar = m_var;
        m_var = buffer.freshVar();
        buffer.emit(m_var + " = trunc i32 " + tmpVar + " to i8");
    }
}

Exp::Exp(Call *call) : Node()
{
    m_text = "";
    m_type = call->m_type;
}

Node::Node(const string& type) : m_text(string(yytext)), m_type(type), m_var(), m_trueLabel(), m_falseLabel() {}

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
