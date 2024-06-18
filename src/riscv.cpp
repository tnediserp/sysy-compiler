#include "inc/riscv.hpp"
#include "inc/ST.hpp"

map<koopa_raw_value_t, string> registers;
Stack rstack; // 记录该变量在栈中相对栈指针的偏移量
map<koopa_raw_value_t, string> glob_data; // 储存全局变量名
int new_branch_num; // 用于间接跳转的新标签

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

/**
 * 将整数映射到寄存器名
 * 
 */
string cast_Reg(int value_num)
{
    // assert(value_num <= 14);
    string str;
    if (value_num >= 0 && value_num <= 6)
    {
        str = "t" + string(1, value_num + '0');
        return str;
    }

    else // if (value_num >= 7 && value_num <= 14)
    {
        str = "a" + string(1, value_num - 7 + '0');
        return str;
    }    
}

string next_Reg(string reg)
{
    string str = reg;
    assert((reg[0] == 't' && reg[1] >= '0' && reg[1] <= '6') || (reg[0] == 'a' && reg[1] >= '0'));

    if (reg == "t6")
        return "a0";

    str[1] = reg[1] + 1;
    return str;
}

/**
 * 给指令命名
 * 
 */
void Name_value(koopa_raw_value_t value, int val_num)
{
    registers[value] = cast_Reg(val_num);
}

void Dist_regs(const koopa_raw_program_t &program)
{
    Dist_regs(program.values);
    Dist_regs(program.funcs);
}

void Dist_regs(const koopa_raw_slice_t &slice)
{
    int val_num = 0;
    for (size_t i = 0; i < slice.len; ++i) 
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                // 访问函数
                Dist_regs(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 访问基本块
                Dist_regs(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
            {
                // 访问指令
                koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(ptr);
                if (value->kind.tag == KOOPA_RVT_RETURN)
                    registers[value] = "a0";
                else 
                    Name_value(value, val_num);
                val_num++;
                break;
            }
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
        }
    }
}

void Dist_regs(const koopa_raw_function_t &func)
{
    Dist_regs(func->bbs);
}

void Dist_regs(const koopa_raw_basic_block_t &bb)
{
    Dist_regs(bb->insts);
}

void Dist_regs(const koopa_raw_value_t &value)
{

}

// 计算slice需要的总共栈空间
int Stack_size(const koopa_raw_slice_t &slice)
{
    int size = 0;
    for (size_t i = 0; i < slice.len; ++i) 
    {
        auto ptr = slice.buffer[i];
        // 根据 slice 的 kind 决定将 ptr 视作何种元素
        switch (slice.kind) {
            case KOOPA_RSIK_FUNCTION:
                // 访问函数
                size += Stack_size(reinterpret_cast<koopa_raw_function_t>(ptr));
                break;
            case KOOPA_RSIK_BASIC_BLOCK:
                // 访问基本块
                size += Stack_size(reinterpret_cast<koopa_raw_basic_block_t>(ptr));
                break;
            case KOOPA_RSIK_VALUE:
            {
                // 访问指令
                size += Stack_size(reinterpret_cast<koopa_raw_value_t>(ptr));
                break;
            }
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
        }
    }
    return size;
}

// 计算函数需要的栈空间
int Stack_size(const koopa_raw_function_t &func)
{
    // 清空栈帧
    rstack.clear();
    // 首先给函数参数分配栈空间
    for (int i = 0; i < func->params.len; i++)
    {
        koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(func->params.buffer[i]);

        rstack.S_offset[value] = -4*i - 4;
        // S_size不需要+1，因为参数存在上一个函数（调用者）的栈帧中
    }

    return Stack_size(func->bbs);
}

// 计算基本块需要的栈空间
int Stack_size(const koopa_raw_basic_block_t &bb)
{
    return Stack_size(bb->insts);
}

// 计算指令需要的局部变量空间，并为该条指令在栈上分配一个位置
// 如果是call 指令，更新A_size
int Stack_size(const koopa_raw_value_t &value)
{   
    size_t size = 4; // 指令需要的局部变量空间，默认为4
    // 不需要栈空间的情形
    if (value->kind.tag == KOOPA_RVT_RETURN || value->kind.tag == KOOPA_RVT_INTEGER || value->kind.tag == KOOPA_RVT_STORE || value->kind.tag == KOOPA_RVT_JUMP || value->kind.tag == KOOPA_RVT_BRANCH)
        return 0;

    // 如果是getelemptr指令，则getelemptr的结果（一个地址）会被作为临时变量储存在栈上

    // 如果是alloc指令，计算所需的空间
    if (value->kind.tag == KOOPA_RVT_ALLOC)
    {
        size = Ptr_size(value->ty);
    }
    // ad-hoc solution: size = 4;
    rstack.S_offset[value] = rstack.S_size;
    rstack.S_size += size;

    // 如果是call指令，根据参数个数更新A_size
    if (value->kind.tag == KOOPA_RVT_CALL)
    {
        int arg_num = value->kind.data.call.args.len;
        rstack.A_size = 4 * max(rstack.A_size, arg_num - 8);
        rstack.R_size = 4;
        // return 4; // %1 = call f()
    }
    return size;
}

