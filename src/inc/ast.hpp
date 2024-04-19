#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <inc/koopa_ir.hpp>
using namespace std;

// 寄存器类
class Register
{
public:
    int num;
    int value;
    bool real; // 寄存器是否真实存在？比如对于一个number,我们使用虚空寄存器来存它。

    Register& operator = (const Register &reg)
    {
        num = reg.num;
        value = reg.value;
        real = reg.real; 
        return *this;
    }

    friend ostream& operator << (ostream &os, const Register &reg)
    {
        if (reg.real)
            os << "%" << reg.num;
        else os << reg.value;
        return os;
    }

    Register operator + (const int x)
    {
        Register reg;
        reg = *this;
        reg.num = num + x;
        reg.real = true;
        return reg;
    }
};

// 所有 AST 的基类
class BaseAST
{
public:
    Register reg;

    virtual ~BaseAST() = default;
    virtual void Dump() const = 0;
    // virtual unique_ptr<BaseIR> AST2IR() const = 0;
    virtual void DumpIR(ostream &os) const = 0; // 根据AST输出文本形式IR
    virtual void DistriReg(){}
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

    /*
    unique_ptr<BaseIR> AST2IR() const override
    {
        auto ir = new ProgramIR(0, 1);
        ir->func_list[0] = func_def->AST2IR();
        return unique_ptr<ProgramIR>(ir);
    }
    */
    void DistriReg() override
    {
        func_def->DistriReg();
    }

    void DumpIR(ostream &os) const override
    {
        func_def->DumpIR(os);
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

    /*
    unique_ptr<BaseIR> AST2IR() const override
    {
        auto ir = new FuncIR(1, ident);
        ir->block_list[0] = block->AST2IR();
        return unique_ptr<FuncIR>(ir);
    }
    */

    void DistriReg() override 
    {
        block->DistriReg();
    }

    void DumpIR(ostream &os) const override
    {
        os << "fun @" << ident << "(): i32 {" << endl;
        block->DumpIR(os);
        os << "}";
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

    /*
    unique_ptr<BaseIR> AST2IR() const override
    {
        return unique_ptr<BaseIR>();
    }
    */

    void DumpIR(ostream &os) const override {}
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

    /*
    unique_ptr<BaseIR> AST2IR() const override
    {
        auto ir = new BlockIR(1, "entry");
        ir->instr_list[0] = stmt->AST2IR();
        return unique_ptr<BlockIR>(ir);
    }
    */

    void DistriReg() override
    {
        stmt->DistriReg();
    }

    void DumpIR(ostream &os) const override
    {
        os << "%" << "entry" << ":" << endl;
        stmt->DumpIR(os);
    }
};

class StmtAST : public BaseAST
{
public:
    // std::string return_;
    // int number;
    unique_ptr<BaseAST> exp;

    void Dump() const override
    {
        
    }

    /*
    unique_ptr<BaseIR> AST2IR() const override
    {
    
        auto ir = new ValueIR(ValueKind::RET, number);
        return unique_ptr<ValueIR>(ir);
    }
    */

    void DistriReg() override 
    {
        exp->DistriReg();
    }  

    void DumpIR(ostream &os) const override
    {
        exp->DumpIR(os);
        os << "ret " << exp->reg << endl;
    }
};

// Exp ::= UnitaryExp
class ExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> uexp;

    void Dump() const override {}
    // unique_ptr<BaseIR> AST2IR() const override {return NULL;}
    void DistriReg() override
    {
        uexp->DistriReg();
        reg = uexp->reg;
    }
    void DumpIR(ostream &os) const override
    {
        uexp->DumpIR(os);
    }
};

// PrimaryExp ::= "(" Exp ")"
class PExp2ExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> exp;
    
    void Dump() const override {}
    // unique_ptr<BaseIR> AST2IR() const override {return NULL;}
    void DistriReg() override
    {
        exp->DistriReg();
        reg = exp->reg;
    }
    void DumpIR(ostream &os) const override
    {
        exp->DumpIR(os);
    }
};

// PrimaryExp ::= Number
class PExp2NumAST: public BaseAST
{
public:
    int num;

    void Dump() const override {}
    void DistriReg() override
    {
        reg.value = num;
        reg.real = false;
        reg.num = 0;
    }
    void DumpIR(ostream &os) const override
    {

    }
};

// UnaryExp ::= PrimaryExp
class UExp2PExpAST: public BaseAST
{
public:
    unique_ptr<BaseAST> pexp;

    void Dump() const override {}
    // unique_ptr<BaseIR> AST2IR() const override {return NULL;}
    void DistriReg() override
    {
        pexp->DistriReg();
        reg = pexp->reg;
    }
    void DumpIR(ostream &os) const override
    {
        pexp->DumpIR(os);
    }
};

// UnaryExp ::= UnaryOp UnaryExp
class UExp2UOp_UExpAST: public BaseAST
{
public: 
    string uop;
    unique_ptr<BaseAST> uexp;

    void Dump() const override {}
    void DistriReg() override
    {
        uexp->DistriReg();

        if (uop != "+")
            reg = uexp->reg + 1;
        else reg = uexp->reg;
    }
    void DumpIR(ostream &os) const override
    {
        uexp->DumpIR(os);
        if (uop == "-")
            os << reg << " = sub 0, " << uexp->reg << endl;

        else if (uop == "!")
            os << reg << " = eq 0, " << uexp->reg << endl;

        else return;
    }
};
