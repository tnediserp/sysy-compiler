#pragma once
#include <memory>
#include <string>
#include <iostream>
using namespace std;

// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
};

// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    unique_ptr<BaseAST> func_def;

    void Dump() const override
    {
        cout << "FuncDefAST { ";
        func_def->Dump();
        cout << " }";
    }
};

// FuncDef 也是 BaseAST
class FuncDefAST : public BaseAST
{
public:
    unique_ptr<BaseAST> func_type;
    string ident;
    unique_ptr<BaseAST> block;

    void Dump() const override
    {
        cout << "FuncDefAST { ";
        func_type->Dump();
        cout << ", " << ident << ", ";
        block->Dump();
        cout << " }";
    }
};

class FuncTypeAST : public BaseAST
{
public:
    string type;

    void Dump() const override
    {
        cout << "FuncTypeAST { ";
        cout << "int";
        cout << " }";
    }
};

class BlockAST : public BaseAST
{
public:
    unique_ptr<BaseAST> stmt;

    void Dump() const override
    {
        cout << "BlockAST { ";
        stmt->Dump();
        cout << " }";
    }
};

class StmtAST : public BaseAST
{
public:
    // std::string return_;
    int number;

    void Dump() const override
    {
        cout << "StmtAST { " << number << " }";
    }
};
