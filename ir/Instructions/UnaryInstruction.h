#ifndef UNARYINSTRUCTION_H
#define UNARYINSTRUCTION_H

#include "Instruction.h"

/// @brief 单目运算指令
class UnaryInstruction : public Instruction {

public:
    /// @brief 构造函数
    /// @param _func 所属函数
    /// @param _op 操作符
    /// @param _srcVal 源操作数
    /// @param _type 类型
    UnaryInstruction(Function * _func, IRInstOperator _op, Value * _srcVal, Type * _type);

    /// @brief 转换成字符串
    void toString(std::string & str) override;
};

#endif // UNARYINSTRUCTION_H