// 计算指针对应的空间大小
int Ptr_size(const koopa_raw_type_t &ty)
{
    auto &ptr = ty->data.pointer;
    // 整数 alloc i32
    if (ptr.base->tag == KOOPA_RTT_INT32)
        return 4;
    
    // 指针 alloc *i32 / *[i32, 2]
    else if (ptr.base->tag == KOOPA_RTT_POINTER)
        return 4;
    
    // 数组 alloc [i32, 2]
    return ptr.base->data.array.len * Ptr_size(ptr.base);
}


// 访问 raw program
void DumpRISC(const koopa_raw_program_t &program, ostream &os)
{
    // os << ".text" << endl;

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
            {
                // 访问指令
                DumpRISC(reinterpret_cast<koopa_raw_value_t>(ptr), os);
                break;
            }
            default:
                // 我们暂时不会遇到其他内容, 于是不对其做任何处理
                assert(false);
        }
    }
}

// 访问全局变量
void DumpRISC(const koopa_raw_global_alloc_t &alloc, const koopa_raw_value_t &value, ostream &os)
{
    int num = glob_data.size();
    string name = "var_" + to_string(num);
    glob_data[value] = name;

    os << ".data" << endl;
    os << ".globl " << name << endl;
    os << name << ":" << endl;

    if (alloc.init->kind.tag == KOOPA_RVT_INTEGER)
        os << ".word " << alloc.init->kind.data.integer.value << endl << endl;

    else if (alloc.init->kind.tag == KOOPA_RVT_ZERO_INIT)
        os << ".zero " << Ptr_size(value->ty) << endl << endl;
    
    else // alloc.init->kind.tag == KOOPA_RVT_AGGREGATE
        DumpRISC(alloc.init->kind.data.aggregate, os);
}

// aggregate
void DumpRISC(const koopa_raw_aggregate_t &aggregate, ostream &os)
{
    for (int i = 0; i < aggregate.elems.len; i++)
    {
        koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(aggregate.elems.buffer[i]);
        if (value->kind.tag == KOOPA_RVT_AGGREGATE)
            DumpRISC(value->kind.data.aggregate, os);
        else // value->kind.tag == KOOPA_RVT_INTEGER
            os << ".word " << value->kind.data.integer.value << endl;
    }

    os << endl;
}

// 访问函数
void DumpRISC(const koopa_raw_function_t &func, ostream &os)
{
    // 跳过库函数声明
    if (func->bbs.len == 0) 
        return;
    
    os << ".text" << endl;
    os << ".globl " << 1 + func->name << endl;
    os << 1 + func->name << ":" << endl;

    // 计算函数所需栈的大小
    int stack_size = Stack_size(func);
    stack_size = rstack.Align(); // 16字节对齐
    
    // 使用 addi
    if (stack_size < 2048)
        os << "addi sp, sp, " << -stack_size << endl;
    
    // 使用 add
    else 
    {
        os << "li t0, " << -stack_size << endl;
        os << "add sp, sp, t0" << endl;
    }

    // 储存ra寄存器
    if (rstack.R_size != 0)
    {
        int ra_pos = rstack.size - 4;
        Store_addr_dump(ra_pos, "ra", os);
    }

    DumpRISC(func->bbs, os);
}

// 访问基本块
void DumpRISC(const koopa_raw_basic_block_t &bb, ostream &os)
{
    os << bb->name + 1 << ":" << endl;
    DumpRISC(bb->insts, os);
}


/**
 * 访问指令
 * 
 */
