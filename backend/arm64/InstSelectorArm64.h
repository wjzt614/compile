///
/// @file InstSelectorArm64.h
/// @brief 指令选择器-Arm64
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
/// </table>
///
#pragma once

#include <map>
#include <vector>

#include "Function.h"
#include "ILocArm64.h"
#include "Instruction.h"
#include "PlatformArm64.h"
#include "SimpleRegisterAllocatorArm64.h"
#include "RegVariable.h"

using namespace std;

/// @brief 指令选择器-Arm64
class InstSelectorArm64 {

    /// @brief 所有的IR指令
    std::vector<Instruction *> & ir;

    /// @brief 指令变换
    ILocArm64 & iloc;

    /// @brief 要处理的函数
    Function * func;

    /// @brief 形参列表
    std::vector<Value *> realArgs;

protected:
    /// @brief 指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate(Instruction * inst);

    /// @brief NOP翻译成Arm64汇编
    /// @param inst IR指令
    void translate_nop(Instruction * inst);

    /// @brief 函数入口指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_entry(Instruction * inst);

    /// @brief 函数出口指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_exit(Instruction * inst);

    /// @brief 赋值指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_assign(Instruction * inst);

    /// @brief Label指令指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_label(Instruction * inst);

    /// @brief goto指令指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_goto(Instruction * inst);

    /// @brief 整数加法指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_add_int32(Instruction * inst);
    
    /// @brief 浮点数加法指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_add_float(Instruction * inst);

    /// @brief 整数减法指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_sub_int32(Instruction * inst);
    
    /// @brief 浮点数减法指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_sub_float(Instruction * inst);

    /// @brief 整数乘法指令翻译成ARM32汇编
    /// @param inst IR指令
    void translate_mul_int32(Instruction * inst);
    
    /// @brief 浮点数乘法指令翻译成ARM32汇编
    /// @param inst IR指令
    void translate_mul_float(Instruction * inst);

    /// @brief 整数除法指令翻译成ARM32汇编
    /// @param inst IR指令
    void translate_div_int32(Instruction * inst);
    
    /// @brief 浮点数除法指令翻译成ARM32汇编
    /// @param inst IR指令
    void translate_div_float(Instruction * inst);

    /// @brief 整数取模指令翻译成ARM32汇编
    /// @param inst IR指令
    void translate_mod_int32(Instruction * inst);

    /// @brief 异或指令翻译成ARM32汇编
    /// @param inst IR指令
    void translate_xor_int32(Instruction * inst);
    /// @brief 比较指令翻译成ARM32汇编
    /// @param inst IR指令
    void translate_cmp_int32(Instruction * inst);
    
    /// @brief 整数到浮点数转换指令翻译成ARM32汇编
    /// @param inst IR指令
    void translate_itof(Instruction * inst);

    /// @brief neg操作指令翻译成ARM32汇编（整数）
    /// @param inst IR指令
    /// @param operator_name 操作码
    /// @param rs_reg_no 结果寄存器号
    /// @param op1_reg_no 源操作数1寄存器号
    void translate_neg_operator(Instruction * inst);
    
    /// @brief neg操作指令翻译成ARM32汇编（浮点数）
    /// @param inst IR指令
    void translate_neg_float_operator(Instruction * inst);

    /// @brief 二元操作指令翻译成Arm64汇编
    /// @param inst IR指令
    /// @param operator_name 操作码
    void translate_two_operator(Instruction * inst, string operator_name);
    
    // 数组
    void translate_gep(Instruction *inst);
    /// @brief 函数调用指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_gep_formalparam(Instruction *inst);
    void translate_store(Instruction *inst);
    void translate_load(Instruction *inst);

    /// @brief 函数调用指令翻译成Arm64汇编
    /// @param inst IR指令
    void translate_call(Instruction * inst);

    ///
    /// @brief 实参指令翻译成Arm64汇编
    /// @param inst
    ///
    void translate_arg(Instruction * inst);

    ///
    /// @brief 输出IR指令
    ///
    void outputIRInstruction(Instruction * inst);

    /// @brief IR翻译动作函数原型
    typedef void (InstSelectorArm64::*translate_handler)(Instruction *);

    /// @brief IR动作处理函数清单
    map<IRInstOperator, translate_handler> translator_handlers;

    ///
    /// @brief 简单的朴素寄存器分配方法
    ///
    SimpleRegisterAllocatorArm64 & simpleRegisterAllocator;

    ///
    /// @brief 函数实参累计
    ///
    int32_t argCount = 0;

    /// @brief 累计的实参个数
    int32_t realArgCount = 0;

    ///
    /// @brief 显示IR指令内容
    ///
    bool showLinearIR = false;

public:
    /// @brief 构造函数
    /// @param _irCode IR指令
    /// @param _func 函数
    /// @param _iloc 后端指令
    InstSelectorArm64(std::vector<Instruction *> & _irCode,
                      ILocArm64 & _iloc,
                      Function * _func,
                      SimpleRegisterAllocatorArm64 & allocator);

    ///
    /// @brief 析构函数
    ///
    ~InstSelectorArm64();

    ///
    /// @brief 设置是否输出线性IR的内容
    /// @param show true显示，false显示
    ///
    void setShowLinearIR(bool show)
    {
        showLinearIR = show;
    }

    /// @brief 指令选择
    void run();
};
