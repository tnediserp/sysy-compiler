#include <inc/ast.hpp>
// 变量ident在ir中的新名字。num是其所在符号表的编号
string IR_name(string ident, int num)
{
    return ident + "_" + to_string(num);
}