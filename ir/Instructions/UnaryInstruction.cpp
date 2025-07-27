#include "UnaryInstruction.h"

/// @brief 构造函数
/// @param _func 所属函数
/// @param _op 操作符
/// @param _srcVal 源操作数
/// @param _type 类型
UnaryInstruction::UnaryInstruction(Function * _func, IRInstOperator _op, Value * _srcVal, Type * _type)
    : Instruction(_func, _op, _type)
{
    addOperand(_srcVal);
}

/// @brief 转换成字符串
/// @param str 转换后的字符串
void UnaryInstruction::toString(std::string & str)
{
    Value * src = getOperand(0);

    switch (op) {
        case IRInstOperator::IRINST_OP_NEG_I:
            // 求负指令，单目运算
            str = getIRName() + " = neg " + src->getIRName();
            break;
        case IRInstOperator::IRINST_OP_NEG_F:
            // 浮点数求负指令，单目运算
            str = getIRName() + " = fneg " + src->getIRName();
            break;

        case IRInstOperator::IRINST_OP_LOGICAL_NOT_I:
            // 逻辑非指令，单目运算，转换为与0比较
            str = getIRName() + " = icmp eq " + src->getIRName() + ",0";
            break;
        default:
            // 未知指令
            Instruction::toString(str);
            break;
    }
}