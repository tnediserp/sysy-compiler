#pragma once
#include<string>
#include<vector>
#include<cassert>

// 符号表entry
struct ST_item {
    bool is_var;
    int value;

    ST_item(bool b = true, int v = 0): is_var(b), value(v) {}
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
        if (n == 0)
            st_stack.clear();
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

    // 增加ident及其表项信息。
    // 注意：添加发生在栈顶，专用于变量定义
    void add_item(string ident, ST_item item)
    {
        int n = st_stack.size();
        assert(n > 0);

        st_stack[n-1].s_table[ident].is_var = item.is_var;
        st_stack[n-1].s_table[ident].value = item.value;
    }

    // 修改ident对应的表项信息
    pair<ST_item, int>
    modify_item(string ident, ST_item item)
    {
        for (int i = st_stack.size() - 1; i >= 0; i--)
        {
            map<string, ST_item>::iterator it = st_stack[i].s_table.find(ident);
            if (it != st_stack[i].s_table.end())
            {
                it->second.is_var = item.is_var;
                it->second.value = item.value;
                return make_pair(it->second, st_stack[i].num);
            }
        }
        assert(false);
    }
};