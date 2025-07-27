///
/// @file ILocArm64.cpp
/// @brief 指令序列管理的实现，ILOC的全称为Intermediate Language for Optimizing Compilers
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-11-21
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-11-21 <td>1.0     <td>zenglj  <td>新做
/// <tr><td>2025-06-07 <td>1.1     <td>DeepSeek  <td>适配ARMv8-A架构
/// </table>
///
#include <cstdio>
#include <string>

#include "ILocArm64.h"
#include "Common.h"
#include "Function.h"
#include "PlatformArm64.h"
#include "Module.h"
#include "ConstFloat.h"
#include "ConstInt.h"

ArmInstArm64::ArmInstArm64(std::string _opcode,
                 std::string _result,
                 std::string _arg1,
                 std::string _arg2,
                 std::string _cond,
                 std::string _addition)
    : opcode(_opcode), cond(_cond), result(_result), arg1(_arg1), arg2(_arg2), addition(_addition), dead(false)
{}

/*
    指令内容替换
*/
void ArmInstArm64::replace(std::string _opcode,
                      std::string _result,
                      std::string _arg1,
                      std::string _arg2,
                      std::string _cond,
                      std::string _addition)
{
    opcode = _opcode;
    result = _result;
    arg1 = _arg1;
    arg2 = _arg2;
    cond = _cond;
    addition = _addition;

#if 0
    // 空操作，则设置为dead
    if (op == "") {
        dead = true;
    }
#endif
}

/*
    设置为无效指令
*/
void ArmInstArm64::setDead()
{
    dead = true;
}

/*
    输出函数
*/
std::string ArmInstArm64::outPut()
{
    // 无用代码，什么都不输出
    if (dead) {
        return "";
    }

    // 占位指令,可能需要输出一个空操作，看是否支持 FIXME
    if (opcode.empty()) {
        return "";
    }

    std::string ret = opcode;

    if (!cond.empty()) {
        ret += cond;
    }

    // 结果输出
    if (!result.empty()) {
        if (result == ":") {
            ret += result;
        } else {
            ret += " " + result;
        }
    }

    // 第一元参数输出
    if (!arg1.empty()) {
        ret += "," + arg1;
    }

    // 第二元参数输出
    if (!arg2.empty()) {
        ret += "," + arg2;
    }

    // 其他附加信息输出
    if (!addition.empty()) {
        ret += "," + addition;
    }

    return ret;
}

#define emit(...) code.push_back(new ArmInstArm64(__VA_ARGS__))

/// @brief 构造函数
/// @param _module 符号表
ILocArm64::ILocArm64(Module * _module)
{
    this->module = _module;
}

/// @brief 析构函数
ILocArm64::~ILocArm64()
{
    std::list<ArmInstArm64 *>::iterator pIter;

    for (pIter = code.begin(); pIter != code.end(); ++pIter) {
        delete (*pIter);
    }
}

/// @brief 删除无用的Label指令
void ILocArm64::deleteUsedLabel()
{
    std::list<ArmInstArm64 *> labelInsts;
    for (ArmInstArm64 * arm: code) {
        if ((!arm->dead) && (arm->opcode[0] == '.') && (arm->result == ":")) {
            labelInsts.push_back(arm);
        }
    }

    for (ArmInstArm64 * labelArm: labelInsts) {
        bool labelUsed = false;

        for (ArmInstArm64 * arm: code) {
            if ((!arm->dead) && (arm->opcode[0] == 'b') && (arm->result == labelArm->opcode)) {
                labelUsed = true;
                break;
            }
        }

        if (!labelUsed) {
            labelArm->setDead();
        }
    }
}

/// @brief 输出汇编
/// @param file 输出的文件指针
/// @param outputEmpty 是否输出空语句
void ILocArm64::outPut(FILE * file, bool outputEmpty)
{
    for (auto arm: code) {

        std::string s = arm->outPut();

        if (arm->result == ":") {
            // Label指令，不需要Tab输出
            fprintf(file, "%s\n", s.c_str());
            continue;
        }

        if (!s.empty()) {
            fprintf(file, "\t%s\n", s.c_str());
        } else if ((outputEmpty)) {
            fprintf(file, "\n");
        }
    }
}

