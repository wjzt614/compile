///
/// @file MoveInstruction.cpp
/// @brief Move指令，也就是DragonIR的Asssign指令
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

#include "MoveInstruction.h"

///
/// @brief 构造函数
/// @param _func 所属的函数
/// @param result 结构操作数
/// @param srcVal1 源操作数
/// @param _isStore 是否是存储操作（针对数组元素赋值）
///
MoveInstruction::MoveInstruction(Function * _func, Value * _result, Value * _srcVal1, bool _isStore)
    : Instruction(_func, IRInstOperator::IRINST_OP_ASSIGN, VoidType::getType()), opType(_isStore ? 1 : 0)
{
    addOperand(_result);
    addOperand(_srcVal1);
}

///
/// @brief 构造函数（支持加载操作）
/// @param _func 所属的函数
/// @param result 结果操作数
/// @param srcVal1 源操作数
/// @param _opType 操作类型：0-普通赋值，1-存储操作，2-加载操作
///
MoveInstruction::MoveInstruction(Function * _func, Value * _result, Value * _srcVal1, int _opType)
    : Instruction(_func, IRInstOperator::IRINST_OP_ASSIGN, VoidType::getType()), opType(_opType)
{
    addOperand(_result);
    addOperand(_srcVal1);
}

/// @brief 转换成字符串显示
/// @param str 转换后的字符串
void MoveInstruction::toString(std::string & str)
{
    Value *dstVal = getOperand(0), *srcVal = getOperand(1);

    if (opType == 1) {
        // 存储操作：*dst = src
        str = "*" + dstVal->getIRName() + " = " + srcVal->getIRName();
    } else if (opType == 2) {
        // 加载操作：dst = *src
        str = dstVal->getIRName() + " = *" + srcVal->getIRName();
    } else if (opType == 3) {
        // 存储地址操作：dst = &src
        str = dstVal->getIRName() + " = &" + srcVal->getIRName();
    }else {
        // 普通赋值：dst = src
        str = dstVal->getIRName() + " = " + srcVal->getIRName();
    }
}
