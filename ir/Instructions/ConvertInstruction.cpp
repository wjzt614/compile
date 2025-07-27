///
/// @file ConvertInstruction.cpp
/// @brief 类型转换指令实现
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

#include "ConvertInstruction.h"

/// @brief 构造函数
/// @param _func 所属函数
/// @param _dest 目标操作数
/// @param _src 源操作数
ConvertInstruction::ConvertInstruction(Function* _func, Value* _dest, Value* _src)
    : Instruction(_func, IRInstOperator::IRINST_OP_FTOI, _dest->getType()), src(_src), dest(_dest)
{
    // 设置临时变量的名字
    if (_dest) {
        this->setName(_dest->getName());
    }
    
    // 添加操作数
    addOperand(src);
}

ConvertInstruction::ConvertInstruction(Function* _func, IRInstOperator op, Value* _dest, Value* _src)
    : Instruction(_func, op, _dest->getType()), src(_src), dest(_dest)
{
    // 设置临时变量的名字
    if (_dest) {
    addOperand(src);
}
}


/// @brief 将指令转换成IR文本
/// @param str IR文本
void ConvertInstruction::toString(std::string& str)
{
    // 确保temp变量有名称
    if (getName().empty() && !getIRName().empty()) {
        str.append(getIRName());
    } else if (!getName().empty()) {
        str.append(getName());
    } else {
        // 如果没有名称，使用%t加数字作为临时名称
        str.append("%t").append(std::to_string(this->getRegId()));
    }
    
    // 源操作数名称
    std::string srcName = "";
    if (src) {
        if (!src->getIRName().empty()) {
            srcName = src->getIRName(); // 优先使用IR名称
        } else {
            // 如果没有IR名称，使用%l加寄存器ID
            srcName = "%l" + std::to_string(src->getRegId());
        }
    }
    
    // 生成转换指令
    if (getOp() == IRInstOperator::IRINST_OP_FTOI) {
        str.append(" = ftoi ").append(srcName);
    } else if (getOp() == IRInstOperator::IRINST_OP_ITOF) {
        str.append(" = itof ").append(srcName);

    }
}

/// @brief 根据源操作数和目标操作数类型，创建相应的类型转换指令
/// @param _func 所属函数
/// @param _dest 目标变量
/// @param _src 源操作数
/// @return ConvertInstruction* 创建的类型转换指令
ConvertInstruction* ConvertInstruction::createConvertInst(Function* _func, Value* _dest, Value* _src)
{
    // 目前只支持浮点到整型的转换
    if (_src->getType()->isFloatType() && _dest->getType()->isIntegerType()) {
        return new ConvertInstruction(_func, IRInstOperator::IRINST_OP_FTOI, _dest, _src);
    }
    // 整型到浮点的转换
    else if (_src->getType()->isIntegerType() && _dest->getType()->isFloatType()) {
        return new ConvertInstruction(_func, IRInstOperator::IRINST_OP_ITOF, _dest, _src);
    }
    // 不支持的类型转换
    return nullptr;
} 