/// @brief 获取当前的代码序列
/// @return 代码序列
std::list<ArmInstArm64 *> & ILocArm64::getCode()
{
    return code;
}

/**
 * 数字变字符串，若flag为真，则变为立即数寻址（加#）
 */
std::string ILocArm64::toStr(int num, bool flag)
{
    std::string ret;

    if (flag) {
        ret = "#";
    }

    ret += std::to_string(num);

    return ret;
}

/*
    产生标签
*/
void ILocArm64::label(std::string name)
{
    // .L1:
    emit(name, ":");
}

/// @brief 0个源操作数指令
/// @param op 操作码
/// @param rs 操作数
void ILocArm64::inst(std::string op, std::string rs)
{
    emit(op, rs);
}

/// @brief 一个操作数指令
/// @param op 操作码
/// @param rs 操作数
/// @param arg1 源操作数
void ILocArm64::inst(std::string op, std::string rs, std::string arg1)
{
    emit(op, rs, arg1);
}

/// @brief 一个操作数指令
/// @param op 操作码
/// @param rs 操作数
/// @param arg1 源操作数
/// @param arg2 源操作数
void ILocArm64::inst(std::string op, std::string rs, std::string arg1, std::string arg2)
{
    emit(op, rs, arg1, arg2);
}

///
/// @brief 注释指令，不包含分号
///
void ILocArm64::comment(std::string str)
{
    emit("@", str);
}

/*
    加载立即数 ldr x0,=#100
*/
void ILocArm64::load_imm(int rs_reg_no, int constant)
{
    // ARMv8 使用 movz/movk 加载立即数
    if (constant >= 0 && constant <= 0xFFFF) {
        // 16位立即数
        emit("mov", PlatformArm64::regName[rs_reg_no], "#" + std::to_string(constant));
    } else{//else if (constant > 0xFFFF && constant <= 0xFFFFFFFF) 
        // 32位立即数需要分两次加载
        emit("mov", PlatformArm64::regName[rs_reg_no], "#" + std::to_string(constant & 0xFFFF));
        emit("movk", PlatformArm64::regName[rs_reg_no], "#" + std::to_string((constant >> 16) & 0xFFFF) + ", lsl #16");
    } /*else {
        // 64位立即数（这里只处理32位，但保留扩展性）
        emit("mov", PlatformArm64::regName[rs_reg_no], "#" + std::to_string(constant & 0xFFFF));
        emit("movk", PlatformArm64::regName[rs_reg_no], "#" + std::to_string((constant >> 16) & 0xFFFF) + ", lsl #16");
        emit("movk", PlatformArm64::regName[rs_reg_no], "#" + std::to_string((constant >> 32) & 0xFFFF) + ", lsl #32");
        emit("movk", PlatformArm64::regName[rs_reg_no], "#" + std::to_string((constant >> 48) & 0xFFFF) + ", lsl #48");
    }*/
}

/// @brief 加载符号值 ldr x0,=g ldr x0,=.L1
/// @param rs_reg_no 结果寄存器编号
/// @param name 符号名
void ILocArm64::load_symbol(int rs_reg_no, std::string name)
{
    // ARMv8 使用 adrp/add 组合加载全局地址
    emit("adrp", PlatformArm64::regName[rs_reg_no], name);
    emit("add", PlatformArm64::regName[rs_reg_no], PlatformArm64::regName[rs_reg_no], 
         "#:lo12:" + name);
}

/// @brief 基址寻址 ldr x0,[x29,#100]
/// @param rsReg 结果寄存器
/// @param base_reg_no 基址寄存器
/// @param offset 偏移
void ILocArm64::load_base(int rs_reg_no, int base_reg_no, int offset)
{
    std::string rsReg = PlatformArm64::regName[rs_reg_no];
    std::string base = PlatformArm64::regName[base_reg_no];

    // ARMv8 偏移范围是 ±4095
    if (PlatformArm64::isDisp(offset)) {
        if (offset == 0) {
            // ldr x8, [x29]
            emit("ldr", rsReg, "[" + base + "]");
        } else {
            // ldr x8, [x29, #16]
            emit("ldr", rsReg, "[" + base + ", #" + std::to_string(offset) + "]");
        }
    } else {
        // 大偏移需要先加载到寄存器
        load_imm(Arm64_TMP_REG_NO, offset);
        // ldr x8, [x29, x10]
        emit("ldr", rsReg, "[" + base + ", " + PlatformArm64::regName[Arm64_TMP_REG_NO] + "]");
    }
}

