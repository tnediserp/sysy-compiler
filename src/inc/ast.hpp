#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <inc/koopa_ir.hpp>
using namespace std;

// 所有 AST 的基类
class BaseAST
{
public:
    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    virtual unique_ptr<BaseIR> AST2IR() const = 0;
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

    unique_ptr<BaseIR> AST2IR() const override
    {
        auto ir = new ProgramIR(0, 1);
        ir->func_list[0] = func_def->AST2IR();
        return unique_ptr<ProgramIR>(ir);
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

    unique_ptr<BaseIR> AST2IR() const override
    {
        auto ir = new FuncIR(1, ident);
        ir->block_list[0] = block->AST2IR();
        return unique_ptr<FuncIR>(ir);
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

    unique_ptr<BaseIR> AST2IR() const override
    {
        return unique_ptr<BaseIR>();
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

    unique_ptr<BaseIR> AST2IR() const override
    {
        auto ir = new BlockIR(1, "entry");
        ir->instr_list[0] = stmt->AST2IR();
        return unique_ptr<BlockIR>(ir);
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

    unique_ptr<BaseIR> AST2IR() const override
    {
        auto ir = new ValueIR(ValueKind::RET, number);
        return unique_ptr<ValueIR>(ir);
    }
};
