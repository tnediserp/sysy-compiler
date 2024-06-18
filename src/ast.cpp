#include "inc/ast.hpp"

ST_stack sym_table; // 符号表
bool ret = false; // 是否遇到return
int if_stmt_num;
int while_stmt_num;
int func_num;
stack<int> while_stack; // 用一个栈来储存while循环的层次
int while_remain_num; // 被break或continue截断的while循环中剩余部分，编号
int tmp_addr;
int tmp_reg;

// 变量ident在ir中的新名字。num是其所在符号表的编号
string Var_name(string ident, int num)
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

string func_name(string ident, int num)
{
    return ident;
}

string Arg_name(string ident, int num)
{
    return "arg_" + ident + "_" + to_string(num);
}

string Arr_name(string ident, int num)
{
    return "arr_" + ident + "_" + to_string(num);
}

string Ptr_name(string ident, int num)
{
    return "ptr_" + ident + "_" + to_string(num);
}

// @arr = alloc [[i32, 3], 2]
void Arrange_alloc(vector<int> dims, ostream &os)
{
    // os << "alloc ";
    for (int i = 0; i < dims.size(); i++)
        os << "[";
    os << "i32";
    for (int i = dims.size() - 1; i >= 0; i--)
        os << ", " << dims[i] << "]";
}

// 将数组的初始化列表整理成标准形式输出
void Arrange_init_list(vector<Register> regs_list, int begin, int end, vector<int> dims, ostream &os)
{
    int prod = 1;
    for (int i = 0; i < dims.size(); i++)
        prod *= dims[i];

    assert(prod == end - begin);

    os << "{";
    // 一维数组，直接输出元素
    if (dims.size() == 1) 
    {
        for (int i = begin; i < end; i++)
        {
            if (i == begin) os << regs_list[i];
            else os << ", " << regs_list[i];
        }  
    }

    // 高维数组，递归处理
    else 
    {
        vector<int> tmp_dims;
        for (int i = 1; i < dims.size(); i++)
            tmp_dims.push_back(dims[i]);
        
        for (int i = begin; i < end; i += (prod / dims[0]))
        {
            if (i != begin) os << ", ";
            Arrange_init_list(regs_list, i, i + prod / dims[0], tmp_dims, os);
        }
            
    }
    os << "}";
}

// 局部数组初始赋值
void Store_arr(vector<Register> regs_list, vector<int> dims, int depth, string base, ostream &os)
{
    if (depth == dims.size())
    {
        os << "store " << regs_list[tmp_reg++] << ", " << base << endl;
        return;
    }

    for (int i = 0; i < dims[depth]; i++)
    {
        string new_base = "%addr" + to_string(tmp_addr++);
        os << new_base << " = getelemptr " << base << ", " << i << endl;
        Store_arr(regs_list, dims, depth + 1, new_base, os);
    }
    
}