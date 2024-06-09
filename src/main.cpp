#include <cassert>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <cstring>
#include <inc/ast.hpp>
#include <inc/koopa.h> // 使用文档提供的文本IR到内存IR转换的标准接口
#include <inc/riscv.hpp>
#include <map>

using namespace std;

// macros
#define FILE_LEN 5000000

// 声明 lexer 的输入, 以及 parser 函数
// 为什么不引用 sysy.tab.hpp 呢? 因为首先里面没有 yyin 的定义
// 其次, 因为这个文件不是我们自己写的, 而是被 Bison 生成出来的
// 你的代码编辑器/IDE 很可能找不到这个文件, 然后会给你报错 (虽然编译不会出错)
// 看起来会很烦人, 于是干脆采用这种看起来 dirty 但实际很有效的手段
extern FILE *yyin;
extern int yyparse(unique_ptr<BaseAST> &ast);
// extern map<koopa_raw_value_t, string> registers;
extern Stack stack;

extern ST_stack sym_table; // 符号表
extern bool eof; // 是否遇到return


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

    ast->Semantic();
    ast->DistriReg(0);

    // 根据mode决定生成何种形式文件
    // koopa IR
    if (!strcmp(mode, "-koopa"))
    {
        ofstream yyout;
        yyout.open(output);
        ast->DumpIR(yyout);
        yyout.close();

        // ast->DumpIR(cout);
    }
    
    // 目标代码
    else if (!strcmp(mode, "-riscv"))
    {
        // 使用临时文件tmp保存文本形式IR
        ofstream tmp;
        tmp.open("tmp.koopa");
        ast->DumpIR(tmp);
        tmp.close();
        
        // 从临时文件中读出文本形式IR，存入raw
        FILE *fp = fopen("tmp.koopa", "r");
        char *str = new char [FILE_LEN];
        fread(str, 1, FILE_LEN, fp);
        koopa_raw_program_t raw = str2raw(str);
        delete [] str;

        // registers.clear();
        // Dist_regs(raw);

        ofstream yyout;
        yyout.open(output);
        DumpRISC(raw, yyout);
        yyout.close();
    }

    else cerr << "wrong mode." << endl;


    return 0;
}