void DumpRISC(const koopa_raw_value_t &value, ostream &os)
{
    // 根据指令类型判断后续需要如何访问
    const auto &kind = value->kind;
    switch (kind.tag) 
    {
        case KOOPA_RVT_RETURN:
            // 访问 return 指令
            DumpRISC(kind.data.ret, registers[value], os);
            os << endl;
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            DumpRISC(kind.data.integer, os);
            break;
        case KOOPA_RVT_BINARY: 
            DumpRISC(kind.data.binary, os);
            Store_addr_dump(value, "t0", os);
            os << endl;
            break;
        case KOOPA_RVT_LOAD: 
            DumpRISC(kind.data.load, os);
            Store_addr_dump(value, "t0", os);
            os << endl;
            break;
        case KOOPA_RVT_STORE: 
            DumpRISC(kind.data.store, os);
            os << endl;
            break;
        case KOOPA_RVT_ALLOC:
            // os << value->ty->data.pointer.base->tag << endl;
            break;
        case KOOPA_RVT_BRANCH: 
            DumpRISC(kind.data.branch, os);
            os << endl;
            break;
        case KOOPA_RVT_JUMP:
            DumpRISC(kind.data.jump, os);
            os << endl;
            break;
        case KOOPA_RVT_CALL:
            DumpRISC(kind.data.call, os);
            Store_addr_dump(value, "a0", os);
            os << endl;
            break;
        case KOOPA_RVT_GLOBAL_ALLOC:
            DumpRISC(kind.data.global_alloc, value, os);
            break;
        case KOOPA_RVT_GET_ELEM_PTR:
            DumpRISC(kind.data.get_elem_ptr, value, os);
            os << endl;
            break;
        case KOOPA_RVT_GET_PTR:
            DumpRISC(kind.data.get_ptr, value, os);
            os << endl;
            break;
        default:
            // 其他类型暂时遇不到
            os << kind.tag << endl;
            // assert(false);
    }
}

// 访问对应类型指令的函数定义

// integer
void DumpRISC(const koopa_raw_integer_t &integer, ostream &os)
{
    os << integer.value;
}

// ret 
void DumpRISC(const koopa_raw_return_t &ret, string reg, ostream &os)
{
    // 若有返回值，加载到a0中
    if (ret.value != NULL)
    {
        Load_addr_dump(ret.value, "a0", os);
    }
    
    // 恢复ra寄存器
    if (rstack.R_size != 0)
    {
        int ra_pos = rstack.size - 4;
        Load_addr_dump(ra_pos, "ra", os);
    }

    // 恢复栈指针
    int stack_size = 16 * ((rstack.size + 15) / 16);

    // 使用 addi
    if (stack_size < 2048)
        os << "addi sp, sp, " << stack_size << endl;
    
    // 使用 add
    else 
    {
        os << "li t0, " << stack_size << endl;
        os << "add sp, sp, t0" << endl;
    }

    os << "ret" << endl;
}

// load
void DumpRISC(const koopa_raw_load_t &load, ostream &os)
{
    auto &src = load.src;
    // 如果是这两种类型，需要先从栈上取出地址，再load该地址
    if (src->kind.tag == KOOPA_RVT_GET_ELEM_PTR || src->kind.tag == KOOPA_RVT_GET_PTR)
    {
        Load_addr_dump(src, "t5", os); // t5中存着真正需要被load的地址
        os << "lw t0, 0(t5)" << endl; // t0中存着得到的值
    }
    else Load_addr_dump(src, "t0", os);
    // os << "sw t0, " << stack.offset[value] << endl;
}

// store value, dest
void DumpRISC(const koopa_raw_store_t &store, ostream &os)
{
    auto &dest = store.dest;
    auto &value = store.value;
    // 如果是这两种类型，需要先从栈上取出地址，再存入该地址
    if (dest->kind.tag == KOOPA_RVT_GET_ELEM_PTR || dest->kind.tag == KOOPA_RVT_GET_PTR)
    {
        Load_addr_dump(value, "t0", os); // t0中存着要被store的值
        Load_addr_dump(dest, "t5", os); // t5中存着真正的目标地址
        os << "sw t0, 0(t5)" << endl;
    }
    else {
        Load_addr_dump(value, "t0", os);
        Store_addr_dump(dest, "t0", os);
    }
    
}

// branch
void DumpRISC(const koopa_raw_branch_t &branch, ostream &os)
{
    // 实现间接跳转
    Load_addr_dump(branch.cond, "t0", os);
    os << "bnez t0, new_branch_true_" << new_branch_num << endl;
    os << "j new_branch_false_" << new_branch_num << endl;

    os << "new_branch_true_" << new_branch_num << ":" << endl;
    os << "la t0, " << branch.true_bb->name + 1 << endl;
    os << "jalr t0, t0, 0" << endl;

    os << "new_branch_false_" << new_branch_num << ":" << endl;
    os << "la t0, " << branch.false_bb->name + 1 << endl;
    os << "jalr t0, t0, 0" << endl;

    new_branch_num++;
}

