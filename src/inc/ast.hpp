#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <inc/koopa_ir.hpp>
#include <cassert>
using namespace std;

// 寄存器类
class Register
{
public:
    int num; // 寄存器编号
    int value; // 寄存器内容
    bool real; // 寄存器是否真实存在？比如对于一个number,我们使用虚空寄存器来存它，此时该虚空寄存器不会占用一个寄存器编号

    // 寄存器深拷贝
    Register& operator = (const Register &reg)
    {
        num = reg.num;
        value = reg.value;
        real = reg.real; 
        return *this;
    }

    /**
     * 输出寄存器
     * 
     * 如果该寄存器真实存在，那么输出编号
     * 如果该寄存器是虚空寄存器，那么输出内容
     */
    friend ostream& operator << (ostream &os, const Register &reg)
    {
        if (reg.real)
            os << "%" << reg.num;
        else os << reg.value;
        return os;
    }

    // 寄存器编号更新
    Register operator + (const int x)
    {
        Register reg;
        reg = *this;
        reg.num = num + x;
        if (x > 0)
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
    virtual void DumpIR(ostream &os) const = 0; // 根据AST输出文本形式IR
    virtual void DistriReg(int lb){} // 分配寄存器，编号以lb为下界
    virtual int Value(){} // 表达式求值
};


// CompUnit 是 BaseAST
class CompUnitAST : public BaseAST
{
public:
    // 用智能指针管理对象
    unique_ptr<BaseAST> func_def;

    void DistriReg(int lb) override
    {
        func_def->DistriReg(lb);
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

    void DistriReg(int lb) override 
    {
        block->DistriReg(lb);
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

    void DumpIR(ostream &os) const override {}
};

class BlockAST : public BaseAST
{
public:
    unique_ptr<BaseAST> stmt;

    void DistriReg(int lb) override
    {
        stmt->DistriReg(lb);
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

    void DistriReg(int lb) override 
    {
        exp->DistriReg(lb);
    }  

    void DumpIR(ostream &os) const override
    {
        os << "ret " << exp->Value() << endl;
        // exp->DumpIR(os);
        // os << "ret " << exp->reg << endl;
    }
};

// Exp ::= LOrExp
class ExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> loexp;

    void DistriReg(int lb) override
    {
        loexp->DistriReg(lb);
        reg = loexp->reg;
    }
    void DumpIR(ostream &os) const override
    {
        loexp->DumpIR(os);
    }

    int Value() override
    {
        return loexp->Value();
    }
};

// PrimaryExp ::= "(" Exp ")"
class PExp2ExpAST : public BaseAST
{
public:
    unique_ptr<BaseAST> exp;
    
    void DistriReg(int lb) override
    {
        exp->DistriReg(lb);
        reg = exp->reg;
    }
    void DumpIR(ostream &os) const override
    {
        exp->DumpIR(os);
    }

    int Value() override
    {
        return exp->Value();
    }
};

// PrimaryExp ::= Number
class PExp2NumAST: public BaseAST
{
public:
    int num;

    void DistriReg(int lb) override
    {
        reg.value = num;
        reg.real = false;
        reg.num = lb;
    }
    void DumpIR(ostream &os) const override
    {

    }
    int Value() override 
    {
        return num;
    }
};

// UnaryExp ::= PrimaryExp
class UExp2PExpAST: public BaseAST
{
public:
    unique_ptr<BaseAST> pexp;

    void DistriReg(int lb) override
    {
        pexp->DistriReg(lb);
        reg = pexp->reg;
    }
    void DumpIR(ostream &os) const override
    {
        pexp->DumpIR(os);
    }
    int Value() override
    {
        return pexp->Value();
    }
};

// UnaryExp ::= UnaryOp UnaryExp
class UExp2UOp_UExpAST: public BaseAST
{
public: 
    string uop;
    unique_ptr<BaseAST> uexp;

    void DistriReg(int lb) override
    {
        uexp->DistriReg(lb);

        assert(uop == "+" || uop == "-" || uop == "!");

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

    int Value() override
    {
        assert(uop == "+" || uop == "-" || uop == "!");
        if (uop == "+")
            return uexp->Value();
        else if (uop == "-")
            return -uexp->Value();
        else return (uexp->Value() == 0);
    }
};

// MulExp ::= UnaryExp
class MExp2UExp_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> uexp;
    
    void DistriReg(int lb) override 
    {
        uexp->DistriReg(lb);
        reg = uexp->reg;
    }

    void DumpIR(ostream &os) const override
    {
        uexp->DumpIR(os);
    }

    int Value() override
    {
        return uexp->Value();
    }
};

// MulExp ::= MulExp ("*" | "/" | "%") UnaryExp
class MExp2MExp_UExp_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> mexp;
    unique_ptr<BaseAST> uexp;
    string op;
    
    void DistriReg(int lb) override 
    {
        mexp->DistriReg(lb);
        uexp->DistriReg(mexp->reg.num);
        
        reg = uexp->reg + 1;
    }

    void DumpIR(ostream &os) const override
    {   
        mexp->DumpIR(os);
        uexp->DumpIR(os);

        assert(op == "*" || op == "/" || op == "%");
        
        if (op == "*")
            os << reg << " = mul " << mexp->reg << ", " << uexp->reg << endl;
        
        else if (op == "/")
            os << reg << " = div " << mexp->reg << ", " << uexp->reg << endl;

        else // if (op == "%")
            os << reg << " = mod " << mexp->reg << ", " << uexp->reg << endl;
    }

    int Value() override 
    {
        assert(op == "*" || op == "/" || op == "%");
        
        if (op == "*")
            return mexp->Value() * uexp->Value();
        
        else if (op == "/")
            return mexp->Value() / uexp->Value();

        else // if (op == "%")
            return mexp->Value() % uexp->Value();
    }
};

// AddExp ::= MulExp
class AExp2MulExp_AST: public BaseAST
{
public: 
    unique_ptr<BaseAST> mexp;
    
