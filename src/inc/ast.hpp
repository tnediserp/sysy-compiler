#pragma once
#include <memory>
#include <string>
#include <iostream>
#include <inc/koopa_ir.hpp>
#include <cassert>
#include <vector>
#include <map>
#include <inc/ST.hpp>
using namespace std;

extern map<string, ST_item> sym_table;
extern bool eof;


// 寄存器类
class Register
{
public:
    int num; // 寄存器编号
    int value; // 寄存器内容
    bool is_var; // 寄存器是否用于存一个变量，如果用于“存”了一个常量，认为该寄存器是虚空的

    // 寄存器深拷贝
    /*
    Register& operator = (const Register &reg)
    {
        num = reg.num;
        value = reg.value;
        is_var = reg.is_var; 
        return *this;
    }
    */

    /**
     * 输出寄存器
     * 
     * 如果该寄存器真实存在，那么输出编号
     * 如果该寄存器是虚空寄存器，那么输出内容
     */
    friend ostream& operator << (ostream &os, const Register &reg)
    {
        if (reg.is_var)
            os << "%" << reg.num;
        else os << reg.value;
        return os;
    }

    // 寄存器编号更新
    /*
    Register operator + (const int x)
    {
        Register reg;
        reg = *this;
        reg.num = num + x;
        if (x > 0)
            reg.real = true;
        return reg;
    }
    */
};

// 所有 AST 的基类
class BaseAST
{
public:
    Register reg;

    virtual ~BaseAST() = default;
    virtual void DumpIR(ostream &os) const = 0; // 根据AST输出文本形式IR
    virtual void DistriReg(int lb){} // 分配寄存器，编号以lb为下界
    virtual int Value() const {return INT32_MAX;}  // 表达式求值
    virtual void Semantic() = 0; // 语义分析
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

    void Semantic() override 
    {
        func_def->Semantic();
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

    void Semantic() override 
    {
        block->Semantic();
    }
};

class FuncTypeAST : public BaseAST
{
public:
    string type;

    void DumpIR(ostream &os) const override {}
    void Semantic() override {}
};

// Block ::= "{" {BlockItem} "}";
class BlockAST : public BaseAST
{
public:
    vector<unique_ptr<BaseAST>> block_item;

    void DistriReg(int lb) override 
    {
        for (int i = 0; i < block_item.size(); i++)
        {
            block_item[i]->DistriReg(lb);
            lb = max(block_item[i]->reg.num, lb);
        }
        reg.num = lb + 1;
    }

    void DumpIR(ostream &os) const override
    {
        os << "%" << "entry" << ":" << endl;
        for (int i = 0; i < block_item.size() && !eof; i++)
            block_item[i]->DumpIR(os);
    }

    void Semantic() override
    {
        for (int i = 0; i < block_item.size(); i++)
            block_item[i]->Semantic();
    }
};

// BlockItem ::= Decl;
class BlockItem2Decl_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> decl;

    void DistriReg(int lb) override
    {
        decl->DistriReg(lb);
        reg.num = decl->reg.num;
    }

    void DumpIR(ostream &os) const override {
        decl->DumpIR(os);
    }
    void Semantic() override 
    {
        decl->Semantic();
    }
};

// BlockItem ::= Stmt;
class BlockItem2Stmt_AST: public BaseAST
{
public: 
    unique_ptr<BaseAST> stmt;

    void DistriReg(int lb) override
    {
        stmt->DistriReg(lb);
        reg.num = stmt->reg.num;
    }
    void DumpIR(ostream &os) const override {
        stmt->DumpIR(os);
    }
    void Semantic() override
    {
        stmt->Semantic();
    }
};

// Stmt ::= "return" Exp ";";
class Stmt2Ret_AST : public BaseAST
{
public:
    // std::string return_;
    // int number;
    unique_ptr<BaseAST> exp;

    void DistriReg(int lb) override 
    {
        exp->DistriReg(lb);
        reg.num = exp->reg.num;
    }  

