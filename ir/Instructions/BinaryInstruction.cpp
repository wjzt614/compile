///
/// @file BinaryInstruction.cpp
/// @brief 二元操作指令
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
#include "BinaryInstruction.h"

/// @brief 构造函数
/// @param _op 操作符
/// @param _result 结果操作数
/// @param _srcVal1 源操作数1
/// @param _srcVal2 源操作数2
BinaryInstruction::BinaryInstruction(Function * _func,
                                     IRInstOperator _op,
                                     Value * _srcVal1,
                                     Value * _srcVal2,
                                     Type * _type)
    : Instruction(_func, _op, _type)
{
    addOperand(_srcVal1);
    addOperand(_srcVal2);
}

/// @brief 转换成字符串
/// @param str 转换后的字符串
void BinaryInstruction::toString(std::string & str)
{
    Value *src1 = getOperand(0), *src2 = getOperand(1);

    switch (op) {
        case IRInstOperator::IRINST_OP_ADD_I:
            // 加法指令，二元运算
            str = getIRName() + " = add " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_ADD_F:
            // 浮点加法指令，二元运算
            str = getIRName() + " = fadd " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_SUB_I:
            // 减法指令，二元运算
            str = getIRName() + " = sub " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_SUB_F:
            // 浮点减法指令，二元运算
            str = getIRName() + " = fsub " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_MUL_I:
            // 乘法指令，二元运算
            str = getIRName() + " = mul " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_MUL_F:
            // 浮点乘法指令，二元运算
            str = getIRName() + " = fmul " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_DIV_I:
            // 除法指令，二元运算
            str = getIRName() + " = div " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_DIV_F:
            // 浮点数除法指令，二元运算
            str = getIRName() + " = fdiv " + src1->getIRName() + "," + src2->getIRName();
            break;

        case IRInstOperator::IRINST_OP_MOD_I:
            // 取模指令，二元运算
            str = getIRName() + " = mod " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_XOR_I:
            // 异或指令，二元运算
            str = getIRName() + " = xor " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_EQ_I:
            // 等于指令，二元运算
            str = getIRName() + " = icmp eq " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_NE_I:
            // 不等于指令，二元运算
            str = getIRName() + " = icmp ne " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_LT_I:
            // 小于指令，二元运算
            str = getIRName() + " = icmp lt " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_LE_I:
            // 小于等于指令，二元运算
            str = getIRName() + " = icmp le " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_GT_I:
            // 大于指令，二元运算
            str = getIRName() + " = icmp gt " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_GE_I:
            // 大于等于指令，二元运算
            str = getIRName() + " = icmp ge " + src1->getIRName() + "," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_GEP:
            // 取数组元素指令，二元运算
            str = getIRName() + " = getelementptr "+ getType()->toString() + ", " + src1->getIRName() + ", 0," + src2->getIRName();
            break;
        case IRInstOperator::IRINST_OP_GEP_FormalParam:
            // 取数组元素指令，二元运算
            str = getIRName() + " = getelementptr "+ getType()->toString() + ", " + src1->getIRName() + ", 0," + src2->getIRName();
            break;
        default:
            // 未知指令
            Instruction::toString(str);
            break;
    }
}
