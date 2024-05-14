#include <inc/riscv.hpp>
#include <inc/ST.hpp>

map<koopa_raw_value_t, string> registers;
Stack rstack; // 记录该变量在栈中相对栈指针的偏移量

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
    return Stack_size(func->bbs);
}

// 计算基本块需要的栈空间
int Stack_size(const koopa_raw_basic_block_t &bb)
{
    return Stack_size(bb->insts);
}

// 计算指令需要的栈空间，并为该条指令在栈上分配一个位置
int Stack_size(const koopa_raw_value_t &value)
{   
    // 不需要栈空间的情形
    if (value->kind.tag == KOOPA_RVT_RETURN || value->kind.tag == KOOPA_RVT_INTEGER || value->kind.tag == KOOPA_RVT_STORE)
        return 0;
    // ad-hoc solution: size = 4;
    rstack.offset[value] = rstack.size;
    rstack.size += 4;
    return 4;
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

// 访问函数
void DumpRISC(const koopa_raw_function_t &func, ostream &os)
{
    os << ".globl " << 1 + func->name << endl;
    os << 1 + func->name << ":" << endl;

    // 计算函数所需栈的大小
    int stack_size = Stack_size(func);
    stack_size = 16 * ((rstack.size + 15) / 16); // 16字节对齐
    
    // 使用 addi
    if (stack_size < 2048)
        os << "addi sp, sp, " << -stack_size << endl;
    
    // 使用 add
    else 
    {
        os << "li t0, " << -stack_size << endl;
        os << "add sp, sp, t0" << endl;
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
    int offset;
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
            offset = rstack.offset[value];
            if (offset < 2048)
                os << "sw t0, " << rstack.offset[value] << "(sp)" << endl;
            
            else {
                os << "li t1, " << offset << endl;
                os << "add t1, t1, sp" << endl;
                os << "sw t0, 0(t1)" << endl;
            }
            os << endl;
            break;
        case KOOPA_RVT_LOAD: 
            DumpRISC(kind.data.load, os);
            offset = rstack.offset[value];
            if (offset < 2048)
                os << "sw t0, " << rstack.offset[value] << "(sp)" << endl;
            
            else {
                os << "li t1, " << offset << endl;
                os << "add t1, t1, sp" << endl;
                os << "sw t0, 0(t1)" << endl;
            }
            os << endl;
            break;
        case KOOPA_RVT_STORE: 
            DumpRISC(kind.data.store, os);
            os << endl;
            break;
        case KOOPA_RVT_ALLOC:
            DumpRISC(kind.data.global_alloc, os);
            break;
        case KOOPA_RVT_BRANCH: 
            DumpRISC(kind.data.branch, os);
            os << endl;
            break;
        case KOOPA_RVT_JUMP:
            DumpRISC(kind.data.jump, os);
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
    if (ret.value->kind.tag == KOOPA_RVT_INTEGER)
        os << "li a0, " << ret.value->kind.data.integer.value << endl;
    else 
    {
        int offset = rstack.offset[ret.value];
        if (offset < 2048)
            os << "lw a0, " << offset << "(sp)" << endl;
        else 
        {
            os << "li t1, " << offset << endl;
            os << "add t1, t1, sp" << endl;
            os << "lw a0, 0(t1)" << endl;
        }
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

// alloc
void DumpRISC(const koopa_raw_global_alloc_t &alloc, ostream &os)
{
    // do nothing.
    return;
}

// load
void DumpRISC(const koopa_raw_load_t &load, ostream &os)
{
    int offset = rstack.offset[load.src];
    if (offset < 2048)
        os << "lw t0, " << rstack.offset[load.src] << "(sp)" << endl;
    
    else {
        os << "li t1, " << offset << endl;
        os << "add t1, t1, sp" << endl;
        os << "lw t0, 0(t1)" << endl;
    }
    // os << "sw t0, " << stack.offset[value] << endl;
}

// store value, dest
void DumpRISC(const koopa_raw_store_t &store, ostream &os)
{
    Load_addr_dump(store.value, "t0", os);
    int offset = rstack.offset[store.dest];
    if (offset < 2048)
        os << "sw t0, " << rstack.offset[store.dest] << "(sp)" << endl;
    
    else {
        os << "li t1, " << offset << endl;
        os << "add t1, t1, sp" << endl;
        os << "sw t0, 0(t1)" << endl;
    }
}

// branch
void DumpRISC(const koopa_raw_branch_t &branch, ostream &os)
{
    Load_addr_dump(branch.cond, "t0", os);
    os << "bnez t0, " << branch.true_bb->name + 1 << endl;
    os << "j " << branch.false_bb->name + 1 << endl;
}

// jump
void DumpRISC(const koopa_raw_jump_t &jump, ostream &os)
{
    os << "j " << jump.target->name + 1 << endl;
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

    // address
    else 
    {
        int offset = rstack.offset[value];
        if (offset < 2048)
            os << "lw " << reg << ", " << offset << "(sp)" << endl;
        else 
        {
            os << "li t1, " << offset << endl;
            os << "add t1, t1, sp" << endl;
            os << "lw " << reg << ", 0(t1)" << endl;
        }
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