    void DumpIR(ostream &os) const override
    {
        exp->DumpIR(os);
        os << "ret " << exp->reg << endl;
        eof = true; // 标志程序已经截止。
    }
    void Semantic() override 
    {
        exp->Semantic();
        reg.is_var = exp->reg.is_var;
        reg.value = exp->reg.value;
    }
};

// Stmt ::= LVal "=" Exp ";"
class Stmt2Assign_AST: public BaseAST
{
public: 
    unique_ptr<BaseAST> lval;
    string ident;
    unique_ptr<BaseAST> exp;

    void DistriReg(int lb) override 
    {
        lval->DistriReg(lb);
        exp->DistriReg(lval->reg.num);
        reg.num = exp->reg.num + 1;
    }

    void DumpIR(ostream &os) const override 
    {
        exp->DumpIR(os);

        // string ident = ((LVal_AST *) (lval.get()))->ident;
        os << "store " << exp->reg << ", @" << ident << endl;
    }

    void Semantic() override 
    {
        lval->Semantic();
        exp->Semantic();

        assert(lval->reg.is_var);
        reg.is_var = true;
        reg.value = exp->reg.value;

        // string ident = ((LVal_AST *) (lval.get()))->ident;
        sym_table[ident].is_var = true;
        sym_table[ident].value = reg.value;
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
        reg.num = loexp->reg.num;
    }
    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        loexp->DumpIR(os);
    }
    void Semantic() override
    {
        loexp->Semantic();
        reg.is_var = loexp->reg.is_var;
        reg.value = loexp->reg.value;
    }

    int Value() const override
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
        reg.num = exp->reg.num;
    }
    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        exp->DumpIR(os);
    }

    void Semantic() override 
    {
        exp->Semantic();
        reg.is_var = exp->reg.is_var;
        reg.value = exp->reg.value;
    }

    int Value() const override
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
        reg.num = lb;
    }
    void DumpIR(ostream &os) const override {}

    void Semantic() override 
    {
        reg.is_var = false;
        reg.value = num;
    }

    int Value() const override 
    {
        return num;
    }
};

// PrimaryExp ::= LVal 
class PExp2Lval_AST: public BaseAST 
{
public: 
    unique_ptr<BaseAST> lval;

    void DistriReg(int lb) override 
    {
        lval->DistriReg(lb);
        reg.num = lval->reg.num;
    }

