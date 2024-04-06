#pragma once
#include <iostream>
#include <fstream>
#include <string>
#include <memory>
using namespace std; 

// The base class of koopa IR
class BaseIR
{
public:
    BaseIR() = default;
    virtual ~BaseIR() = default;
    virtual void Dump(ostream &os) const = 0;
};

// Program
class ProgramIR: public BaseIR
{
public:
    int var_num, func_num;
    unique_ptr<BaseIR> *var_list;
    unique_ptr<BaseIR> *func_list;

    ProgramIR(int vnum = 0, int fnum = 0): var_num(vnum), func_num(fnum)
    {
        var_list = new unique_ptr<BaseIR>[var_num];
        func_list = new unique_ptr<BaseIR>[func_num];
    }

    ~ProgramIR()
    {
        delete [] var_list;
        delete [] func_list;
    }

    void Dump(ostream &os) const override
    {
        int i;

        for (i = 0; i < var_num; i++)
            var_list[i]->Dump(os);

        for (i = 0; i < func_num; i++)
            func_list[i]->Dump(os);
    }
};

// Functions
class FuncIR: public BaseIR
{
public:
    int block_num;
    string ident;
    unique_ptr<BaseIR> *block_list;

    FuncIR(int bnum, string s): block_num(bnum), ident(s)
    {
        block_list = new unique_ptr<BaseIR>[block_num];
    }

    ~FuncIR()
    {
        delete [] block_list;
    }

    void Dump(ostream &os) const override
    {
        int i;

        os << "fun @" << ident << "(): i32 {" << endl;
        for (i = 0; i < block_num; i++)
            block_list[i]->Dump(os);
        
        os << "}";
    }
};

// Basic blocks
class BlockIR: public BaseIR
{
public:
    int instr_num; 
    string ident;
    unique_ptr<BaseIR> *instr_list;

    BlockIR(int inum, string s): instr_num(inum), ident(s)
    {
        instr_list = new unique_ptr<BaseIR>[instr_num];
    }

    ~BlockIR()
    {
        delete [] instr_list;
    }

    void Dump(ostream &os) const override 
    {
        int i;
        os << "%" << ident << ":" << endl;
        for (i = 0; i < instr_num; i++)
        {
            instr_list[i]->Dump(os);
            os << endl;
        }
    }
};

enum ValueKind {
        INTEGER,
        ZEROINIT,
        FUNCARGREF,
        GLOBALALLOC,
        LOAD, 
        STORE, 
        GETPTR,
        GETELEMPTR,
        BINARY, 
        BRANCH, 
        JUMP, 
        CALL, 
        RET
    };

// values
class ValueIR: public BaseIR
{
public:
    ValueKind value_kind;
    int value;

    ValueIR(ValueKind kind, int v): BaseIR(), value_kind(kind), value(v) {}

    void Dump(ostream &os) const override
    {
        os << "   ";
        switch (value_kind)
        {
        case RET:
            os << "ret " << value;
            break;
        
        default:
            cerr << "bad value kind.";
        }
    }
};