/// @brief 基址寻址 str x0,[x29,#100]
/// @param srcReg 源寄存器
/// @param base_reg_no 基址寄存器
/// @param disp 偏移
/// @param tmp_reg_no 可能需要临时寄存器编号
void ILocArm64::store_base(int src_reg_no, int base_reg_no, int disp, int tmp_reg_no)
{
    std::string srcReg = PlatformArm64::regName[src_reg_no];
    std::string base = PlatformArm64::regName[base_reg_no];

    if (PlatformArm64::isDisp(disp)) {
        if (disp == 0) {
            // str x8, [x29]
            emit("str", srcReg, "[" + base + "]");
        } else {
            // str x8, [x29, #16]
            emit("str", srcReg, "[" + base + ", #" + std::to_string(disp) + "]");
        }
    } else {
        // 大偏移需要先加载到寄存器
        load_imm(tmp_reg_no, disp);
        // str x8, [x29, x10]
        emit("str", srcReg, "[" + base + ", " + PlatformArm64::regName[tmp_reg_no] + "]");
    }
}

/// @brief 寄存器Mov操作
/// @param rs_reg_no 结果寄存器
/// @param src_reg_no 源寄存器
void ILocArm64::mov_reg(int rs_reg_no, int src_reg_no)
{
    emit("mov", PlatformArm64::regName[rs_reg_no], PlatformArm64::regName[src_reg_no]);
}

/// @brief 加载变量到寄存器，保证将变量放到reg中
/// @param rs_reg_no 结果寄存器
/// @param src_var 源操作数
void ILocArm64::load_var(int rs_reg_no, Value * src_var)
{
    if (Instanceof(constVal, ConstInt *, src_var)) {
        // 整型常量
        load_imm(rs_reg_no, constVal->getVal());
    } else if (Instanceof(constFloat, ConstFloat *, src_var)) {
        // 浮点常量
        // 暂时使用整数寄存器加载浮点常量的位模式
        // 这里需要进一步完善浮点数的处理
        float fval = constFloat->getVal();
        int32_t ival = *reinterpret_cast<int32_t*>(&fval);
        load_imm(rs_reg_no, ival);
    } else if (src_var->getRegId() != -1) {
        // 源操作数为寄存器变量
        int32_t src_regId = src_var->getRegId();
        if (src_regId != rs_reg_no) {
            mov_reg(rs_reg_no, src_regId);
        }
    } else if (Instanceof(globalVar, GlobalVariable *, src_var)) {
        // 全局变量
        load_symbol(rs_reg_no, globalVar->getName());
        // ldr x8, [x8]
        if(!src_var->getType()->isArrayType()){// 非数组类型
            // 直接加载全局变量的值到寄存器
        emit("ldr", PlatformArm64::regName[rs_reg_no], 
             "[" + PlatformArm64::regName[rs_reg_no] + "]");
        }
    } else {
        // 栈+偏移的寻址方式
        int32_t var_baseRegId = -1;
        int64_t var_offset = -1;
        bool result = src_var->getMemoryAddr(&var_baseRegId, &var_offset);
        if (!result) {
            minic_log(LOG_ERROR, "BUG");
        }
        load_base(rs_reg_no, var_baseRegId, var_offset);
    }
}

/// @brief 加载变量地址到寄存器
/// @param rs_reg_no
/// @param var
void ILocArm64::lea_var(int rs_reg_no, Value * var)
{
    // 被加载的变量肯定不是常量！
    // 被加载的变量肯定不是寄存器变量！

    if (Instanceof(globalVar, GlobalVariable *, var)) {
        // 全局变量地址
        load_symbol(rs_reg_no, globalVar->getName());
    } else {
        // 局部变量地址
        int32_t var_baseRegId = -1;
        int64_t var_offset = -1;
        bool result = var->getMemoryAddr(&var_baseRegId, &var_offset);
        if (!result) {
            minic_log(LOG_ERROR, "BUG");
        }
        leaStack(rs_reg_no, var_baseRegId, var_offset);
    }
}