    void DumpIR(ostream &os) const override 
    {
        if (!reg.is_var) return;
        lval->DumpIR(os);
    }
    void Semantic() override
    {
        lval->Semantic();
        reg.is_var = lval->reg.is_var;
        reg.value = lval->reg.value;
    }
    int Value() const override 
    {
        return lval->Value();
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
        reg.num = pexp->reg.num;
    }
    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        pexp->DumpIR(os);
    }
    void Semantic() override
    {
        pexp->Semantic();
        reg.is_var = pexp->reg.is_var;
        reg.value = pexp->reg.value;
    }
    int Value() const override
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
            reg.num = uexp->reg.num + 1;
        else reg.num = uexp->reg.num;
    }
    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        uexp->DumpIR(os);
        
        if (uop == "-")
            os << reg << " = sub 0, " << uexp->reg << endl;

        else if (uop == "!")
            os << reg << " = eq 0, " << uexp->reg << endl;

        else return;
    }

    void Semantic() override
    {
        uexp->Semantic();

        assert(uop == "+" || uop == "-" || uop == "!");
        reg.is_var = uexp->reg.is_var;
        if (uop == "+")
            reg.value = uexp->reg.value;
        else if (uop == "-")
            reg.value = -uexp->reg.value;
        else reg.value = (uexp->reg.value == 0);
    }
    int Value() const override
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
        reg.num = uexp->reg.num;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        uexp->DumpIR(os);
    }
    void Semantic() override 
    {
        uexp->Semantic();
        reg.is_var = uexp->reg.is_var;
        reg.value = uexp->reg.value;
    }

    int Value() const override
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
        
        reg.num = uexp->reg.num + 1;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
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

    void Semantic() override
    {
        mexp->Semantic();
        uexp->Semantic();

        reg.is_var = mexp->reg.is_var || uexp->reg.is_var;
        assert(op == "*" || op == "/" || op == "%");
        
        if (op == "*")
            reg.value = mexp->reg.value * uexp->reg.value;
        
        else if (op == "/")
            reg.value = mexp->reg.value / uexp->reg.value;

        else // if (op == "%")
            reg.value = mexp->reg.value % uexp->reg.value;
    }

    int Value() const override 
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
        reg.num = mexp->reg.num;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        mexp->DumpIR(os);
    }

    void Semantic() override
    {
        mexp->Semantic();
        reg.is_var = mexp->reg.is_var;
        reg.value = mexp->reg.value;
    }
    int Value() const override
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
        reg.num = mexp->reg.num + 1;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        aexp->DumpIR(os);
        mexp->DumpIR(os);

        assert(op == "+" || op == "-");
        
        if (op == "+")
            os << reg << " = add " << aexp->reg << ", " << mexp->reg << endl;
        
        else // if (op == "-")
            os << reg << " = sub " << aexp->reg << ", " << mexp->reg << endl;
    }

    void Semantic() override
    {
        aexp->Semantic();
        mexp->Semantic();

        reg.is_var = aexp->reg.is_var || mexp->reg.is_var;
        assert(op == "+" || op == "-");
        
        if (op == "+")
            reg.value = aexp->reg.value + mexp->reg.value;

        else // if (op == "-")
            reg.value = aexp->reg.value - mexp->reg.value;
    }

    int Value() const override
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
        reg.num = aexp->reg.num;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        aexp->DumpIR(os);
    }

    void Semantic() override
    {
        aexp->Semantic();
        reg.is_var = aexp->reg.is_var;
        reg.value = aexp->reg.value;
    }

    int Value() const override 
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
        reg.num = aexp->reg.num + 1;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
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

    void Semantic() override
    {
        rexp->Semantic();
        aexp->Semantic();

        reg.is_var = rexp->reg.is_var || rexp->reg.is_var;
        assert(rel == "<" || rel == ">" || rel == "<=" || rel == ">=");
        
        if (rel == "<")
            reg.value = (rexp->reg.value < aexp->reg.value);
        
        else if (rel == ">")
            reg.value = (rexp->reg.value > aexp->reg.value);

        else if (rel == "<=")
            reg.value = (rexp->reg.value <= aexp->reg.value);
        
        else // if (rel == ">=")
            reg.value = (rexp->reg.value >= aexp->reg.value);
    }

    int Value() const override
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
        reg.num = rexp->reg.num;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        rexp->DumpIR(os);
    }

    void Semantic() override
    {
        rexp->Semantic();
        reg.is_var = rexp->reg.is_var;
        reg.value = rexp->reg.value;
    }

    int Value() const override
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
            imm_reg.num = rexp->reg.num + 1; imm_reg.is_var = true;
            reg.num = rexp->reg.num + 2;
        }
        else 
        {
            imm_reg.num = rexp->reg.num + 1; imm_reg.is_var = true;
            reg.num = rexp->reg.num + 1;
        }
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
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

    void Semantic() override
    {
        eexp->Semantic();
        rexp->Semantic();

        reg.is_var = eexp->reg.is_var || rexp->reg.is_var;
        assert(rel == "==" || rel == "!=");
        
        if (rel == "==")
            reg.value = (eexp->reg.value == rexp->reg.value);
        
        else // (rel == "!=")
            reg.value = (eexp->reg.value != rexp->reg.value);
    }

    int Value() const override 
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
        reg.num = eexp->reg.num;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        eexp->DumpIR(os);
    }

    void Semantic() override
    {
        eexp->Semantic();
        reg.is_var = eexp->reg.is_var;
        reg.value = eexp->reg.value;
    }

    int Value() const override
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
        imm_reg1.num = eexp->reg.num + 1; imm_reg1.is_var = true;
        imm_reg2.num = eexp->reg.num + 2; imm_reg2.is_var = true;
        imm_reg3.num = eexp->reg.num + 3; imm_reg3.is_var = true;
        reg.num = eexp->reg.num + 4;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
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

    void Semantic() override
    {
        laexp->Semantic();
        eexp->Semantic();

        reg.is_var = laexp->reg.is_var || eexp->reg.is_var;
        reg.value = laexp->reg.value && eexp->reg.value;
    }

    int Value() const override
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
        reg.num = laexp->reg.num;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        laexp->DumpIR(os);
    }

    void Semantic() override
    {
        laexp->Semantic();
        reg.is_var = laexp->reg.is_var;
        reg.value = laexp->reg.value;
    }

    int Value() const override
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
        imm_reg1.num = laexp->reg.num + 1; imm_reg1.is_var = true;
        imm_reg2.num = laexp->reg.num + 2; imm_reg2.is_var = true;
        reg.num = laexp->reg.num + 3;
    }

    void DumpIR(ostream &os) const override
    {
        if (!reg.is_var) return;
        loexp->DumpIR(os);
        laexp->DumpIR(os);

        // 按位或非零，则一定有一个为真
        os << imm_reg1 << " = or " << loexp->reg << ", " << laexp->reg << endl;
        os << imm_reg2 << " = eq " << imm_reg1 << ", 0" << endl;
        os << reg << " = eq " << imm_reg2 << ", 0" << endl;
    }

    void Semantic() override
    {
        loexp->Semantic();
        laexp->Semantic();

        reg.is_var = loexp->reg.is_var || laexp->reg.is_var;
        reg.value = loexp->reg.value || laexp->reg.value;
    }

    int Value() const override
    {
        return loexp->Value() || laexp->Value();
    }
};

