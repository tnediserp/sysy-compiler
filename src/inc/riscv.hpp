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
void CheckReg(const koopa_raw_value_t &value);
bool Load_imm(const koopa_raw_value_t &value, string reg);
void Load_imm_dump(const koopa_raw_value_t &value, ostream &os);
void Load_addr_dump(const koopa_raw_value_t &value, string reg, ostream &os);
int Stack_size(const koopa_raw_slice_t &slice);
int Stack_size(const koopa_raw_function_t &func);
int Stack_size(const koopa_raw_basic_block_t &bb);
int Stack_size(const koopa_raw_value_t &value);

class Stack
{
public: 
    int size;
    map<koopa_raw_value_t, int> offset;

    Stack(): size(0)
    {
        offset.clear();
    }

    void clear()
    {
        size = 0;
        offset.clear();
    }
};