// jump
void DumpRISC(const koopa_raw_jump_t &jump, ostream &os)
{
    // 实现间接跳转
    os << "la t0, "  << jump.target->name + 1 << endl;
    os << "jalr t0, t0, 0" << endl;
}

// call
void DumpRISC(const koopa_raw_call_t &call, ostream &os)
{
    for (int i = 0; i < call.args.len; i++)
    {
        koopa_raw_value_t value = reinterpret_cast<koopa_raw_value_t>(call.args.buffer[i]);

        // 前8个参数load到寄存器a0-a7中
        if (i < 8)
        {
            string reg = "a" + to_string(i);
            Load_addr_dump(value, reg, os);
        }

        // 后面的寄存器放在栈帧上，相当于一次store
        else 
        {
            Load_addr_dump(value, "t0", os);
            int offset = (i - 8) * 4;
            Store_addr_dump(offset, "t0", os);
        }
    }

    os << "call " << call.callee->name + 1 << endl;

    // 把返回值保存到call指令对应的内存中
}

// getelemptr
void DumpRISC(const koopa_raw_get_elem_ptr_t &getelemptr, const koopa_raw_value_t &value, ostream &os)
{
    auto &src = getelemptr.src;
    auto &index = getelemptr.index;

    // 如果src是alloc出来的，那么其地址已知，此时直接add
    if (src->kind.tag == KOOPA_RVT_ALLOC)
    {
        int offset = rstack.Offset(src);
        // 使用 addi
        if (offset < 2048)
            os << "addi t0, sp, " << offset << endl;
        
        // 使用 add
        else 
        {
            os << "li t1, " << offset << endl;
            os << "add t0, sp, t1" << endl;
        }
    }

    // 如果src是全局变量，处理方法类似，但是使用la
    else if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
        os << "la t0, " << glob_data[src] << endl;

    // 否则src是getelemptr的结果，此时真正基地址存在临时变量里
    else  // src->kind.tag == KOOPA_RVT_GET_ELEM_PTR
        Load_addr_dump(src, "t0", os);
    
    // 到此，基地址存放在t0中
    
    Load_addr_dump(index, "t1", os); // index存放在t1中

    int size = Ptr_size(value->ty); // 一个单元的长度
    os << "li t2" << ", " << size << endl; // size存放在t2中

    os << "mul t1, t1, t2" << endl;
    os << "add t0, t0, t1" << endl;

    // 存放结果
    Store_addr_dump(value, "t0", os);
}

// getptr
void DumpRISC(const koopa_raw_get_ptr_t &getptr, const koopa_raw_value_t &value, ostream &os)
{
    auto &src = getptr.src;
    auto &index = getptr.index;

    // 如果src是alloc出来的，那么其地址已知，此时直接add
    if (src->kind.tag == KOOPA_RVT_ALLOC)
    {
        int offset = rstack.Offset(src);
        // 使用 addi
        if (offset < 2048)
            os << "addi t0, sp, " << offset << endl;
        
        // 使用 add
        else 
        {
            os << "li t1, " << offset << endl;
            os << "add t0, sp, t1" << endl;
        }
    }

    // 如果src是全局变量，处理方法类似，但是使用la
    else if (src->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
        os << "la t0, " << glob_data[src] << endl;

    // 否则src是load的结果，此时真正基地址存在临时变量里
    else  // src->kind.tag == KOOPA_RVT_GET_ELEM_PTR
        Load_addr_dump(src, "t0", os);
    
    // 到此，基地址存放在t0中
    
    Load_addr_dump(index, "t1", os); // index存放在t1中

    int size = Ptr_size(value->ty); // 一个单元的长度
    os << "li t2" << ", " << size << endl; // size存放在t2中

    os << "mul t1, t1, t2" << endl;
    os << "add t0, t0, t1" << endl;

    // 存放结果
    Store_addr_dump(value, "t0", os);
}

// 检查指令是否已分配寄存器
void CheckReg(const koopa_raw_value_t &value)
{
    assert(registers.find(value) != registers.end());
}

// 给立即数分配寄存器，若已分配（例如一条binary指令），返回false
// 否则返回true
bool Load_imm(const koopa_raw_value_t &value, string reg)
{
    // 已经分配了寄存器
    if (registers.find(value) != registers.end())
        return false;
    
    assert(value->kind.tag == KOOPA_RVT_INTEGER);
    
    if (value->kind.data.integer.value == 0)
    {
        registers[value] = "x0";
        return false;
    }
        
    else 
    {
        registers[value] = reg;
        return true;
    } 
}

// 输出 li reg, imm 指令
void Load_imm_dump(const koopa_raw_value_t &value, ostream &os)
{
    CheckReg(value);
    assert(value->kind.tag == KOOPA_RVT_INTEGER);

    os << "li " << registers[value] << ", " << value->kind.data.integer.value << endl;
}

// 把value对应的内容（可能是一个地址或立即数）load到寄存器reg中
void Load_addr_dump(const koopa_raw_value_t &value, string reg, ostream &os)
{
    // integer
    if (value->kind.tag == KOOPA_RVT_INTEGER)
        os << "li " << reg << ", " << value->kind.data.integer.value << endl;

    // global variable
    else if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        os << "la t1, " << glob_data[value] << endl;
        os << "lw " << reg << ", 0(t1)" << endl;
    }
        
    // address
    else 
    {
        int offset = rstack.Offset(value);
        Load_addr_dump(offset, reg, os);
    }       
}