// Decl ::= ConstDecl;
class Decl2Const_AST: public BaseAST 
{
public: 
    unique_ptr<BaseAST> constdecl;

    void DistriReg(int lb) override 
    {
        constdecl->DistriReg(lb);
        reg.num = lb;
    }

    void DumpIR(ostream &os) const override { /* do nothing. */ }

    void Semantic() override
    {
        constdecl->Semantic();
    }
};

// Decl ::= VarDecl
class Decl2Var_AST: public BaseAST
{
public: 
    unique_ptr<BaseAST> var_decl;

    void DistriReg(int lb) override
    {
        var_decl->DistriReg(lb);
        reg.num = var_decl->reg.num + 1;
    }

    void DumpIR(ostream &os) const override 
    {
        var_decl->DumpIR(os);
    }

    void Semantic() override 
    {
        var_decl->Semantic();
    }
};

// ConstDecl ::= "const" BType ConstDef {"," ConstDef} ";";
class ConstDecl_AST: public BaseAST
{
public: 
    string btype;
    vector<unique_ptr<BaseAST>> constdefs;

    void DistriReg(int lb) override
    {
        for (int i = 0; i < constdefs.size(); i++)
            constdefs[i]->DistriReg(lb);
        reg.num = lb + 1;
    }

    void DumpIR(ostream &os) const override { /* do nothing. */ }
    void Semantic() override 
    {
        for (int i = 0; i < constdefs.size(); i++)
            constdefs[i]->Semantic();
    }
};

// VarDecl ::= BType VarDef {"," VarDef} ";"
class VarDecl_AST: public BaseAST 
{
public:
    string btype;
    vector<unique_ptr<BaseAST>> vardefs;

    void DistriReg(int lb) override 
    {
        for (int i = 0; i < vardefs.size(); i++)
        {
            vardefs[i]->DistriReg(lb);
            lb = max(vardefs[i]->reg.num, lb);
        }
        reg.num = lb + 1;
    }

    void DumpIR(ostream &os) const override 
    {
        for (int i = 0; i < vardefs.size(); i++)
            vardefs[i]->DumpIR(os);
    }

    void Semantic() override 
    {
        for (int i = 0; i < vardefs.size(); i++)
            vardefs[i]->Semantic();
    }
};

// ConstDef ::= IDENT "=" ConstInitVal;
class ConstDef_AST: public BaseAST 
{
public: 
    string ident;
    unique_ptr<BaseAST> constinitval;

