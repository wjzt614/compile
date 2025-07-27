#include "DeadCodeEliminator.h"
#include "Instructions/GotoInstruction.h"
#include "Instructions/ExitInstruction.h"
#include "Instructions/BinaryInstruction.h"
#include "Instructions/FuncCallInstruction.h"
#include <iostream>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <gvc.h>
#include "EntryInstruction.h"
#include <filesystem>

DeadCodeEliminator::DeadCodeEliminator(Module* _module) : module(_module) {}

void DeadCodeEliminator::run() {
    std::cout << "执行死代码优化..." << std::endl;
    
    // 0. 标记未使用的函数
    markUnusedFunctions();
    
    // 对模块中的每个函数执行死代码消除
    for (auto& func : module->getFunctionList()) {
        // 跳过内置函数
        if (func->isBuiltin()) {
            continue;
        }
        
        std::cout << "开始处理函数: " << func->getName() << std::endl;
        
        // 1. 预处理直接常量条件
        processConstantConditions(func);
        
        // 2. 标记恒真/恒假条件下的死代码
        markConstantConditions(func);
        
        // 3. 标记return语句后的不可达代码
        markCodeAfterReturn(func);
        
        // 4. 标记不可达的代码块
        markUnreachableBlocks(func);
        
        // 5. 统计已标记的死代码指令数量
        int deadCount = 0;
        for (auto& inst : func->getInterCode().getInsts()) {
            if (inst->isDead()) {
                deadCount++;
            }
        }
        std::cout << "函数 " << func->getName() << " 中标记了 " << deadCount << " 条死代码指令" << std::endl;
    }
    
    std::cout << "死代码优化完成" << std::endl;
}

// 收集所有函数调用
std::unordered_set<std::string> DeadCodeEliminator::collectFunctionCalls() {
    std::unordered_set<std::string> calledFunctions;
    
    // 先添加main函数，它总是被调用的
    calledFunctions.insert("main");
    
    // 遍历所有函数和指令，寻找函数调用
    for (auto& func : module->getFunctionList()) {
        if (func->isBuiltin()) continue;
        
        for (auto& inst : func->getInterCode().getInsts()) {
            // 检查是否是函数调用指令
            if (inst->getOp() == IRInstOperator::IRINST_OP_FUNC_CALL) {
                auto* callInst = dynamic_cast<FuncCallInstruction*>(inst);
                if (callInst) {
                    calledFunctions.insert(callInst->getCalledName());
                }
            }
        }
    }
    
    return calledFunctions;
}

// 标记未使用的函数
void DeadCodeEliminator::markUnusedFunctions() {
    // 收集所有被调用的函数
    std::unordered_set<std::string> calledFunctions = collectFunctionCalls();
    
    std::cout << "分析未使用的函数..." << std::endl;
    std::cout << "被调用的函数: ";
    for (const auto& name : calledFunctions) {
        std::cout << name << " ";
    }
    std::cout << std::endl;
    
    // 标记未被调用的函数中的所有指令为死代码
    for (auto& func : module->getFunctionList()) {
        if (func->isBuiltin()) continue;
        
        std::string funcName = func->getName();
        if (calledFunctions.find(funcName) == calledFunctions.end()) {
            std::cout << "标记未使用的函数: " << funcName << std::endl;
            
            // 标记该函数的所有指令为死代码
            for (auto& inst : func->getInterCode().getInsts()) {
                if (inst->getOp() != IRInstOperator::IRINST_OP_LABEL)inst->setDead();
            }
        }
    }
}

