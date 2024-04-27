#include <inc/riscv.hpp>

map<koopa_raw_value_t, string> registers;
map<string, int> sym_table; // 符号表

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
    assert(value_num <= 14);
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
    DumpRISC(func->bbs, os);
}

// 访问基本块
void DumpRISC(const koopa_raw_basic_block_t &bb, ostream &os)
{
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
            break;
        case KOOPA_RVT_INTEGER:
            // 访问 integer 指令
            DumpRISC(kind.data.integer, os);
            break;
        case KOOPA_RVT_BINARY: 
            DumpRISC(kind.data.binary, registers[value], os);
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
    if (Load_imm(ret.value, reg))
        Load_imm_dump(ret.value, os);
    
    CheckReg(ret.value);
    os << "mv a0, " << registers[ret.value] << endl;
    os << "ret" << endl;
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

// binary
void DumpRISC(const koopa_raw_binary_t &binary, string reg, ostream &os)
{
    // 如果lhs或rhs是立即数，将它们load到寄存器中
    bool lreg_li = Load_imm(binary.lhs, reg);
    bool rreg_li = Load_imm(binary.rhs, next_Reg(reg));

    string lreg = registers[binary.lhs], rreg = registers[binary.rhs];

    if (lreg_li) Load_imm_dump(binary.lhs, os);

    switch (binary.op)
    {
    case KOOPA_RBO_EQ:
        if (rreg_li) Load_imm_dump(binary.rhs, os);

        // xor t0, t0, x0;
        os << "xor " << reg << ", " << lreg << ", " << rreg << endl;

        // seqz t0
        os << "seqz " << reg << ", " << reg << endl;
        break;

    case KOOPA_RBO_ADD:
        // addi
        if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
            os << "addi " << reg << ", "<< lreg << ", " << binary.rhs->kind.data.integer.value << endl;

        // add
        else os << "add " << reg << ", "<< lreg << ", " << rreg << endl;
        break;
    
    case KOOPA_RBO_SUB:
        if (rreg_li) Load_imm_dump(binary.rhs, os);
        os << "sub " << reg << ", "<< lreg << ", " << rreg << endl;
        break;

    case KOOPA_RBO_MUL:
        if (rreg_li) Load_imm_dump(binary.rhs, os);
        os << "mul " << reg << ", "<< lreg << ", " << rreg << endl;
        break;


    case KOOPA_RBO_DIV: 
        if (rreg_li) Load_imm_dump(binary.rhs, os);
        os << "div " << reg << ", "<< lreg << ", " << rreg << endl;
        break;

    case KOOPA_RBO_MOD:
        if (rreg_li) Load_imm_dump(binary.rhs, os);
        os << "rem " << reg << ", "<< lreg << ", " << rreg << endl;
        break;

    case KOOPA_RBO_AND:
        // andi
        if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
            os << "andi " << reg << ", "<< lreg << ", " << binary.rhs->kind.data.integer.value << endl;

        // and
        else os << "and " << reg << ", "<< lreg << ", " << rreg << endl;
        break;
    
    case KOOPA_RBO_OR:
        // ori
        if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
            os << "ori " << reg << ", "<< lreg << ", " << binary.rhs->kind.data.integer.value << endl;

        // or
        else os << "or " << reg << ", "<< lreg << ", " << rreg << endl;
        break;
    
    case KOOPA_RBO_XOR:
        // xori
        if (binary.rhs->kind.tag == KOOPA_RVT_INTEGER)
            os << "xori " << reg << ", "<< lreg << ", " << binary.rhs->kind.data.integer.value << endl;

        // xor
        else os << "xor " << reg << ", "<< lreg << ", " << rreg << endl;
        break;

    case KOOPA_RBO_GT:
        if (rreg_li) Load_imm_dump(binary.rhs, os);
        os << "sgt " << reg << ", "<< lreg << ", " << rreg << endl;
        break;
    
    case KOOPA_RBO_LT:
        if (rreg_li) Load_imm_dump(binary.rhs, os);
        os << "slt " << reg << ", "<< lreg << ", " << rreg << endl;
        break;

    case KOOPA_RBO_GE:
        if (rreg_li) Load_imm_dump(binary.rhs, os);
        os << "slt " << reg << ", "<< lreg << ", " << rreg << endl;
        os << "seqz " << reg << ", " << reg << endl;
        break;

    case KOOPA_RBO_LE:
        if (rreg_li) Load_imm_dump(binary.rhs, os);
        os << "sgt " << reg << ", "<< lreg << ", " << rreg << endl;
        os << "seqz " << reg << ", " << reg << endl;
        break;

    default:
        assert(false);
        break;
    }
}