///
/// @file GotoInstruction.h
/// @brief 无条件跳转指令即goto指令
///
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-09-29
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// </table>
///
#pragma once

#include <string>

#include "Instruction.h"
#include "LabelInstruction.h"
#include "Function.h"

///
/// @brief 跳转指令
///
class GotoInstruction final : public Instruction {

public:

    //前一条比较指令的类型，用于后端生成指令
    IRInstOperator fore_cmp_inst_op;
    //前一条比较指令，用于后端生成指令
    int32_t fore_instId;
    Instruction * fore_inst;
    ///
    /// @brief 无条件跳转指令的构造函数
    /// @param target 跳转目标
    ///
    GotoInstruction(Function * _func, Instruction * _target);

    ///
    /// @brief 条件跳转指令的构造函数
    /// @param target 跳转目标
    /// @param cond 条件值
    ///
    GotoInstruction(Function * _func, Instruction * _target, Value * _cond);

    ///
    /// @brief 条件跳转指令的构造函数
    /// @param target 跳转目标
    /// @param trueTarget 条件为真时的跳转目标
    /// @param falseTarget 条件为假时的跳转目标
    /// @param cond 条件值
    ///
    GotoInstruction(Function * _func, Instruction * _trueTarget, Instruction * _falseTarget, Value * _cond);

    /// @brief 转换成字符串
    void toString(std::string & str) override;

    ///
    /// @brief 获取目标Label指令
    /// @return LabelInstruction*
    ///
    [[nodiscard]] LabelInstruction * getTarget() const;

    ///
    /// @brief 获取条件为真时的跳转目标Label指令
    /// @return LabelInstruction*
    ///
    [[nodiscard]] LabelInstruction * getTrueTarget() const;

    ///
    /// @brief 获取条件为假时的跳转目标Label指令
    /// @return LabelInstruction*
    ///
    [[nodiscard]] LabelInstruction * getFalseTarget() const;

    ///
    /// @brief 获取条件值
    /// @return Value*
    ///
    [[nodiscard]] Value * getCondition() const;

private:
    ///
    /// @brief 跳转到的目标Label指令
    ///
    LabelInstruction * target = nullptr;

    ///
    /// @brief 条件为真时的跳转目标Label指令
    ///
    LabelInstruction * trueTarget = nullptr;

    ///
    /// @brief 条件为假时的跳转目标Label指令
    ///
    LabelInstruction * falseTarget = nullptr;

    ///
    /// @brief 条件值，如果为nullptr则表示无条件跳转
    ///
    Value * condition = nullptr;
};