// 预处理立即数条件
void DeadCodeEliminator::processConstantConditions(Function* func) {
    auto& insts = func->getInterCode().getInsts();
    
    std::cout << "  预处理立即数条件..." << std::endl;
    
    // 处理 icmp ne 0,0 和 icmp ne 1,0 这样的立即数比较
    for (size_t i = 0; i < insts.size(); i++) {
        auto* inst = insts[i];
        
        // 检查是否是条件比较指令
        if (inst->getOp() == IRInstOperator::IRINST_OP_NE_I || inst->getOp() == IRInstOperator::IRINST_OP_EQ_I) {
            auto* binaryInst = dynamic_cast<BinaryInstruction*>(inst);
            if (binaryInst && binaryInst->getOperandsNum() == 2) {
                Value* left = binaryInst->getOperand(0);
                Value* right = binaryInst->getOperand(1);
                
                // 检查操作数是否都是立即数
                if (left && right) {
                    std::string leftName = left->getIRName();
                    std::string rightName = right->getIRName();
                    
                    // 检查是否是数字字面量
                    bool leftIsNum = isNumeric(leftName);
                    bool rightIsNum = isNumeric(rightName);
                    
                    if (leftIsNum && rightIsNum) {
                        // 两个操作数都是立即数，可以在编译时确定结果
                        int leftVal = std::stoi(leftName);
                        int rightVal = std::stoi(rightName);
                        bool result;
                        
                        // 根据比较操作确定结果
                        if (inst->getOp() == IRInstOperator::IRINST_OP_EQ_I) {
                            result = (leftVal == rightVal);
                        } else { // IRINST_OP_NE_I
                            result = (leftVal != rightVal);
                        }
                        
                        std::cout << "  发现立即数比较: " << leftName << (inst->getOp() == IRInstOperator::IRINST_OP_EQ_I ? " == " : " != ") << rightName << " 结果为: " << result << std::endl;
                        
                        // 设置为常量
                        binaryInst->setConst(true);
                        binaryInst->const_int = result ? 1 : 0;
                        
                        // 向后查找使用这个比较结果的bc指令
                        for (size_t j = i + 1; j < insts.size(); j++) {
                            if (insts[j]->getOp() == IRInstOperator::IRINST_OP_GOTO) {
                                auto* gotoInst = dynamic_cast<GotoInstruction*>(insts[j]);
                                if (gotoInst && gotoInst->getCondition() == binaryInst) {
                                    std::cout << "  发现使用该比较结果的goto指令" << std::endl;
                                    
                                    // 创建新的跳转指令
                                    GotoInstruction* newGoto;
                                    if (result) { // 条件为真
                                        // 跳转到true分支
                                        if (gotoInst->getTrueTarget()) {
                                            newGoto = new GotoInstruction(func, gotoInst->getTrueTarget());
                                            insts[j] = newGoto;
                                            
                                            // 标记false分支为死代码
                                            if (gotoInst->getFalseTarget()) {
                                                markDeadBranch(insts, j, gotoInst->getFalseTarget());
                                            }
                                        }
                                    } else { // 条件为假
                                        // 跳转到false分支
                                        if (gotoInst->getFalseTarget()) {
                                            newGoto = new GotoInstruction(func, gotoInst->getFalseTarget());
                                            insts[j] = newGoto;
                                            
                                            // 标记true分支为死代码
                                            if (gotoInst->getTrueTarget()) {
                                                markDeadBranch(insts, j, gotoInst->getTrueTarget());
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// 标记一个特定分支为死代码
void DeadCodeEliminator::markDeadBranch(std::vector<Instruction*>& insts, size_t startIdx, LabelInstruction* branchLabel) {
    bool inBranch = false;
    std::string branchName = branchLabel->getName();
    
    std::cout << "  标记分支为死代码: " << branchName << std::endl;
    
    for (size_t i = startIdx + 1; i < insts.size(); i++) {
        auto* inst = insts[i];
        
        // 检查是否是目标标签
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            auto* labelInst = dynamic_cast<LabelInstruction*>(inst);
            if (labelInst) {
                if (labelInst == branchLabel) {
                    inBranch = true;
                    std::cout << "  开始标记" << std::endl;
                } else if (inBranch) {
                    inBranch = false;
                    std::cout << "  停止标记" << std::endl;
                    break;
                }
            }
        }
        
        // 标记该分支中的指令为死代码
        if (inBranch) {
            if (inst->getOp() != IRInstOperator::IRINST_OP_LABEL)inst->setDead();
            std::cout << "  标记指令为死代码" << std::endl;
        }
    }
}

// 判断字符串是否为数字
bool DeadCodeEliminator::isNumeric(const std::string& str) {
    if (str.empty()) return false;
    
    // 检查负号
    size_t start = (str[0] == '-') ? 1 : 0;
    
    // 空字符串或只有负号
    if (start == str.size()) return false;
    
    // 检查剩余字符是否都是数字
    for (size_t i = start; i < str.size(); i++) {
        if (!std::isdigit(str[i])) return false;
    }
    
    return true;
}

void DeadCodeEliminator::markConstantConditions(Function* func) {
    // 获取函数的指令序列
    auto& insts = func->getInterCode().getInsts();
    
    std::cout << "  分析条件跳转指令..." << std::endl;
    
    for (size_t i = 0; i < insts.size(); i++) {
        auto* inst = insts[i];
        
        // 检查是否是条件跳转指令
        if (inst->getOp() == IRInstOperator::IRINST_OP_GOTO) {
            std::cout << "  发现goto指令: " << i << std::endl;
            
            auto* gotoInst = dynamic_cast<GotoInstruction*>(inst);
            if (gotoInst) {
                std::cout << "  条件: " << (gotoInst->getCondition() ? "存在" : "不存在") << std::endl;
                std::cout << "  True目标: " << (gotoInst->getTrueTarget() ? gotoInst->getTrueTarget()->getName() : "不存在") << std::endl;
                std::cout << "  False目标: " << (gotoInst->getFalseTarget() ? gotoInst->getFalseTarget()->getName() : "不存在") << std::endl;
            }
            
            if (gotoInst && gotoInst->getCondition() && gotoInst->getTrueTarget() && gotoInst->getFalseTarget()) {
                Value* condition = gotoInst->getCondition();
                std::cout << "  条件值: " << condition->getIRName() << std::endl;
                std::cout << "  是否常量: " << (condition->isConst() ? "是" : "否") << std::endl;
                if (condition->isConst()) {
                    std::cout << "  常量值: " << condition->const_int << std::endl;
                }
                
                // 检查条件是否为常量
                if (condition->isConst()) {
                    // 检查恒真条件（常量非零）
                    if (condition->const_int != 0) {
                        std::cout << "发现恒真条件，优化跳转到: " << gotoInst->getTrueTarget()->getName() << std::endl;
                        
                        // 将条件跳转替换为无条件跳转
                        auto* newGoto = new GotoInstruction(func, gotoInst->getTrueTarget());
                        insts[i] = newGoto;
                        
                        // 标记false分支为死代码
                        auto falseLabel = gotoInst->getFalseTarget();
                        bool inFalseBranch = false;
                        
                        for (size_t j = i + 1; j < insts.size(); j++) {
                            auto* nextInst = insts[j];
                            
                            // 如果遇到falseLabel，开始标记
                            if (nextInst->getOp() == IRInstOperator::IRINST_OP_LABEL && 
                                dynamic_cast<LabelInstruction*>(nextInst) == falseLabel) {
                                inFalseBranch = true;
                            }
                            
                            // 如果遇到其他标签，停止标记
                            if (inFalseBranch && nextInst->getOp() == IRInstOperator::IRINST_OP_LABEL && 
                                dynamic_cast<LabelInstruction*>(nextInst) != falseLabel) {
                                inFalseBranch = false;
                            }
                            
                            // 标记false分支中的指令为死代码
                            if (inFalseBranch) {
                                if (nextInst->getOp() != IRInstOperator::IRINST_OP_LABEL)nextInst->setDead();
                            }
                        }
                    }
                    // 检查恒假条件（常量为零）
                    else {
                        std::cout << "发现恒假条件，优化跳转到: " << gotoInst->getFalseTarget()->getName() << std::endl;
                        
                        // 将条件跳转替换为无条件跳转
                        auto* newGoto = new GotoInstruction(func, gotoInst->getFalseTarget());
                        insts[i] = newGoto;
                        
                        // 标记true分支为死代码
                        auto trueLabel = gotoInst->getTrueTarget();
                        bool inTrueBranch = false;
                        
                        for (size_t j = i + 1; j < insts.size(); j++) {
                            auto* nextInst = insts[j];
                            
                            // 如果遇到trueLabel，开始标记
                            if (nextInst->getOp() == IRInstOperator::IRINST_OP_LABEL && 
                                dynamic_cast<LabelInstruction*>(nextInst) == trueLabel) {
                                inTrueBranch = true;
                            }
                            
                            // 如果遇到其他标签，停止标记
                            if (inTrueBranch && nextInst->getOp() == IRInstOperator::IRINST_OP_LABEL && 
                                dynamic_cast<LabelInstruction*>(nextInst) != trueLabel) {
                                inTrueBranch = false;
                            }
                            
                            // 标记true分支中的指令为死代码
                            if (inTrueBranch) {
                                if (nextInst->getOp() != IRInstOperator::IRINST_OP_LABEL)nextInst->setDead();
                            }
                        }
                    }
                }
                // 特殊检查：处理常量表达式比较（例如 if (0) 或 if (1)）
                else if (User* user = dynamic_cast<User*>(condition)) {
                    std::cout << "  条件是User类型" << std::endl;
                    std::cout << "  操作数数量: " << user->getOperandsNum() << std::endl;
                    
                    // 如果用户有两个操作数，可能是条件表达式
                    if (user->getOperandsNum() == 2) {
                        // 获取两个操作数
                        Value* left = user->getOperand(0);
                        Value* right = user->getOperand(1);
                        
                        std::cout << "  左操作数: " << (left ? left->getIRName() : "NULL") << std::endl;
                        std::cout << "  右操作数: " << (right ? right->getIRName() : "NULL") << std::endl;
                        std::cout << "  左操作数是常量: " << (left && left->isConst() ? "是" : "否") << std::endl;
                        std::cout << "  右操作数是常量: " << (right && right->isConst() ? "是" : "否") << std::endl;
                        if (left && left->isConst()) std::cout << "  左操作数值: " << left->const_int << std::endl;
                        if (right && right->isConst()) std::cout << "  右操作数值: " << right->const_int << std::endl;
                        
                        // 检查是否是 if (0 == 0) 这种恒为真的情况
                        if (left && right && left->isConst() && right->isConst() && 
                            (condition->getIRName().find("icmp eq") != std::string::npos || 
                             condition->getIRName().find("icmp ne") != std::string::npos)) {
                            
                            bool isEqual = (left->const_int == right->const_int);
                            bool conditionResult;
                            
                            // 根据操作类型确定条件结果
                            if (condition->getIRName().find("icmp eq") != std::string::npos) {
                                conditionResult = isEqual;
                            } else { // icmp ne
                                conditionResult = !isEqual;
                            }
                            
                            // 根据条件结果选择跳转目标
                            LabelInstruction* targetLabel = conditionResult ? 
                                gotoInst->getTrueTarget() : gotoInst->getFalseTarget();
                            
                            // 将条件跳转替换为无条件跳转
                            auto* newGoto = new GotoInstruction(func, targetLabel);
                            insts[i] = newGoto;
                            
                            // 标记不会执行的分支为死代码
                            LabelInstruction* deadLabel = conditionResult ? 
                                gotoInst->getFalseTarget() : gotoInst->getTrueTarget();
                            bool inDeadBranch = false;
                            
                            for (size_t j = i + 1; j < insts.size(); j++) {
                                auto* nextInst = insts[j];
                                
                                // 如果遇到deadLabel，开始标记
                                if (nextInst->getOp() == IRInstOperator::IRINST_OP_LABEL && 
                                    dynamic_cast<LabelInstruction*>(nextInst) == deadLabel) {
                                    inDeadBranch = true;
                                }
                                
                                // 如果遇到其他标签，停止标记
                                if (inDeadBranch && nextInst->getOp() == IRInstOperator::IRINST_OP_LABEL && 
                                    dynamic_cast<LabelInstruction*>(nextInst) != deadLabel) {
                                    inDeadBranch = false;
                                }
                                
                                // 标记死分支中的指令为死代码
                                if (inDeadBranch) {
                                    if (nextInst->getOp() != IRInstOperator::IRINST_OP_LABEL)nextInst->setDead();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

void DeadCodeEliminator::markCodeAfterReturn(Function* func) {
    // 获取函数的指令序列
    auto& insts = func->getInterCode().getInsts();
    
    std::cout << "  标记return后的不可达代码..." << std::endl;
    
    // 基本块结构
    struct BasicBlock {
        size_t startIdx;         // 块开始索引
        size_t endIdx;           // 块结束索引
        std::string label;       // 标签名
        bool hasReturn = false;  // 是否包含return指令
        std::vector<std::string> successors; // 后继块
    };
    
    // 收集所有基本块
    std::unordered_map<std::string, BasicBlock> blocks;
    std::string currentLabel;
    
    // 第一遍：识别所有基本块和它们的范围
    for (size_t i = 0; i < insts.size(); i++) {
        auto* inst = insts[i];
        
        // 如果是新的标签，开始一个新的基本块
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            // 结束前一个基本块
            if (!currentLabel.empty()) {
                blocks[currentLabel].endIdx = i - 1;
            }
            
            // 开始新的基本块
            auto* labelInst = dynamic_cast<LabelInstruction*>(inst);
            currentLabel = labelInst->getName();
            blocks[currentLabel].startIdx = i;
            blocks[currentLabel].label = currentLabel;
        }
        
        // 检查是否包含return指令
        if (inst->getOp() == IRInstOperator::IRINST_OP_EXIT) {
            if (!currentLabel.empty()) {
                blocks[currentLabel].hasReturn = true;
            }
        }
    }
    
    // 结束最后一个基本块
    if (!currentLabel.empty()) {
        blocks[currentLabel].endIdx = insts.size() - 1;
    }
    
    // 第二遍：建立基本块间的控制流图
    currentLabel = "";
    for (size_t i = 0; i < insts.size(); i++) {
        auto* inst = insts[i];
        
        // 更新当前基本块
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            auto* labelInst = dynamic_cast<LabelInstruction*>(inst);
            currentLabel = labelInst->getName();
        }
        
        // 处理跳转指令
        if (inst->getOp() == IRInstOperator::IRINST_OP_GOTO) {
            auto* gotoInst = dynamic_cast<GotoInstruction*>(inst);
            if (gotoInst) {
                // 无条件跳转
                if (!gotoInst->getCondition() && gotoInst->getTarget()) {
                    blocks[currentLabel].successors.push_back(gotoInst->getTarget()->getName());
                } 
                // 条件跳转
                else if (gotoInst->getTrueTarget() && gotoInst->getFalseTarget()) {
                    blocks[currentLabel].successors.push_back(gotoInst->getTrueTarget()->getName());
                    blocks[currentLabel].successors.push_back(gotoInst->getFalseTarget()->getName());
                }
            }
        }
        // 如果基本块以非跳转、非return指令结束，则它会顺序执行到下一个基本块
        else if (i == blocks[currentLabel].endIdx && 
                 inst->getOp() != IRInstOperator::IRINST_OP_EXIT) {
            // 查找下一个基本块
            for (size_t j = i + 1; j < insts.size(); j++) {
                if (insts[j]->getOp() == IRInstOperator::IRINST_OP_LABEL) {
                    auto* nextLabel = dynamic_cast<LabelInstruction*>(insts[j]);
                    blocks[currentLabel].successors.push_back(nextLabel->getName());
                    break;
                }
            }
        }
    }
    
    // 第三遍：标记基本块内部return后的指令
    for (auto& pair : blocks) {
        auto& block = pair.second;
        bool afterReturn = false;
        
        // 标记基本块内return后的指令为死代码
        for (size_t i = block.startIdx; i <= block.endIdx && i < insts.size(); i++) {
            auto* inst = insts[i];
            
            if (inst->getOp() == IRInstOperator::IRINST_OP_EXIT) {
                afterReturn = true;
                std::cout << "  发现函数 " << func->getName() << " 中的return指令" << std::endl;
            } else if (afterReturn) {
                if (inst->getOp() != IRInstOperator::IRINST_OP_LABEL)inst->setDead();
                std::string instStr;
                inst->toString(instStr);
                std::cout << "  标记return后的指令为死代码: " << instStr << std::endl;
            }
        }
    }
    
    // 第四遍：计算可达性，标记不可达的基本块
    std::unordered_set<std::string> reachable;
    std::unordered_set<std::string> hasReturnBlocks; // 记录包含return的基本块
    std::stack<std::string> stack;
    
    // 收集所有包含return的基本块
    for (auto& pair : blocks) {
        if (pair.second.hasReturn) {
            hasReturnBlocks.insert(pair.first);
        }
    }
    
    // 从函数入口开始
    if (!blocks.empty()) {
        auto it = blocks.begin();
        stack.push(it->first);
        reachable.insert(it->first);
    }
    
    // DFS搜索可达的基本块
    while (!stack.empty()) {
        std::string current = stack.top();
        stack.pop();
        
        // 如果当前基本块包含return，不再继续搜索其后继
        if (hasReturnBlocks.find(current) != hasReturnBlocks.end()) {
            continue;
        }
        
        for (const auto& next : blocks[current].successors) {
            if (reachable.find(next) == reachable.end()) {
                reachable.insert(next);
                stack.push(next);
            }
        }
    }
    
    // 标记不可达的基本块中的指令为死代码
    for (auto& pair : blocks) {
        // 如果基本块不可达，或者在某个包含return的基本块之后
        if (reachable.find(pair.first) == reachable.end()) {
            auto& block = pair.second;
            
            for (size_t i = block.startIdx; i <= block.endIdx && i < insts.size(); i++) {
                if (insts[i]->getOp() != IRInstOperator::IRINST_OP_LABEL)insts[i]->setDead();
                std::string instStr;
                insts[i]->toString(instStr);
                std::cout << "  标记不可达基本块中的指令为死代码: " << instStr << std::endl;
            }
        }
    }
    
    // 特殊处理：检查无条件跳转后的代码
    for (size_t i = 0; i < insts.size(); i++) {
        auto* inst = insts[i];
        
        // 如果是无条件跳转或return指令
        if ((inst->getOp() == IRInstOperator::IRINST_OP_GOTO && 
             !dynamic_cast<GotoInstruction*>(inst)->getCondition()) ||
            inst->getOp() == IRInstOperator::IRINST_OP_EXIT) {
            
            // 标记同一基本块中后续的所有指令为死代码
            // 直到遇到下一个标签
            bool foundLabel = false;
            for (size_t j = i + 1; j < insts.size() && !foundLabel; j++) {
                if (insts[j]->getOp() == IRInstOperator::IRINST_OP_LABEL) {
                    foundLabel = true;
                } else {
                    if (insts[j]->getOp() != IRInstOperator::IRINST_OP_LABEL)insts[j]->setDead();
                    std::string instStr;
                    insts[j]->toString(instStr);
                    std::cout << "  标记跳转/return后的指令为死代码: " << instStr << std::endl;
                }
            }
        }
    }
}

void DeadCodeEliminator::markUnreachableBlocks(Function* func , bool justCFG) {
    // 获取函数的指令序列
    auto& insts = func->getInterCode().getInsts();
    int64_t labelIndex = 0;
    // 汇编指令输出前要确保Label的名字有效
    for (auto inst: insts) {
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            inst->setName(IR_LABEL_PREFIX + std::to_string(labelIndex++));
        }
    }
    //printf("指令数量: %ld\n", insts.size());
    // 建立基本块跳转图
    std::unordered_map<std::string, std::vector<std::string>> blockGraph;
    std::unordered_map<std::string, size_t> labelToIndex;
    std::vector<std::string> blocks;
    // 第一遍：收集所有标签和它们的索引
    for (size_t i = 0; i < insts.size(); i++) {
        if(insts[i]->getOp() == IRInstOperator::IRINST_OP_ENTRY){
            //printf("  发现入口指令: %s\n", insts[i]->getIRName().c_str());
            auto* entryInst = dynamic_cast<EntryInstruction*>(insts[i]);
            if (entryInst) {
                std::string entryLabel = func->getName() + "_entry";
                labelToIndex[entryLabel] = i;
                blocks.push_back(entryLabel);
                blockGraph[entryLabel] = std::vector<std::string>();
            }
            continue;
        }
        if (insts[i]->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            //printf("  发现标签指令: %s\n", insts[i]->getIRName().c_str());
            auto* labelInst = dynamic_cast<LabelInstruction*>(insts[i]);
            std::string label = labelInst->getName();
            labelToIndex[label] = i;
            blocks.push_back(label);
            blockGraph[label] = std::vector<std::string>();
        }
    }
    
    // 第二遍：建立跳转关系
    std::string currentBlock = func->getName() + "_entry";
    bool hasGoto = false;
    hasGoto =  hasGoto;
    for (size_t i = 0; i < insts.size(); i++) {
        auto* inst = insts[i];
        
        // 更新当前基本块
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            hasGoto = false;
            currentBlock = dynamic_cast<LabelInstruction*>(inst)->getName();
            //std::cout<<currentBlock<<std::endl;
            // 处理跳转指令
        }else if (inst->getOp() == IRInstOperator::IRINST_OP_GOTO) {
            auto* gotoInst = dynamic_cast<GotoInstruction*>(inst);
            hasGoto = true;
            // 无条件跳转
            if (!gotoInst->getCondition()) {
                std::string target = gotoInst->getTarget()->getName();
                blockGraph[currentBlock].push_back(target);
                //std::cout<<currentBlock<<target<<std::endl;
            } 
            // 条件跳转
            else if (gotoInst->getTrueTarget() && gotoInst->getFalseTarget()) {
                std::string trueTarget = gotoInst->getTrueTarget()->getName();
                std::string falseTarget = gotoInst->getFalseTarget()->getName();
                blockGraph[currentBlock].push_back(trueTarget);
                blockGraph[currentBlock].push_back(falseTarget);
            }
            // 如果不是跳转指令且是基本块的最后一条指令，默认流向下一个基本块
        }
        if (!hasGoto &&i + 1 < insts.size() && insts[i + 1]->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            std::string nextBlock = dynamic_cast<LabelInstruction*>(insts[i + 1])->getName();
            blockGraph[currentBlock].push_back(nextBlock);
            //std::cout<<currentBlock<<nextBlock<<std::endl;
        }
    }
    
    // 使用DFS标记可达的基本块
    std::unordered_set<std::string> reachable;
    std::stack<std::string> stack;
    
    // 从入口基本块开始
    if (!blocks.empty()) {
        stack.push(blocks[0]);
        reachable.insert(blocks[0]);
        
        while (!stack.empty()) {
            //printf("当前基本块: %s\n", stack.top().c_str());
            std::string current = stack.top();
            stack.pop();
            
            for (const auto& next : blockGraph[current]) {
                if (reachable.find(next) == reachable.end()) {
                    reachable.insert(next);
                    stack.push(next);
                }
            }
        }
    }
    
    // 标记不可达的基本块中的所有指令为死代码
    currentBlock = "";
    bool isReachable = true;
    std::unordered_set<std::string> unReachBlocks;
    for (auto* inst : insts) {
        // 更新当前基本块和可达性
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            currentBlock = dynamic_cast<LabelInstruction*>(inst)->getName();
            isReachable = reachable.find(currentBlock) != reachable.end();
            
            if (!isReachable) {
                //std::cout << "标记不可达的基本块: " << currentBlock << std::endl;
            }
        }
        
        // 标记不可达基本块中的指令为死代码
        if (!isReachable) {
            unReachBlocks.insert(currentBlock);
            if (inst->getOp() != IRInstOperator::IRINST_OP_LABEL)inst->setDead();
        }
    }
    GVC_t * gvc = gvContext();

    // 创建一个空的图
    Agraph_t * g = agopen("g", Agdirected, nullptr);
    std::unordered_map<std::string, Agnode_t*> nodeMap; // 节点映射
    //遍历block，创建所有node
    for (auto block: blocks) {
        //创建节点
        Agnode_t * n1 = agnode(g, block.data(), 1);
        agsafeset(n1, "shape", "box", "");
        agsafeset(n1, "label", block.data(), "");
        if(unReachBlocks.find(block) != unReachBlocks.end()){
            agsafeset(n1, (char *) "style", (char *) "filled", (char *) "");
            agsafeset(n1, (char *) "fillcolor", (char *) "yellow", (char *) "");
        }
        nodeMap[block] = n1; // 保存节点到映射中
    }

    //遍历当前函数的所有block，创建所有edge
    for (auto block: blocks) {
        //创建边
        auto from_node = nodeMap[block];
        for (const auto & exit_label: blockGraph[block]) {
            auto to_node = nodeMap[exit_label];
            if (to_node != nullptr) {
                agedge(g, from_node, to_node, nullptr, 1);
            }
        }
    }
    //输出图片；每一个函数输出一张图
    // 设置布局
    gvLayout(gvc, g, "dot");
    // 设置输出格式
    std::string dest_directory = "./CFG/"; //输出文件夹
    std::string outputFormat = "png";      //输出格式
    std::string outputFile = dest_directory + func->getName() + ".png";

    // 检查文件夹是否存在
    if (!std::filesystem::exists(dest_directory)) {
        // 如果文件夹不存在，则创建文件夹
        std::filesystem::create_directories(dest_directory);
    }
    bool print_flag = true; // 是否打印图形
    if (print_flag) {
        // 渲染图并输出到文件
        FILE * fp = fopen(outputFile.c_str(), "w");
        gvRender(gvc, g, outputFormat.c_str(), fp);
        fclose(fp);
    }

    // 释放资源
    gvFreeLayout(gvc, g);
    agclose(g);
    gvFreeContext(gvc);
} 