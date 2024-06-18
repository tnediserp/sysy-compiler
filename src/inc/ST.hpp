#pragma once
#include<string>
#include<vector>
#include<map>
#include<cassert>

using namespace std;

// 符号表中符号代表的可能类型
typedef enum {
    NO_TYPE, // 无效类型
    VALUE_CONST, // 常数
    VALUE_VARIABLE, // 变量
    VALUE_PTR, // 指针
    ARG_VAR, // 函数变量参数
    ARG_PTR, // 函数变量指针
    ARRAY_CONST, // 常量数组
    ARRAY_VARIABLE, // 变量数组
    FUNC_INT, // int 函数
    FUNC_VOID, // void 函数
} ST_item_t;

// 符号表entry
struct ST_item {
    ST_item_t type;
    int dim_num; // 数组维数
    int value;
    
    ST_item(ST_item_t ty = NO_TYPE, int d = 0, int v = 0): type(ty), dim_num(d), value(v) {}
};

// 单个block的符号表
class ST
{
public:
    int num; // 编号
    map<string, ST_item> s_table;

    ST(int n): num(n)
    {
        s_table.clear();
    }
};

// 全局符号表，组织成一个栈的形式，理解成树上的一条路径
class ST_stack
{
public: 
    int ttl_blks; // 记录程序中总的block数（包括已经弹出的）
    int top_num; // 当前栈顶编号
    vector<ST> st_stack;

    ST_stack(int n=0): ttl_blks(n), top_num(n)
    {
        st_stack.clear();
        push_scope(); // push进一个全局作用域
    }

    void push_scope()
    {
        st_stack.push_back(ST(ttl_blks));
        top_num = ttl_blks;
        ttl_blks++;
    }

    void pop_scope()
    {
        st_stack.pop_back();
        int n = st_stack.size();
        if (n > 0) top_num = st_stack[n - 1].num;
        else top_num = 0;
    }

    // 查找标识符ident，返回对应的表项信息和所在符号表的编号
    pair<ST_item, int> 
    look_up(string ident)
    {
        for (int i = st_stack.size() - 1; i >= 0; i--)
        {
            map<string, ST_item>::iterator it = st_stack[i].s_table.find(ident);
            if (it != st_stack[i].s_table.end())
            {
                return make_pair(it->second, st_stack[i].num);
            }
        }
        assert(false);
    }

    // 查找函数标识符
    ST_item find_func(string ident)
    {
        assert(st_stack[0].num == 0);
        map<string, ST_item>::iterator it = st_stack[0].s_table.find(ident);
        if (it != st_stack[0].s_table.end())
        {
            return it->second;
        }
        assert(false);
    }

    // 增加ident及其表项信息。
    // 注意：添加发生在栈顶，专用于变量定义和函数定义
    // 在为函数参数分配实际空间时，会覆盖掉参数
    void add_item(string ident, ST_item item)
    {
        int n = st_stack.size();
        assert(n > 0);

        st_stack[n-1].s_table[ident].type = item.type;
        st_stack[n-1].s_table[ident].dim_num = item.dim_num;
        st_stack[n-1].s_table[ident].value = item.value;
    }

    // 修改ident对应的表项信息
    // 只允许修改值，不允许修改类型
    pair<ST_item, int>
    modify_item(string ident, int value)
    {
        for (int i = st_stack.size() - 1; i >= 0; i--)
        {
            map<string, ST_item>::iterator it = st_stack[i].s_table.find(ident);
            if (it != st_stack[i].s_table.end())
            {
                // it->second.type = item.type;
                it->second.value = value;
                return make_pair(it->second, st_stack[i].num);
            }
        }
        assert(false);
    }
};