#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <cstring>
#include <inc/ast.hpp>
#include <inc/koopa.h> // 使用文档提供的文本IR到内存IR转换的标准接口

using namespace std;

// macros
#define FILE_LEN 10000

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
koopa_raw_program_t str2raw(const char *str);
void DumpRISC(const koopa_raw_program_t &program, ostream &os);
void DumpRISC(const koopa_raw_slice_t &slice, ostream &os);
void DumpRISC(const koopa_raw_function_t &func, ostream &os);
void DumpRISC(const koopa_raw_basic_block_t &bb, ostream &os);
void DumpRISC(const koopa_raw_value_t &value, ostream &os);
void DumpRISC(const koopa_raw_integer_t &integer, ostream &os);
void DumpRISC(const koopa_raw_return_t &ret, ostream &os);

// 将文本形式IR转换为内存形式
koopa_raw_program_t str2raw(const char *str)
{
    // 解析字符串 str, 得到 Koopa IR 程序
    koopa_program_t program;
    koopa_error_code_t ret = koopa_parse_from_string(str, &program);
    assert(ret == KOOPA_EC_SUCCESS);  // 确保解析时没有出错
    // 创建一个 raw program builder, 用来构建 raw program
    koopa_raw_program_builder_t builder = koopa_new_raw_program_builder();
    // 将 Koopa IR 程序转换为 raw program
    koopa_raw_program_t raw = koopa_build_raw_program(builder, program);
    // 释放 Koopa IR 程序占用的内存
    koopa_delete_program(program);
    return raw;
}


// 访问 raw program
void DumpRISC(const koopa_raw_program_t &program, ostream &os)
{
    os << ".text" << endl;

    DumpRISC(program.values, os);
    // 访问所有函数
    DumpRISC(program.funcs, os);
}

// 访问 raw slice
void DumpRISC(const koopa_raw_slice_t &slice, ostream &os)
{
    for (size_t i = 0; i < slice.len; ++i) 
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                // 访问函数
                DumpRISC(reinterpret_cast<koopa_raw_function_t>(ptr), os);
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 访问基本块
                DumpRISC(reinterpret_cast<koopa_raw_basic_block_t>(ptr), os);
                break;
            case KOOPA_RSIK_VALUE:
                // 访问指令
                DumpRISC(reinterpret_cast<koopa_raw_value_t>(ptr), os);
                break;
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
        }
    }
}

// 访问函数
void DumpRISC(const koopa_raw_function_t &func, ostream &os)
{
    os << ".globl " << 1 + func->name << endl;
    os << 1 + func->name << ":" << endl;
    DumpRISC(func->bbs, os);
}

// 访问基本块
void DumpRISC(const koopa_raw_basic_block_t &bb, ostream &os)
{
    DumpRISC(bb->insts, os);
}

// 访问指令
void DumpRISC(const koopa_raw_value_t &value, ostream &os)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    switch (kind.tag) 
    {
        case KOOPA_RVT_RETURN:
            // 访问 return 指令
            DumpRISC(kind.data.ret, os);
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            DumpRISC(kind.data.integer, os);
            break;
        default:
            // 其他类型暂时遇不到
            assert(false);
    }
}

// 访问对应类型指令的函数定义

// integer
void DumpRISC(const koopa_raw_integer_t &integer, ostream &os)
{
    os << integer.value;
}

// ret 
void DumpRISC(const koopa_raw_return_t &ret, ostream &os)
{
    os << "li a0, ";
    DumpRISC(ret.value, os);
    os << endl << "ret" << endl;
}



int main(int argc, const char *argv[]) {
    // 解析命令行参数. 测试脚本/评测平台要求你的编译器能接收如下参数:
    // compiler 模式 输入文件 -o 输出文件
    assert(argc == 5);
    auto mode = argv[1];
    auto input = argv[2];
    auto output = argv[4];

    // 打开输入文件, 并且指定 lexer 在解析的时候读取这个文件
    yyin = fopen(input, "r");
    assert(yyin);

    // 调用 parser 函数, parser 函数会进一步调用 lexer 解析输入文件的
    unique_ptr<BaseAST> ast;
    auto ret = yyparse(ast);
    assert(!ret);

    ast->DistriReg();

    // 使用临时文件tmp保存文本形式IR
    ofstream tmp;
    tmp.open("tmp.koopa");
    ast->DumpIR(tmp);
    tmp.close();

    // 根据mode决定生成何种形式文件
    // koopa IR
    if (!strcmp(mode, "-koopa"))
    {
        ofstream yyout;
        yyout.open(output);
        ast->DumpIR(yyout);
        yyout.close();
    }
    
    // 目标代码
    else if (!strcmp(mode, "-riscv"))
    {
        // 从临时文件中读出文本形式IR，存入raw
        FILE *fp = fopen("tmp.koopa", "r");
        char *str = new char [FILE_LEN];
        fread(str, 1, FILE_LEN, fp);
        koopa_raw_program_t raw = str2raw(str);
        delete [] str;

        ofstream yyout;
        yyout.open(output);
        DumpRISC(raw, yyout);
        yyout.close();
    }

    else cerr << "wrong mode." << endl;


    return 0;
}