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
    buffer.emit("br i1 " + isZero + ", label %" + trueLabel + ", label %" + nextLabel);
    buffer.emit(trueLabel + ":");
    string errorVar = buffer.freshStr();
    buffer.emitGlobal(errorVar + R"( = private unnamed_addr constant [23 x i8] c"Error division by zero\00", align 1)");
    string msgPtr = buffer.freshVar();
    buffer.emit(msgPtr + " = getelementptr [23 x i8], [23 x i8]* " + errorVar + ", i32 0, i32 0");
    buffer.emit("call void @print(i8* " + msgPtr + ")");
    buffer.emit("call void @exit(i32 1)");
    buffer.emit("br label %" + nextLabel);
    buffer.emit(nextLabel + ":");
}

void Exp::processArithmetic(Exp *exp1, Node *op, Exp *exp2)
{
    string operation = op->m_text;
    bool areNums = exp1->checkNumber() && exp2->checkNumber();
    bool oneIsInt = exp1->checkInt() || exp2->checkInt();
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
        string tmpVar2 = buffer.freshVar();
        m_var = buffer.freshVar();
        buffer.emit(tmpVar2 + " = trunc i32 " + tmpVar + " to i8");
        buffer.emit(m_var + " = zext i8 " + tmpVar2 + " to i32");
    }
}

void Exp::processAndOr(Exp *exp1, Node *op, Exp *exp2)
{
    string operation = op->m_text;
    bool areBools = exp1->checkBool() && exp2->checkBool();
    if(!areBools)
    {
        errorMismatch(yylineno);
        exit(1);
    }
    m_type = "BOOL";
    m_trueLabel = exp2->m_trueLabel;
    m_falseLabel = exp2->m_falseLabel;
    m_nextLabel = exp2->m_nextLabel;
    if(operation == "or") {
        buffer.emit(exp1->m_trueLabel + ":");
        buffer.emit("br label %" + m_trueLabel);
    } else if(operation == "and") {
        buffer.emit(exp1->m_falseLabel + ":");
        buffer.emit("br label %" + m_falseLabel);
    }
}

void Exp::processRelop(Exp *exp1, Node *op, Exp *exp2)
{
    string operation = op->m_text;
    bool areNums = exp1->checkNumber() && exp2->checkNumber();
    if(!areNums)
    {
        errorMismatch(yylineno);
        exit(1);
    }
    m_type = "BOOL";
    m_trueLabel = buffer.freshLabel();
    m_falseLabel = buffer.freshLabel();
    m_nextLabel = buffer.freshLabel();
    string cmpCmd;
    if(operation == "<")
        cmpCmd = "slt";
    else if(operation == ">")
        cmpCmd = "sgt";
    else if(operation == "<=")
        cmpCmd = "sle";
    else if(operation == ">=")
        cmpCmd = "sge";
    else if(operation == "==")
        cmpCmd = "eq";
    else if(operation == "!=")
        cmpCmd = "ne";

    string tmpVar = buffer.freshVar();
    buffer.emit(tmpVar + " = icmp " + cmpCmd + " i32 " + exp1->m_var + ", " + exp2->m_var);
    buffer.emit("br i1 " + tmpVar + ", label %" + m_trueLabel + ", label %" + m_falseLabel);
}


