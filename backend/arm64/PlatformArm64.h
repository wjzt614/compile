///
/// @file PlatformArm64.h
/// @brief  Arm64平台相关头文件
/// @author zenglj (zenglj@live.com)
/// @version 1.0
/// @date 2024-11-21
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-11-21 <td>1.0     <td>zenglj  <td>新做
/// <tr><td>2025-06-07 <td>1.1     <td>DeepSeek  <td>适配ARMv8-A架构
/// </table>
///
#pragma once

#include <string>

#include "RegVariable.h"

// 二元操作时寄存器分配为源操作数1用REG_ALLOC_SIMPLE_SRC1_REG_NO，
//   源操作数2用REG_ALLOC_SIMPLE_SRC2_REG_NO
//   结果用REG_ALLOC_SIMPLE_DST_REG_NO
#define REG_ALLOC_SIMPLE_SRC1_REG_NO 5
#define REG_ALLOC_SIMPLE_SRC2_REG_NO 6
#define REG_ALLOC_SIMPLE_DST_REG_NO 6

// 临时寄存器改为 x16（ARMv8中的临时寄存器）
#define Arm64_TMP_REG_NO 16
#define Arm64_TMP_REG_NO2 17
#define Arm64_TMP_REG_NO3 18
#define Arm64_TMP_REG_NO4 19
// 帧指针 FP 改为 x29（ARMv8专用帧指针寄存器）
#define Arm64_FP_REG_NO 29

// 链接寄存器 LX 改为 x30（LR在ARMv8中是x30）
#define Arm64_LX_REG_NO 30

// 堆栈指针 SP 在ARMv8中是专用寄存器，直接使用"sp"名称
#define Arm64_SP_REG_NO 31

/// @brief Arm64平台信息
class PlatformArm64 {

    /// @brief 循环左移两位
    /// @param num
    static void roundLeftShiftTwoBit(unsigned int & num);

    /// @brief 判断num是否是常数表达式
    /// @param num
    /// @return
    static bool __constExpr(int num);

public:
    /// @brief 同时处理正数和负数
    /// @param num
    /// @return
    static bool constExpr(int num);

    /// @brief 判定是否是合法的偏移
    /// @param num
    /// @return
    static bool isDisp(int num);

    /// @brief 判断是否是合法的寄存器名
    /// @param name 寄存器名字
    /// @return 是否是
    static bool isReg(std::string name);

    /// @brief 最大寄存器数目（ARMv8有31个通用寄存器 + SP）
    static const int maxRegNum = 32;

    /// @brief 可使用的通用寄存器的个数
    static const int maxUsableRegNum = 31;  // x0-x30

    /// @brief 寄存器的名字，x0-x30 + sp
    static const std::string regName[maxRegNum];

    /// @brief 对寄存器分配Value，记录位置
    static RegVariable * intRegVal[PlatformArm64::maxRegNum];
};