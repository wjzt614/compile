///
/// @file FloatType.cpp
/// @brief 浮点型类型类，描述32位的float类型
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

#include "FloatType.h"

// 初始化静态成员变量
FloatType * FloatType::oneInstance = nullptr;

FloatType * FloatType::getType()
{
    if (oneInstance == nullptr) {
        oneInstance = new FloatType();
    }
    return oneInstance;
} 