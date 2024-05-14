#include <inc/ast.hpp>

ST_stack sym_table; // 符号表
bool eof = false; // 是否遇到return
int if_stmt_num;
int while_stmt_num;
stack<int> while_stack; // 用一个栈来储存while循环的层次
int while_remain_num; // 被break或continue截断的while循环中剩余部分，编号

// 变量ident在ir中的新名字。num是其所在符号表的编号
string IR_name(string ident, int num)
{
    return "var_" + ident + "_" + to_string(num);
}

string if_stmt_name(string ident, int num)
{
    return "%" + ident + "_" + to_string(num);
}

string logic_name(string ident, int num)
{
    return ident + "_" + to_string(num);
}