    void DistriReg(int lb) override 
    {
        constinitval->DistriReg(lb);
        reg.num = lb;
    }

    void DumpIR(ostream &os) const override { /* do nothing. */ }
    void Semantic() override
    {
        constinitval->Semantic();

        assert(!constinitval->reg.is_var);
        sym_table[ident].is_var = false;
        sym_table[ident].value = constinitval->reg.value;

        reg.is_var = false;
        reg.value = constinitval->reg.value;
    }
};

// VarDef ::= IDENT "=" InitVal
class VarDef_init_AST: public BaseAST
{
public: 
    string ident;
    unique_ptr<BaseAST> initval;

    void DistriReg(int lb) override
    {
        initval->DistriReg(lb);
        reg.num = initval->reg.num;
    }

    void DumpIR(ostream &os) const override
    {
        initval->DumpIR(os);
        os << "@" << ident <<  " = alloc i32" << endl;
        os << "store " << initval->reg << ", @" << ident << endl;
    }

    void Semantic() override
    {
        initval->Semantic();
        sym_table[ident].is_var = true;
        sym_table[ident].value = initval->reg.value;

        reg.is_var = true;
        reg.value = initval->reg.value;
    }

};

// VarDef ::= IDENT
class VarDef_noninit_AST: public BaseAST
{
public: 
    string ident;
    
    void DistriReg(int lb) override 
    {
        reg.num = lb + 1;
    }

    void DumpIR(ostream &os) const override 
    {
        os << "@" << ident <<  " = alloc i32" << endl;
    }

    void Semantic() override
    {
        sym_table[ident].is_var = true;
        sym_table[ident].value = 0;

        reg.is_var = true;
        reg.value = 0;
    }
};

// ConstInitVal ::= ConstExp;
class ConstInitVal_AST: public BaseAST
{
public: 
    unique_ptr<BaseAST> const_exp;

    void DistriReg(int lb) override
    {
        const_exp->DistriReg(lb);
        reg.num = lb;
    }

    void DumpIR(ostream &os) const override {/* do nothing. */}
    void Semantic() override 
    {
        const_exp->Semantic();

        assert(!const_exp->reg.is_var);
        reg.is_var = false;
        reg.value = const_exp->reg.value;
    }
    int Value() const override
    {
        return const_exp->Value();
    }
};

// InitVal ::= Exp;
class InitVal_AST: public BaseAST
{
public: 
    unique_ptr<BaseAST> exp;

    void DistriReg(int lb) override 
    {
        exp->DistriReg(lb);
        reg.num = exp->reg.num;
    }

    void DumpIR(ostream &os) const override
    {
        exp->DumpIR(os);
    }

    void Semantic() override 
    {
        exp->Semantic();
        reg.is_var = exp->reg.is_var;
        reg.value = exp->reg.value;
    }
};

// LVal ::= IDENT;
class LVal_AST: public BaseAST 
{
public: 
    string ident;

    void DistriReg(int lb) override
    {
        reg.num = lb + 1;
    }

    void DumpIR(ostream &os) const override 
    {
        if (!reg.is_var) return;

        // variable: dump here
        os << reg << " = load @" << ident << endl;
    }
    void Semantic() override
    {
        assert(sym_table.find(ident) != sym_table.end());
        reg.is_var = sym_table[ident].is_var;
        reg.value = sym_table[ident].value;
    }
    int Value() const override 
    {
        assert(sym_table.find(ident) != sym_table.end());
        return sym_table[ident].value;
    }
};

// ConstExp ::= Exp;
class ConstExp_AST: public BaseAST
{
public:
    unique_ptr<BaseAST> exp;

    void DistriReg(int lb) override
    {
        exp->DistriReg(lb);
        reg.num = lb;
    }

    void DumpIR(ostream &os) const override {/* do nothing. */}
    void Semantic() override 
    {
        exp->Semantic();
        assert(!exp->reg.is_var);
        reg.is_var = false;
        reg.value = exp->reg.value;
    }

    int Value() const override
    {
        return exp->Value();
    }
};