///
/// @file MoveInstruction.h
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
#pragma once

#include <string>

#include "Value.h"
#include "Instruction.h"

class Function;

///
/// @brief 复制指令
///
class MoveInstruction : public Instruction {

private:
    /// @brief 操作类型：0-普通赋值，1-存储操作，2-加载操作
    int opType;

public:
    ///
    /// @brief 构造函数
    /// @param _func 所属的函数
    /// @param result 结构操作数
    /// @param srcVal1 源操作数
    /// @param _isStore 是否是存储操作（针对数组元素赋值）
    ///
    MoveInstruction(Function * _func, Value * result, Value * srcVal1, bool _isStore = false);

    ///
    /// @brief 构造函数（支持加载操作）
    /// @param _func 所属的函数
    /// @param result 结果操作数
    /// @param srcVal1 源操作数
    /// @param _opType 操作类型：0-普通赋值，1-存储操作，2-加载操作
    ///
    MoveInstruction(Function * _func, Value * result, Value * srcVal1, int _opType);

    /// @brief 转换成字符串
    void toString(std::string & str) override;

    /// @brief 获取操作类型
    /// @return 操作类型：0-普通赋值，1-存储操作，2-加载操作，3-存储地址
    int getOpType() const
    {
        return opType;
    }

    /// @brief 获取目标操作数
    /// @return 目标操作数
    Value * getDst()
    {
        return getOperand(0);
    }

    /// @brief 获取源操作数
    /// @return 源操作数
    Value * getSrc()
    {
        return getOperand(1);
    }
};