Exp::Exp(Exp *exp1, Node *op, Exp *exp2) : Node()
{
    m_text = "";
    string operation = op->m_text;
    bool areNums = exp1->checkNumber() && exp2->checkNumber();
    if(operation == "+" || operation == "-" || operation == "*" || operation == "/")
    {
        processArithmetic(exp1, op, exp2);
    } else if(operation == "and" || operation == "or") {
        processAndOr(exp1, op, exp2);
    } else if(operation == "<" || operation == ">" || operation == "<=" || operation == ">=" || operation == "==" || operation == "!=") {
        processRelop(exp1, op, exp2);
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
    m_var = buffer.freshVar();
    string tmpVar = buffer.freshVar();
    buffer.emit(tmpVar + " = getelementptr i32, i32* %varStack, i32 " + to_string(entry->offset));
    buffer.emit(m_var + " = load i32, i32* " + tmpVar);

    if(m_type == "BOOL")
    {
        m_trueLabel = buffer.freshLabel();
        m_falseLabel = buffer.freshLabel();
        m_nextLabel = buffer.freshLabel();
        string isOne = buffer.freshVar();
        buffer.emit(isOne + " = icmp eq i32 " + m_var + ", 1");
        buffer.emit("br i1 " + isOne + ", label %" + m_trueLabel + ", label %" + m_falseLabel);
        m_var = "";
    }
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
        m_var = buffer.freshVar();
        buffer.emit(m_var + " = add i32 0, " + node->m_text);
    } else if(op == "NOT") {
        m_type = "BOOL";
        bool isBool = node->checkBool();
        if(!isBool)
        {
            errorMismatch(yylineno);
            exit(1);
        }
        m_trueLabel = buffer.freshLabel();
        m_falseLabel = buffer.freshLabel();
        m_nextLabel = buffer.freshLabel();
        buffer.emit(node->m_trueLabel + ":");
        buffer.emit("br label %" + m_falseLabel);
        buffer.emit(node->m_falseLabel + ":");
        buffer.emit("br label %" + m_trueLabel);

    } else if(op == "TRUE" || op == "FALSE") {
        m_type = "BOOL";
        m_trueLabel = buffer.freshLabel();
        m_falseLabel = buffer.freshLabel();
        m_nextLabel = buffer.freshLabel();
        if(op == "TRUE")
            buffer.emit("br label %" + m_trueLabel);
        else if(op == "FALSE")
            buffer.emit("br label %" + m_falseLabel);
    } else if(op == "STRING") {
        m_type = "STRING";
        m_var = buffer.freshVar();
        string strVar = buffer.freshStr();
        string txt = node->m_text.substr(1, node->m_text.size() - 2);
        buffer.emitGlobal(strVar + " = private unnamed_addr constant [" + to_string(txt.size() + 1)
                         +  R"( x i8] c")" + txt + R"(\00", align 1)");
        string tmp = "[" + to_string(txt.size() + 1) + " x i8]";
        buffer.emit(m_var + " = getelementptr " + tmp + ", " + tmp + "* " + strVar + ", i32 0, i32 0");
    } else if(op == "SAME") {
        m_type = node->m_type;
        m_trueLabel = node->m_trueLabel;
        m_falseLabel = node->m_falseLabel;
        m_nextLabel = node->m_nextLabel;
        m_var = node->m_var;
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
        string tmpVar2 = buffer.freshVar();
        m_var = buffer.freshVar();
        buffer.emit(tmpVar2 + " = trunc i32 " + tmpVar + " to i8");
        buffer.emit(m_var + " = zext i8 " + tmpVar2 + " to i32");
    }
}

Exp::Exp(Call *call) : Node()
{
    m_text = "";
    m_type = call->m_type;
    m_var = call->m_var;
    m_trueLabel = call->m_trueLabel;
    m_falseLabel = call->m_falseLabel;
}

Node::Node(const string& type) : m_text(string(yytext)), m_type(type), m_var(), m_trueLabel(),
                                 m_falseLabel(), m_nextLabel() {}

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

void assignBool(Exp* exp, ScopeTableEntry* entry)
{
    string tmpVar = buffer.freshVar();
    buffer.emit(exp->m_trueLabel + ":");
    buffer.emit(tmpVar + " = getelementptr i32, i32* %varStack, i32 " + to_string(entry->offset));
    buffer.emit("store i32 1, i32* " + tmpVar);
    buffer.emit("br label %" + exp->m_nextLabel);
    tmpVar = buffer.freshVar();
    buffer.emit(exp->m_falseLabel + ":");
    buffer.emit(tmpVar + " = getelementptr i32, i32* %varStack, i32 " + to_string(entry->offset));
    buffer.emit("store i32 0, i32* " + tmpVar);
    buffer.emit("br label %" + exp->m_nextLabel);
    buffer.emit(exp->m_nextLabel + ":");
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
    ScopeTableEntry* entry = scopesTable->getVar(symbol);

    if(m_type == "BOOL" && exp != nullptr)
    {
        assignBool(exp, entry);
        return;
    }

    string tmpVar = buffer.freshVar();
    buffer.emit(tmpVar + " = getelementptr i32, i32* %varStack, i32 " + to_string(entry->offset));
    if(exp != nullptr)
        buffer.emit("store i32 " + exp->m_var + ", i32* " + tmpVar);
    else
        buffer.emit("store i32 0, i32* " + tmpVar);
}

