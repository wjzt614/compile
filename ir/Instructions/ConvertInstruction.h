///
/// @file ConvertInstruction.h
/// @brief 类型转换指令
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

#include "Instruction.h"
#include "Function.h"
#include "IntegerType.h"
#include "FloatType.h"

/// @brief 类型转换IR指令类
class ConvertInstruction : public Instruction {

public:
    /// @brief 构造函数
    /// @param _func 所属函数
    /// @param op 操作类型（IRINST_OP_FTOI或IRINST_OP_ITOF）
    /// @param _dest 结果
    /// @param _src 源操作数
    ConvertInstruction(Function * _func, IRInstOperator op, Value * _dest, Value * _src);

    /// @brief 构造函数（向后兼容）
    /// @param _func 所属函数

    /// @param _dest 结果
    /// @param _src 源操作数
    ConvertInstruction(Function * _func, Value * _dest, Value * _src);

    /// @brief 析构函数
    virtual ~ConvertInstruction() = default;

    ///
    /// @brief 转换成IR指令文本形式
    /// @param str IR指令文本
    ///
    void toString(std::string & str) override;

    ///
    /// @brief 根据源操作数和目标操作数类型，创建相应的类型转换指令
    /// @param _func 所属函数
    /// @param _dest 目标变量
    /// @param _src 源操作数
    /// @return ConvertInstruction* 创建的类型转换指令
    ///
    static ConvertInstruction * createConvertInst(Function * _func, Value * _dest, Value * _src);

protected:
    /// @brief 源操作数
    Value * src;

    /// @brief 目标变量
    Value * dest;
}; 