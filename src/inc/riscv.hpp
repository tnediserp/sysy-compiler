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
void DumpRISC(const koopa_raw_binary_t &binary, string reg, ostream &os);
void CheckReg(const koopa_raw_value_t &value);
bool Load_imm(const koopa_raw_value_t &value, string reg);
void Load_imm_dump(const koopa_raw_value_t &value, ostream &os);