    void DistriReg(int lb) override 
    {
        mexp->DistriReg(lb);
        reg = mexp->reg;
    }

    void DumpIR(ostream &os) const override
    {
        mexp->DumpIR(os);
    }

    int Value() override
    {
        return mexp->Value();
    }
};

// AddExp ::= AddExp ("+" | "-") MulExp
class AExp2AExp_MExp_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> aexp;
    unique_ptr<BaseAST> mexp;
    string op;
    
    void DistriReg(int lb) override 
    {
        aexp->DistriReg(lb);
        mexp->DistriReg(aexp->reg.num);
        reg = mexp->reg + 1;
    }

    void DumpIR(ostream &os) const override
    {
        aexp->DumpIR(os);
        mexp->DumpIR(os);

        assert(op == "+" || op == "-");
        
        if (op == "+")
            os << reg << " = add " << aexp->reg << ", " << mexp->reg << endl;
        
        else // if (op == "-")
            os << reg << " = sub " << aexp->reg << ", " << mexp->reg << endl;
    }

    int Value() override
    {
        assert(op == "+" || op == "-");
        
        if (op == "+")
            return aexp->Value() + mexp->Value();
        
        else // if (op == "-")
            return aexp->Value() - mexp->Value();
    }
};

// RelExp ::= AddExp
class RExp2AExp_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> aexp;
    
    void DistriReg(int lb) override 
    {
        aexp->DistriReg(lb);
        reg = aexp->reg;
    }

    void DumpIR(ostream &os) const override
    {
        aexp->DumpIR(os);
    }

    int Value() override 
    {
        return aexp->Value();
    }
};

// RelExp ::= RelExp ("<" | ">" | "<=" | ">=") AddExp
class RExp2R_rel_A_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> rexp;
    unique_ptr<BaseAST> aexp;
    string rel;

    void DistriReg(int lb) override 
    {
        rexp->DistriReg(lb);
        aexp->DistriReg(rexp->reg.num);
        reg = aexp->reg + 1;
    }

    void DumpIR(ostream &os) const override
    {
        rexp->DumpIR(os);
        aexp->DumpIR(os);

        assert(rel == "<" || rel == ">" || rel == "<=" || rel == ">=");
        
        if (rel == "<")
            os << reg << " = lt " << rexp->reg << ", " << aexp->reg << endl;
        
        else if (rel == ">")
            os << reg << " = gt " << rexp->reg << ", " << aexp->reg << endl;

        else if (rel == "<=")
            os << reg << " = le " << rexp->reg << ", " << aexp->reg << endl;
        
        else // if (rel == ">=")
            os << reg << " = ge " << rexp->reg << ", " << aexp->reg << endl;
    }

    int Value() override
    {
        assert(rel == "<" || rel == ">" || rel == "<=" || rel == ">=");
        
        if (rel == "<")
            return (rexp->Value() < aexp->Value());
        
        else if (rel == ">")
            return (rexp->Value() > aexp->Value());

        else if (rel == "<=")
            return (rexp->Value() <= aexp->Value());
        
        else // if (rel == ">=")
            return (rexp->Value() >= aexp->Value());
    }
};

// EqExp ::= RelExp
class EExp2RExp_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> rexp;
    
    void DistriReg(int lb) override 
    {
        rexp->DistriReg(lb);
        reg = rexp->reg;
    }

    void DumpIR(ostream &os) const override
    {
        rexp->DumpIR(os);
    }

    int Value() override
    {
        return rexp->Value();
    }
};

