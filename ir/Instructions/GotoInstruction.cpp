///
/// @file GotoInstruction.cpp
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

#include "VoidType.h"

#include "GotoInstruction.h"

///
/// @brief 无条件跳转指令的构造函数
/// @param target 跳转目标
///
GotoInstruction::GotoInstruction(Function * _func, Instruction * _target)
    : Instruction(_func, IRInstOperator::IRINST_OP_GOTO, VoidType::getType())
{
    // 无条件跳转
    target = static_cast<LabelInstruction *>(_target);
    condition = nullptr;
    trueTarget = nullptr;
    falseTarget = nullptr;
}

///
/// @brief 条件跳转指令的构造函数
/// @param target 跳转目标
/// @param cond 条件值
///
GotoInstruction::GotoInstruction(Function * _func, Instruction * _target, Value * _cond)
    : Instruction(_func, IRInstOperator::IRINST_OP_GOTO, VoidType::getType())
{
    // 单目标条件跳转
    target = static_cast<LabelInstruction *>(_target);
    condition = _cond;
    trueTarget = nullptr;
    falseTarget = nullptr;
}

///
/// @brief 条件跳转指令的构造函数
/// @param trueTarget 真目标
/// @param falseTarget 假目标
/// @param cond 条件值
///
GotoInstruction::GotoInstruction(Function * _func, Instruction * _trueTarget, Instruction * _falseTarget, Value * _cond)
    : Instruction(_func, IRInstOperator::IRINST_OP_GOTO, VoidType::getType())
{
    // 双目标条件跳转
    target = nullptr;
    trueTarget = static_cast<LabelInstruction *>(_trueTarget);
    falseTarget = static_cast<LabelInstruction *>(_falseTarget);
    condition = _cond;
}

/// @brief 转换成IR指令文本
void GotoInstruction::toString(std::string & str)
{
    if (condition) {
        if (trueTarget && falseTarget) {
            // 使用bc指令进行条件跳转，有两个目标标签
            str = "bc " + condition->getIRName() + ", " + trueTarget->getIRName() + ", " + falseTarget->getIRName();
        } else if (target) {
            // 使用bc指令进行条件跳转，只有一个目标标签
            str = "bc " + condition->getIRName() + ", " + target->getIRName();
        }
    } else if (target) {
        // 使用br指令进行无条件跳转
        str = "br " + target->getIRName();
    }
}

///
/// @brief 获取目标Label指令
/// @return LabelInstruction* label指令
///
LabelInstruction * GotoInstruction::getTarget() const
{
    return target;
}

///
/// @brief 获取条件值
/// @return Value* 条件值
///
Value * GotoInstruction::getCondition() const
{
    return condition;
}

///
/// @brief 获取真目标
/// @return LabelInstruction* 真目标
///
LabelInstruction * GotoInstruction::getTrueTarget() const
{
    return trueTarget;
}

///
/// @brief 获取假目标
/// @return LabelInstruction* 假目标
///
LabelInstruction * GotoInstruction::getFalseTarget() const
{
    return falseTarget;
}