// 把reg的内容store到value对应的地址中
void Store_addr_dump(const koopa_raw_value_t &value, string reg, ostream &os)
{
    if (value->kind.tag == KOOPA_RVT_GLOBAL_ALLOC)
    {
        os << "la t1, " << glob_data[value] << endl;
        os << "sw " << reg << ", 0(t1)" << endl;
    }

    else 
    {
        int offset = rstack.Offset(value);
        Store_addr_dump(offset, reg, os);
    }
}

// 把地址sp + offset中的内容load到寄存器reg中
void Load_addr_dump(int offset, string reg, ostream &os)
{
    // value是函数参数，存在寄存器中
    if (offset < 0)
    {
        os << "mv " << reg << ", " << "a" << -offset / 4 - 1 << endl;
    }

    // 不在寄存器中
    else if (offset < 2048)
        os << "lw " << reg << ", " << offset << "(sp)" << endl;
    else 
    {
        os << "li t1, " << offset << endl;
        os << "add t1, t1, sp" << endl;
        os << "lw " << reg << ", 0(t1)" << endl;
    }
}

// 把寄存器reg中的内容store到地址sp + offset中
void Store_addr_dump(int offset, string reg, ostream &os)
{
    if (offset < 2048)
        os << "sw " << reg << ", " << offset << "(sp)" << endl;
    
    else {
        os << "li t1, " << offset << endl;
        os << "add t1, t1, sp" << endl;
        os << "sw " << reg << ", 0(t1)" << endl;
    }
}

// binary
void DumpRISC(const koopa_raw_binary_t &binary, ostream &os)
{
    // 将lhs load 到 t0 中，rhs load 到 t1 中
    Load_addr_dump(binary.lhs, "t0", os);
    Load_addr_dump(binary.rhs, "t1", os);


    switch (binary.op)
    {
    case KOOPA_RBO_NOT_EQ:
        os << "xor t0, t0, t1" << endl;
        os << "snez t0, t0" << endl;
        break;
    case KOOPA_RBO_EQ:
        // xor t0, t0, x0;
        os << "xor t0, t0, t1" << endl;

        // seqz t0
        os << "seqz t0, t0" << endl;
        break;

    case KOOPA_RBO_ADD:
        os << "add t0, t0, t1" << endl;
        break;
    
    case KOOPA_RBO_SUB:
        os << "sub t0, t0, t1" << endl;
        break;

    case KOOPA_RBO_MUL:
        os << "mul t0, t0, t1" << endl;
        break;


    case KOOPA_RBO_DIV: 
        os << "div t0, t0, t1" << endl;
        break;

    case KOOPA_RBO_MOD:
        os << "rem t0, t0, t1" << endl;
        break;

    case KOOPA_RBO_AND:
        os << "and t0, t0, t1" << endl;
        break;
    
    case KOOPA_RBO_OR:
        os << "or t0, t0, t1" << endl;
        break;
    
    case KOOPA_RBO_XOR:
        os << "xor t0, t0, t1" << endl;
        break;

    case KOOPA_RBO_GT:
        os << "sgt t0, t0, t1" << endl;
        break;
    
    case KOOPA_RBO_LT:
        os << "slt t0, t0, t1" << endl;
        break;

    case KOOPA_RBO_GE:
        os << "slt t0, t0, t1" << endl;
        os << "seqz t0, t0" << endl;
        break;

    case KOOPA_RBO_LE:
        os << "sgt t0, t0, t1" << endl;
        os << "seqz t0, t0" << endl;
        break;

    default:
        assert(false);
        break;
    }

    // store
    // os << "sw t0, " << stack.offset[value] << "(sp)" << endl;
}