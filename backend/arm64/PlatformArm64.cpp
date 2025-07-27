///
/// @file PlatformArm64.cpp
/// @brief  Arm64平台相关实现
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
#include "PlatformArm64.h"

#include "IntegerType.h"

// ARMv8寄存器命名规则：
// x0-x28: 通用寄存器
// x29: 帧指针(fp)
// x30: 链接寄存器(lr)
// sp: 堆栈指针
const std::string PlatformArm64::regName[PlatformArm64::maxRegNum] = {
    "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7",  // 参数/临时寄存器
    "x8", "x9", "x10", "x11", "x12", "x13", "x14", "x15",  // 临时寄存器
    "x16", "x17", "x18", "x19", "x20", "x21", "x22", "x23",  // 临时/被调用者保存
    "x24", "x25", "x26", "x27", "x28",  // 被调用者保存
    "x29", // 帧指针(fp)
    "x30", // 链接寄存器(lr)
    "sp"   // 堆栈指针
};

RegVariable * PlatformArm64::intRegVal[PlatformArm64::maxRegNum] = {
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[0], 0),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[1], 1),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[2], 2),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[3], 3),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[4], 4),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[5], 5),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[6], 6),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[7], 7),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[8], 8),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[9], 9),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[10], 10),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[11], 11),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[12], 12),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[13], 13),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[14], 14),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[15], 15),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[16], 16),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[17], 17),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[18], 18),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[19], 19),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[20], 20),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[21], 21),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[22], 22),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[23], 23),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[24], 24),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[25], 25),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[26], 26),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[27], 27),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[28], 28),
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[29], 29), // x29 (fp)
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[30], 30), // x30 (lr)
    new RegVariable(IntegerType::getTypeInt(), PlatformArm64::regName[31], 31)  // sp
};

/// @brief 循环左移两位（保留原实现，可能用于立即数处理）
void PlatformArm64::roundLeftShiftTwoBit(unsigned int & num)
{
    // 取左移即将溢出的两位
    const unsigned int overFlow = num & 0xc0000000;

    // 将溢出部分追加到尾部
    num = (num << 2) | (overFlow >> 30);
}

/// @brief 判断num是否是常数表达式（保留原实现）
bool PlatformArm64::__constExpr(int num)
{
    unsigned int new_num = (unsigned int) num;

    for (int i = 0; i < 16; i++) {

        if (new_num <= 0xff) {
            return true;
        }

        roundLeftShiftTwoBit(new_num);
    }

    return false;
}

/// @brief 同时处理正数和负数（保留原实现）
bool PlatformArm64::constExpr(int num)
{
    return __constExpr(num) || __constExpr(-num);
}

/// @brief 判定是否是合法的偏移（修改范围以适应ARMv8）
/// @param num
/// @return
bool PlatformArm64::isDisp(int num)
{
    // ARMv8的LDR/STR指令支持±256KB范围偏移
    // 但通常编译器使用±4095范围（12位立即数）
    return num >= -4095 && num <= 4095;
}

/// @brief 判断是否是合法的寄存器名（完全重写）
/// @param s 寄存器名字
/// @return 是否是
bool PlatformArm64::isReg(std::string name)
{
    // 检查sp
    if (name == "sp") {
        return true;
    }
    
    // 检查x0-x30格式
    if (name.length() > 1 && name[0] == 'x') {
        try {
            int regNum = std::stoi(name.substr(1));
            return regNum >= 0 && regNum <= 30;
        } catch (...) {
            return false;
        }
    }
    
    return false;
}