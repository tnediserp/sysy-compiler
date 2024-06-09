#pragma once
#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <cstring>
#include "ast.hpp"
#include "koopa.h" // 使用文档提供的文本IR到内存IR转换的标准接口
#include <map>
#include <unordered_map>

using namespace std;

koopa_raw_program_t str2raw(const char *str);
void Dist_regs(const koopa_raw_program_t &program);
void Dist_regs(const koopa_raw_slice_t &slice);
void Dist_regs(const koopa_raw_function_t &func);
void Dist_regs(const koopa_raw_basic_block_t &bb);
void Dist_regs(const koopa_raw_value_t &value);
void DumpRISC(const koopa_raw_program_t &program, ostream &os);
void DumpRISC(const koopa_raw_slice_t &slice, ostream &os);
void DumpRISC(const koopa_raw_function_t &func, ostream &os);
void DumpRISC(const koopa_raw_basic_block_t &bb, ostream &os);
void DumpRISC(const koopa_raw_value_t &value, ostream &os);
void DumpRISC(const koopa_raw_integer_t &integer, ostream &os);
void DumpRISC(const koopa_raw_return_t &ret, string reg, ostream &os);
void DumpRISC(const koopa_raw_binary_t &binary, ostream &os);
void DumpRISC(const koopa_raw_global_alloc_t &alloc, ostream &os);
void DumpRISC(const koopa_raw_load_t &load, ostream &os);
void DumpRISC(const koopa_raw_store_t &store, ostream &os);
void DumpRISC(const koopa_raw_branch_t &branch, ostream &os);
void DumpRISC(const koopa_raw_jump_t &jump, ostream &os);
void DumpRISC(const koopa_raw_call_t &call, ostream &os);
void DumpRISC(const koopa_raw_global_alloc_t &alloc, const koopa_raw_value_t &value, ostream &os);
void DumpRISC(const koopa_raw_get_elem_ptr_t &getelemptr, const koopa_raw_value_t &value, ostream &os);
void DumpRISC(const koopa_raw_get_ptr_t &getptr, const koopa_raw_value_t &value, ostream &os);
void DumpRISC(const koopa_raw_aggregate_t &aggregate, ostream &os);
void CheckReg(const koopa_raw_value_t &value);
bool Load_imm(const koopa_raw_value_t &value, string reg);
void Load_imm_dump(const koopa_raw_value_t &value, ostream &os);
void Load_addr_dump(const koopa_raw_value_t &value, string reg, ostream &os);
void Load_addr_dump(int offset, string reg, ostream &os);
void Store_addr_dump(const koopa_raw_value_t &value, string reg, ostream &os);
void Store_addr_dump(int offset, string reg, ostream &os);
int Stack_size(const koopa_raw_slice_t &slice);
int Stack_size(const koopa_raw_function_t &func);
int Stack_size(const koopa_raw_basic_block_t &bb);
int Stack_size(const koopa_raw_value_t &value);
int Ptr_size(const koopa_raw_type_t &ty);

class Stack
{
public: 
    int size; // 栈的大小
    int R_size; // 储存ra需要的空间
    int S_size; // 局部变量所需空间
    int A_size; // 传参预留的栈空间
    map<koopa_raw_value_t, int> S_offset; // 局部变量偏移（相对于局部变量）

    Stack(): size(0), R_size(0), S_size(0), A_size(0)
    {
        S_offset.clear();
    }

    void clear()
    {
        size = R_size = S_size = A_size = 0;
        S_offset.clear();
    }

    // 返回value在栈中真正的offset
    // 如果返回值<0,说明value是函数参数，存在寄存器中
    int Offset(koopa_raw_value_t value)
    {
        assert(S_offset.find(value) != S_offset.end());
        // 局部变量
        if (S_offset[value] >= 0)
            return S_offset[value] + A_size;

        // 第9个以后的参数
        else if (S_offset[value] < -32)
            return size - (S_offset[value] + 36);
        
        // 前8个参数
        else return S_offset[value];
    }

    int Align() 
    {
        size = R_size + A_size + S_size;
        size = 16 * ((size + 15) / 16);
        return size;
    }
};