#include <inc/ast.hpp>

ST_stack sym_table; // 符号表
bool eof = false; // 是否遇到return
int if_stmt_num;

// 变量ident在ir中的新名字。num是其所在符号表的编号
string IR_name(string ident, int num)
{
    return ident + "_" + to_string(num);
}

string if_stmt_name(string ident, int num)
{
    return "%" + ident + "_" + to_string(num);
}