/// @brief 保存寄存器到变量，保证将计算结果（x8）保存到变量
/// @param src_reg_no 源寄存器
/// @param dest_var  变量
/// @param tmp_reg_no 第三方寄存器
void ILocArm64::store_var(int src_reg_no, Value * dest_var, int tmp_reg_no)
{
    // 被保存目标变量肯定不是常量

    if (dest_var->getRegId() != -1) {
        // 寄存器变量
        int dest_reg_id = dest_var->getRegId();
        if (src_reg_no != dest_reg_id) {
            mov_reg(dest_reg_id, src_reg_no);
        }
    } else if (Instanceof(globalVar, GlobalVariable *, dest_var)) {
        // 全局变量
        load_symbol(tmp_reg_no, globalVar->getName());
        // str x8, [x10]
        emit("str", PlatformArm64::regName[src_reg_no], 
             "[" + PlatformArm64::regName[tmp_reg_no] + "]");
    } else {
        // 局部变量
        int32_t dest_baseRegId = -1;
        int64_t dest_offset = -1;
        bool result = dest_var->getMemoryAddr(&dest_baseRegId, &dest_offset);
        if (!result) {
            minic_log(LOG_ERROR, "BUG");
        }
        store_base(src_reg_no, dest_baseRegId, dest_offset, tmp_reg_no);
    }
}

/// @brief 加载栈内变量地址
/// @param rsReg 结果寄存器号
/// @param base_reg_no 基址寄存器
/// @param off 偏移
void ILocArm64::leaStack(int rs_reg_no, int base_reg_no, int off)
{
    std::string rs_reg_name = PlatformArm64::regName[rs_reg_no];
    std::string base_reg_name = PlatformArm64::regName[base_reg_no];

    if (off == 0) {
        // add x8, x29, #0 可以直接 mov
        mov_reg(rs_reg_no, base_reg_no);
    } else if (PlatformArm64::isDisp(off)) {
        // add x8, x29, #16
        emit("add", rs_reg_name, base_reg_name, "#" + std::to_string(off));
    } else {
        // 大偏移
        load_imm(rs_reg_no, off);
        // add x8, x29, x8
        emit("add", rs_reg_name, base_reg_name, rs_reg_name);
    }
}

/// @brief 函数内栈内空间分配（局部变量、形参变量、函数参数传值，或不能寄存器分配的临时变量等）
/// @param func 函数
/// @param tmp_reg_No
void ILocArm64::allocStack(Function * func, int tmp_reg_no)
{
    // 超过8个的函数调用参数个数，多余8个，则需要栈传值
    int funcCallArgCnt = func->getMaxFuncCallArgCnt() - 8;
    if (funcCallArgCnt < 0) {
        funcCallArgCnt = 0;
    }
    if (funcCallArgCnt > 0) {
        funcCallArgCnt = funcCallArgCnt * 8; // 每个参数8字节
        funcCallArgCnt = (funcCallArgCnt + 15) & ~15;
    }
    // ARMv8 栈帧分配
    int stackSize = func->getMaxDep();
    
    stackSize += funcCallArgCnt ; // 函数调用参数空间
    // 确保16字节对齐
    if (stackSize % 16 != 0) {
        stackSize = (stackSize + 15) & ~15;
    }

    if (stackSize == 0) {
        return;
    }

    // 分配栈空间
    if (stackSize <= 4095) {
        emit("sub", "sp", "sp", "#" + std::to_string(stackSize));
    } else {
        // 大栈帧分配
        load_imm(tmp_reg_no, stackSize);
        emit("sub", "sp", "sp", PlatformArm64::regName[tmp_reg_no]);
    }
    // 设置栈帧基址寄存器
    //inst("add", PlatformArm64::regName[Arm64_FP_REG_NO], "sp", toStr(stackSize-funcCallArgCnt * 8));
}

/// @brief 调用函数fun
/// @param fun
void ILocArm64::call_fun(std::string name)
{
    // 函数调用
    emit("bl", name);
}

/// @brief NOP操作
void ILocArm64::nop()
{
    emit("nop");
}

///
/// @brief 无条件跳转指令
/// @param label 目标Label名称
///
void ILocArm64::jump(std::string label)
{
    emit("b", label);
}

/// @brief 仅操作码的指令
/// @param op 操作码
void ILocArm64::inst(std::string op)
{
    emit(op, "");
}