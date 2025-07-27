#ifndef DEAD_CODE_ELIMINATOR_H
#define DEAD_CODE_ELIMINATOR_H

#include "Function.h"
#include "Module.h"
#include "Instruction.h"
#include "Instructions/LabelInstruction.h"
#include <set>
#include <map>
#include <vector>
#include <string>
#include <unordered_set>

/// @brief 死代码消除优化器
class DeadCodeEliminator {
private:
    /// @brief IR模块
    Module* module;

    /// @brief 预处理立即数条件
    /// @param func 当前函数
    void processConstantConditions(Function* func);
    
public:
    /// @brief 标记不可达的代码块
    /// @param func 当前函数
    void markUnreachableBlocks(Function* func ,bool justCFG = false);
private:
    /// @brief 标记恒真/恒假条件下的死代码
    /// @param func 当前函数
    void markConstantConditions(Function* func);

    /// @brief 标记return语句后的不可达代码
    /// @param func 当前函数
    void markCodeAfterReturn(Function* func);
    
    /// @brief 标记一个特定分支为死代码
    /// @param insts 指令序列
    /// @param startIdx 起始索引
    /// @param branchLabel 分支标签
    void markDeadBranch(std::vector<Instruction*>& insts, size_t startIdx, LabelInstruction* branchLabel);
    
    /// @brief 判断字符串是否为数字
    /// @param str 字符串
    /// @return 是否为数字
    bool isNumeric(const std::string& str);
    
    /// @brief 分析并标记未使用的函数
    void markUnusedFunctions();
    
    /// @brief 收集所有函数调用
    /// @return 被调用的函数名集合
    std::unordered_set<std::string> collectFunctionCalls();

public:
    /// @brief 构造函数
    /// @param _module IR模块
    DeadCodeEliminator(Module* _module);

    /// @brief 执行死代码消除优化
    void run();
};

#endif // DEAD_CODE_ELIMINATOR_H 