Statement::Statement() : Node()
{
    buffer.emit("ret i32 0");
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

    if(type == "BOOL")
    {
        assignBool(exp, entry);
        return;
    }

    string tmpVar = buffer.freshVar();
    buffer.emit(tmpVar + " = getelementptr i32, i32* %varStack, i32 " + to_string(entry->offset));
    buffer.emit("store i32 " + exp->m_var + ", i32* " + tmpVar);

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
    if(node->m_text == "break")
        buffer.emit("br label %" + ScopesTable::getInstance()->getLastWhileNextLabel());
    else if(node->m_text == "continue")
        buffer.emit("br label %" + ScopesTable::getInstance()->getLastWhileBeforeLabel());
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

void Call::callFunction(Node *node, Exp *exp, ScopeTableFunctionEntry* entry)
{
    if(node->m_text == "readi") {
        m_var = buffer.freshVar();
        buffer.emit(m_var + " = call i32 @readi(i32 " + exp->m_var + ")");
    } else if(node->m_text == "printi") {
        buffer.emit("call void @printi(i32 " + exp->m_var + ")");
    } else if(node->m_text == "print") {
        buffer.emit("call void @print(i8* " + exp->m_var + ")");
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
    callFunction(node, exp, entry);
    m_text = "";
    m_type = entry->type;
}

Program::Program() : Node()
{
    //ScopesTable::getInstance()->removeScope();
    buffer.emit("    ret i32 0");
    buffer.emit("}");
}

void CheckConditionIsBool(Node* node)
{
    if(node->m_type != "BOOL")
    {
        errorMismatch(yylineno);
        exit(1);
    }
}

void initProgram()
{
    buffer.emitGlobal("declare i32 @scanf(i8*, ...)");
    buffer.emitGlobal("declare i32 @printf(i8*, ...)");
    buffer.emitGlobal("declare void @exit(i32)");
    buffer.emitGlobal(R"(@.int_specifier_scan = constant [3 x i8] c"%d\00")");
    buffer.emitGlobal(R"(@.int_specifier = constant [4 x i8] c"%d\0A\00")");
    buffer.emitGlobal(R"(@.str_specifier = constant [4 x i8] c"%s\0A\00")");

    buffer.emitGlobal("define i32 @readi(i32) {");
    buffer.emitGlobal("    %ret_val = alloca i32");
    buffer.emitGlobal("    %spec_ptr = getelementptr [3 x i8], [3 x i8]* @.int_specifier_scan, i32 0, i32 0");
    buffer.emitGlobal("    call i32 (i8*, ...) @scanf(i8* %spec_ptr, i32* %ret_val)");
    buffer.emitGlobal("    %val = load i32, i32* %ret_val");
    buffer.emitGlobal("    ret i32 %val");
    buffer.emitGlobal("}");

    buffer.emitGlobal("define void @printi(i32) {");
    buffer.emitGlobal("    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0");
    buffer.emitGlobal("    call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)");
    buffer.emitGlobal("    ret void");
    buffer.emitGlobal("}");

    buffer.emitGlobal("define void @print(i8*) {");
    buffer.emitGlobal("    %spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0");
    buffer.emitGlobal("    call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)");
    buffer.emitGlobal("    ret void");
    buffer.emitGlobal("}");

    buffer.emit("define i32 @main(){");
    buffer.emit("%varStack = alloca i32, i32 50");
}