// EqExp ::= EqExp ("==" | "!=") RelExp
class EExp2E_eq_R_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> eexp;
    unique_ptr<BaseAST> rexp;
    string rel;
    Register imm_reg; // 用于计算中间结果的寄存器

    void DistriReg(int lb) override 
    {
        eexp->DistriReg(lb);
        rexp->DistriReg(eexp->reg.num);

        assert(rel == "==" || rel == "!=");

        // 需要使用imm_reg保存中间结果
        if (rel == "!=")
        {
            imm_reg = rexp->reg + 1;
            reg = rexp->reg + 2;
        }
        else 
        {
            imm_reg = rexp->reg + 1;
            reg = rexp->reg + 1;
        }
    }

    void DumpIR(ostream &os) const override
    {
        eexp->DumpIR(os);
        rexp->DumpIR(os);

        assert(rel == "==" || rel == "!=");
        
        if (rel == "==")
            os << reg << " = eq " << eexp->reg << ", " << rexp->reg << endl;
        
        else // (rel == "!=")
        {
            // 先判断两者相等，再取非
            os << imm_reg << " = eq "<< eexp->reg << ", " << rexp->reg << endl;
            os << reg << " = eq " << imm_reg << ", 0" << endl;
        }
    }

    int Value() override 
    {
        assert(rel == "==" || rel == "!=");
        
        if (rel == "==")
            return eexp->Value() == rexp->Value();
        
        else // (rel == "!=")
            return eexp->Value() != rexp->Value();
    }
};

// LAndExp ::= EqExp
class LAExp2EExp_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> eexp;
    
    void DistriReg(int lb) override 
    {
        eexp->DistriReg(lb);
        reg = eexp->reg;
    }

    void DumpIR(ostream &os) const override
    {
        eexp->DumpIR(os);
    }

    int Value() override
    {
        return eexp->Value();
    }
};

// LAndExp ::= LAndExp "&&" EqExp
class LAExp2L_and_E_AST: public BaseAST
{
public: 
    unique_ptr<BaseAST> laexp;
    unique_ptr<BaseAST> eexp;
    Register imm_reg1, imm_reg2, imm_reg3; // 用于计算中间结果的临时寄存器

    void DistriReg(int lb) override 
    {
        laexp->DistriReg(lb);
        eexp->DistriReg(laexp->reg.num);

        // 需要使用imm_reg保存中间结果
        imm_reg1 = eexp->reg + 1;
        imm_reg2 = eexp->reg + 2;
        imm_reg3 = eexp->reg + 3;
        reg = eexp->reg + 4;
    }

    void DumpIR(ostream &os) const override
    {
        laexp->DumpIR(os);
        eexp->DumpIR(os);

        // laexp == 0？
        os << imm_reg1 << " = eq " << laexp->reg << ", 0" <<endl; 
        // eexp = 0?
        os << imm_reg2 << " = eq " << eexp->reg << ", 0" <<endl; 
        // laexp == 0 || eexp == 0?
        os << imm_reg3 << " = or " << imm_reg1 << ", " << imm_reg2 << endl;
        // 取反
        os << reg << " = eq " << imm_reg3 << ", 0" << endl;
    }

    int Value() override
    {
        return laexp->Value() && eexp->Value();
    }
};

// LOrExp ::= LAndExp
class LOExp2LAExp_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> laexp;
    
    void DistriReg(int lb) override 
    {
        laexp->DistriReg(lb);
        reg = laexp->reg;
    }

    void DumpIR(ostream &os) const override
    {
        laexp->DumpIR(os);
    }

    int Value() override
    {
        return laexp->Value();
    }
};

// LOrExp ::= LOrExp "||" LAndExp
class LOExp2L_or_LA_AST: public BaseAST 
{
public: 
    unique_ptr<BaseAST> loexp;
    unique_ptr<BaseAST> laexp;
    Register imm_reg1, imm_reg2;

    void DistriReg(int lb) override 
    {
        loexp->DistriReg(lb);
        laexp->DistriReg(loexp->reg.num);

        // 需要使用imm_reg保存中间结果
        imm_reg1 = laexp->reg + 1;
        imm_reg2 = laexp->reg + 2;
        reg = laexp->reg + 3;
    }

    void DumpIR(ostream &os) const override
    {
        loexp->DumpIR(os);
        laexp->DumpIR(os);

        // 按位或非零，则一定有一个为真
        os << imm_reg1 << " = or " << loexp->reg << ", " << laexp->reg << endl;
        os << imm_reg2 << " = eq " << imm_reg1 << ", 0" << endl;
        os << reg << " = eq " << imm_reg2 << ", 0" << endl;
    }

    int Value() override
    {
        return loexp->Value() || laexp->Value();
    }
};