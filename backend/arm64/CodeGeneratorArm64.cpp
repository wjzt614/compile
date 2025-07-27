///
/// @file CodeGeneratorArm64.cpp
/// @brief Arm64的后端处理实现
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
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include "Function.h"
#include "Module.h"
#include "PlatformArm64.h"
#include "Values/FormalParam.h"
#include "CodeGeneratorArm64.h"
#include "InstSelectorArm64.h"
#include "SimpleRegisterAllocatorArm64.h"
#include "ILocArm64.h"
#include "RegVariable.h"
#include "FuncCallInstruction.h"
#include "ArgInstruction.h"
#include "MoveInstruction.h"
#include "BinaryInstruction.h"
/// @brief 构造函数
/// @param tab 符号表
CodeGeneratorArm64::CodeGeneratorArm64(Module * _module) : CodeGeneratorAsm(_module)
{}

/// @brief 析构函数
CodeGeneratorArm64::~CodeGeneratorArm64()
{}

/// @brief 产生汇编头部分
void CodeGeneratorArm64::genHeader()
{
    //fprintf(fp, "%s\n", ".arch armv8-a");
    //fprintf(fp, "%s\n", ".arm");
    //fprintf(fp, "%s\n", ".cpu cortex-a53");
    //fprintf(fp, "%s\n", ".fpu vfpv4");
    //fprintf(fp, "%s\n", ".text");
}

/// @brief 全局变量Section，主要包含初始化的和未初始化过的
void CodeGeneratorArm64::genDataSection()
{
    // 生成数据段
    fprintf(fp, ".data\n");

    // 处理全局变量
    for (auto var: module->getGlobalVariables()) {
        if (var->isInBSSSection()) {
            // BSS段变量
            fprintf(fp, ".comm %s, %d, %d\n", 
                    var->getName().c_str(), 
                    var->getType()->getSize(), 
                    8);//var->getAlignment());
        } else {
            // 初始化的全局变量
            fprintf(fp, ".global %s\n", var->getName().c_str());
            fprintf(fp, ".align %d\n", 3);//var->getAlignment());
            fprintf(fp, ".type %s, %%object\n", var->getName().c_str());
            fprintf(fp, "%s:\n", var->getName().c_str());
            if (var->getType()->isArrayType()) {
                // 获取数组总字节大小
                //int space = var->getType()->getSize();
                // 计算数组元素个数（假设每个元素4字节）
                int totalElements = var->intVal[0];//space / 4;
                
                if (var->intVal) {
                    int initLength = *var->intVal;  // 初始值数量
                    int zeroCount = 0;              // 连续0计数器
                    
                    // 遍历所有需要初始化的元素（包括显式初始化和隐式0填充）
                    for (int i = 0; i < totalElements; ++i) {
                        int value = (i < initLength) ? var->intVal[i + 1] : 0;
                        
                        if (value != 0) {
                            // 输出累积的0区域
                            if (zeroCount > 0) {
                                fprintf(fp, ".zero %d\n", zeroCount * 8);//4);
                                zeroCount = 0;
                            }
                            // 输出非零值
                            fprintf(fp, ".xword 0x%x\n", value);
                        } else {
                            // 累积连续0
                            zeroCount++;
                        }
                    }
                    
                    // 处理末尾剩余的0区域
                    if (zeroCount > 0) {
                        fprintf(fp, ".zero %d\n", zeroCount * 8);//4);
                    }
                    
                    delete[] var->intVal;  // 释放初始化数组
                } else {
                    // 整个数组初始化为0
                    fprintf(fp, ".zero %d\n", totalElements * 8);//4);
                }
            } else {
                string varName = var->getName()[0] == '@' ? var->getName().substr(1) : var->getName();
                int32_t i = var->intVal ? *var->intVal : 0;
                fprintf(fp, ".xword 0x%x\n", i);
                delete var->intVal;
            }            
            // TODO: 初始化值处理
            /*
            if (Instanceof(intVar, ConstInt *, var->getInitValue())) {
                fprintf(fp, "\t.word %d\n", intVar->getVal());
            } else {
                // 默认初始化为0
                fprintf(fp, "\t.space %d\n", var->getType()->getSize());
            }
            */
        }
    }
}

///
/// @brief 获取IR变量相关信息字符串
/// @param str
///
void CodeGeneratorArm64::getIRValueStr(Value * val, std::string & str)
{
    std::string name = val->getName();
    std::string IRName = val->getIRName();
    int32_t regId = val->getRegId();
    int32_t baseRegId;
    int64_t offset;
    std::string showName;

    if (name.empty() && (!IRName.empty())) {
        showName = IRName;
    } else if ((!name.empty()) && IRName.empty()) {
        showName = name;
    } else if ((!name.empty()) && (!IRName.empty())) {
        showName = name + ":" + IRName;
    } else {
        showName = "";
    }

    if (regId != -1) {
        // 寄存器
        str += "\t@ " + showName + ":" + PlatformArm64::regName[regId];
    } else if (val->getMemoryAddr(&baseRegId, &offset)) {
        // 栈内寻址，[x29,#4]
        str += "\t@ " + showName + ":[" + PlatformArm64::regName[baseRegId] + ",#" + std::to_string(offset) + "]";
    }
}

/// @brief 针对函数进行汇编指令生成，放到.text代码段中
/// @param func 要处理的函数
void CodeGeneratorArm64::genCodeSection(Function * func)
{
    // 寄存器分配以及栈内局部变量的站内地址重新分配
    registerAllocation(func);

    // 获取函数的指令列表
    std::vector<Instruction *> & IrInsts = func->getInterCode().getInsts();
    //std::cout<<IrInsts.size()<<" IR指令数量"<<std::endl;
    for(long unsigned int i = 0; i < IrInsts.size(); i++) {
        //printf("处理指令: %s\n", IrInsts[i]->getName().c_str());
        //printf("指令类型: %d\n", IrInsts[i]->isDead());
        if (IrInsts[i]->isDead()) {
            std::string str;
            IrInsts[i]->toString(str);
            //printf("删除死代码指令: %s\n", str.c_str());
            IrInsts[i]->clearOperands();
            IrInsts.erase(IrInsts.begin() + i);
            i--;
        }
    }
    //std::cout<<IrInsts.size()<<" IR指令数量2"<<std::endl;
    // 汇编指令输出前要确保Label的名字有效
    for (auto inst: IrInsts) {
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            inst->setName(IR_LABEL_PREFIX + std::to_string(labelIndex++));
        }
    }

    // ILOC代码序列
    ILocArm64 iloc(module);

    // 指令选择生成汇编指令
    InstSelectorArm64 instSelector(IrInsts, iloc, func, simpleRegisterAllocator);
    instSelector.setShowLinearIR(this->showLinearIR);
    instSelector.run();

    // 删除无用的Label指令
    //iloc.deleteUsedLabel();
    fprintf(fp, "%s\n", ".text");
    // ILOC代码输出为汇编代码
    fprintf(fp, ".align %d\n", 4);//func->getAlignment());
    fprintf(fp, ".global %s\n", func->getName().c_str());
    fprintf(fp, ".type %s, %%function\n", func->getName().c_str());
    fprintf(fp, "%s:\n", func->getName().c_str());

    // 开启时输出IR指令作为注释
    if (this->showLinearIR) {
        // 输出有关局部变量的注释
        for (auto localVar: func->getVarValues()) {
            std::string str;
            getIRValueStr(localVar, str);
            if (!str.empty()) {
                fprintf(fp, "%s\n", str.c_str());
            }
        }

        // 输出指令关联的临时变量信息
        for (auto inst: func->getInterCode().getInsts()) {
            if (inst->hasResultValue()) {
                std::string str;
                getIRValueStr(inst, str);
                if (!str.empty()) {
                    fprintf(fp, "%s\n", str.c_str());
                }
            }
        }
    }
//std::cout<<IrInsts.size()<<" IR指令数量3"<<std::endl;
    iloc.outPut(fp);
}

/// @brief 寄存器分配
/// @param func 函数指针
void CodeGeneratorArm64::registerAllocation(Function * func)
{
    // 内置函数不需要处理
    if (func->isBuiltin()) {
        return;
    }

    // ARMv8寄存器分配策略：
    // - x0-x7: 参数/临时寄存器（调用者保存）
    // - x8: 间接结果寄存器
    // - x9-x15: 临时寄存器（调用者保存）
    // - x16-x17: 平台寄存器（调用者保存）
    // - x18: 平台保留寄存器
    // - x19-x28: 被调用者保存寄存器
    // - x29: 帧指针
    // - x30: 链接寄存器
    // - sp: 堆栈指针

    std::vector<int32_t> & protectedRegNo = func->getProtectedReg();
    
    // 保护被调用者保存寄存器 (x19-x28)
    //for (int i = 19; i <= 28; i++) {
    //    protectedRegNo.push_back(i);
    //}
    protectedRegNo.push_back(Arm64_TMP_REG_NO);
    // 保护帧指针 (x29) 和链接寄存器 (x30)
    protectedRegNo.push_back(Arm64_FP_REG_NO);
    if (1 || func->getExistFuncCall()) {
        protectedRegNo.push_back(Arm64_LX_REG_NO);//protectedRegNo.push_back(REG_ALLOC_SIMPLE_LX_REG_NO);
    }
#ifndef FUNC_PARAM_STACK_ONE
    // 对于形参的前8个参数默认分配8个局部变量，用于保存前8个寄存器的值
    // 前8个之后的形参直接用实参的地址进行访问。
    // 如果定义宏FUNC_PARAM_STACK_ONE，则前8个参数后的形参也分配局部变量，
    // 需要额外的赋值指令，把实参的值赋值给形参对应的局部变量
    adjustFormalParamStack(func);
#endif

    // 为局部变量和临时变量在栈内分配空间
    stackAlloc(func);

    // 函数形参要求前八个寄存器分配，后面的参数采用栈传递
    //adjustFormalParamInsts(func);

    // 调整函数调用指令，主要是前八个寄存器传值，后面用栈传递
    //adjustFuncCallInsts(func);

#if 0
    // 临时输出调整后的IR指令
    std::string irCodeStr;
    func->toString(irCodeStr);
    std::cout << irCodeStr << std::endl;
#endif
}

/// @brief 寄存器分配前对函数内的指令进行调整，以便方便寄存器分配
/// @param func 要处理的函数
void CodeGeneratorArm64::adjustFormalParamStack(Function * func)
{
    // 函数形参要求前8个寄存器分配，后面的参数采用栈传递
    // 针对后面的栈传递参数，一种处理方式为增加赋值操作，内存赋值给形参，形参再次分配空间
    // 另外一种处理方式，形参直接用实参的地址，也就是变为内存变量
    std::vector<Instruction *> newInsts;

    // 函数形参的前四个参数采用的是寄存器传值，需要把值拷贝到本地栈形参变量上
    auto & params = func->getParams();

    // 因为假定用的R1与R2进行运算，而寄存器传值也用R0到R3
    // 如果先处理了后面的参数，R1与R2的值会被更改。
    for (int k = 0; k < (int) params.size() && k <= 7; k++) {

        // 形参名字无效，则忽略
        if (params[k]->getName().empty()) {
            continue;
        }

        Value * val = params[k];

        // 我们需要确保这是一个FormalParam类型，因为Value基类没有setRegId方法
        if (auto* formalParam = dynamic_cast<FormalParam*>(val)) {
            // 产生赋值指令,R#n寄存器赋值到对应的变量上，放到放到Entry指令的后面
            // 注意R#n寄存器需要事先分配几个Value
            formalParam->setRegId(k);
        }
        // newInsts.push_back(new AssignIRInst(val, RegVal[k]));
    }

    // 形参的前8个采用fp+偏移寻址，后面的sp+偏移寻址实参空间
    // 根据C语言的约定，除前8个外的实参进行值传递，逆序入栈
    int fp_esp = func->getMaxDep() + (int) func->getProtectedReg().size() * 4;
    fp_esp = 0;//(fp_esp + 15) & ~15;  // 16 字节对齐
    for (int k = 8; k < (int) params.size(); k++) {

        Value * val = params[k];

        // 目前假定变量大小都是8字节

        // 形参名字无效，则忽略
        if (params[k]->getName().empty()) {

            // 增加8字节
            fp_esp += 8;

            continue;
        }

#ifdef FUNC_PARAM_STACK_ONE
        // 新建一个内存变量，用于栈传值到内存中
        Value * newVal = new MemValue(BasicType::TYPE_INT);
        newVal->baseRegNo = REG_ALLOC_SIMPLE_FP_REG_NO;
        newVal->setOffset(fp_esp);

        // 新建一个赋值指令
        newInsts.push_back(new AssignIRInst(val, newVal));
#else
        // 确保这是一个FormalParam类型，因为Value基类没有setMemoryAddr方法
        if (auto* formalParam = dynamic_cast<FormalParam*>(val)) {
            formalParam->setMemoryAddr(Arm64_FP_REG_NO, fp_esp);
        }
#endif

        // 增加8字节
        fp_esp += 8;
    }

    // 当前函数的指令列表
    auto & insts = func->getInterCode().getInsts();

    // Entry指令的下一条指令
    //auto pEntryAfterIter = insts.begin() + 1;

    // 逐个插入指令到entry的后面
    if (insts.empty()) {
    insts.insert(insts.end(), newInsts.begin(), newInsts.end());
} else {
    insts.insert(insts.begin() + 1, newInsts.begin(), newInsts.end());
}
    //insts.insert(pEntryAfterIter, newInsts.begin(), newInsts.end());
}

/// @brief 寄存器分配前对函数内的指令进行调整，以便方便寄存器分配
/// @param func 要处理的函数
void CodeGeneratorArm64::adjustFormalParamInsts(Function * func)
{
    // ARMv8 使用 x0-x7 传递前8个参数
    auto & params = func->getParams();

    // 形参的前八个通过寄存器来传值 x0-x7
    for (int k = 0; k < (int) params.size() && k < 8; k++) {
        params[k]->setRegId(k);
    }

    // 栈传递的参数（超过8个的部分）
    int64_t fp_esp = 0;//func->getMaxDep() + (func->getProtectedReg().size() * 8);
    for (int k = 8; k < (int) params.size(); k++) {
        // 每个参数8字节（64位系统）
        // 目前假定变量大小都是8字节。实际要根据类型来计算
        params[k]->setMemoryAddr(Arm64_FP_REG_NO, fp_esp);
        fp_esp += 8;
    }
}

/// @brief 寄存器分配前对函数内的指令进行调整，以便方便寄存器分配
/// @param func 要处理的函数
void CodeGeneratorArm64::adjustFuncCallInsts(Function * func)
{
    // 当前函数的指令列表
    auto & insts = func->getInterCode().getInsts();

    // 函数返回值用 x0 寄存器
    for (auto pIter = insts.begin(); pIter != insts.end(); pIter++) {
        if (Instanceof(callInst, FuncCallInstruction *, *pIter)) {
            // 实参前八个要寄存器传值，其它参数通过栈传递
            int stackOffset = 0;
            for (int32_t k = 8; k < callInst->getOperandsNum(); k++) {
                auto arg = callInst->getOperand(k);

                // 新建一个内存变量，用于栈传值
                LocalVariable * newVal = func->newLocalVarValue(IntegerType::getTypeInt());
                newVal->setMemoryAddr(Arm64_SP_REG_NO, stackOffset);
                stackOffset += 8;  // 每个参数8字节

                Instruction * assignInst = new MoveInstruction(func, newVal, arg);
                callInst->setOperand(k, newVal);

                pIter = insts.insert(pIter, assignInst);
                pIter++;
            }

            // 寄存器传递的参数（前8个）
            for (int k = 0; k < callInst->getOperandsNum() && k < 8; k++) {
                auto arg = callInst->getOperand(k);
                if (arg->getRegId() == k) {
                    continue;  // 寄存器已正确分配
                } else {
                    Instruction * assignInst = new MoveInstruction(func, PlatformArm64::intRegVal[k], callInst->getOperand(k));
                    callInst->setOperand(k, PlatformArm64::intRegVal[k]);
                    pIter = insts.insert(pIter, assignInst);
                    pIter++;
                }
            }

            // 产生ARG指令
            for (int k = 0; k < callInst->getOperandsNum(); k++) {
                auto arg = callInst->getOperand(k);
                pIter = insts.insert(pIter, new ArgInstruction(func, arg));
                pIter++;
            }

            // 处理返回值
            if (callInst->hasResultValue() && callInst->getRegId() != 0) {
                Instruction * assignInst = new MoveInstruction(func, callInst, PlatformArm64::intRegVal[0]);
                pIter = insts.insert(pIter + 1, assignInst);
            }
        }
    }
}

/// @brief 栈空间分配
/// @param func 要处理的函数
void CodeGeneratorArm64::stackAlloc(Function * func)
{
    // 栈偏移从0开始
    int funcCallArgCnt = func->getMaxFuncCallArgCnt() - 8;
    if (funcCallArgCnt < 0) {
        funcCallArgCnt = 0;
    }
    if (funcCallArgCnt > 0) {
        funcCallArgCnt = funcCallArgCnt * 8; // 每个参数8字节
        funcCallArgCnt = (funcCallArgCnt + 15) & ~15;
    }
    int32_t sp_esp = funcCallArgCnt ;  // 每个参数8字节

    // 确保16字节对齐
    auto align16 = [](int size) {
        return (size + 15) & ~15;
    };
    //需补充数组的处理
    // 处理局部变量
    std::vector<LocalVariable *> & vars = func->getVarValues();
    for (auto var: vars) {
        // 对于简单类型的寄存器分配策略，假定临时变量和局部变量都保存在栈中，属于内存
        // 而对于图着色等，临时变量一般是寄存器，局部变量也可能修改为寄存器
        // TODO 考虑如何进行分配使得临时变量尽量保存在寄存器中，作为优化点考虑

        // regId不为-1，则说明该变量分配为寄存器
        // baseRegNo不等于-1，则说明该变量肯定在栈上，属于内存变量，之前肯定已经分配过
        if ((var->getRegId() == -1) && (!var->getMemoryAddr())) {
            if (var->getType()->isArrayType()) {

                int32_t size = var->getType()->getSize();
                
                // ARMv8要求16字节对齐
                //size = (size + 15) & ~15;
                size = 8;
                if(Instanceof(arrType, ArrayType *,var->getType())){
                    //std::cerr<<"大小"<<((ArrayType*)arrType)->getNumElements()<<std::endl;
                    size = 8 * arrType->getNumElements();
                    //printf("数组大小:%d\n", size);
                    if(!size){
                        size = 8;
                    }
                    //std::cerr<<"ss"<<arrType->getElementType()<<std::endl;
                }
                // 设置栈偏移
                var->setMemoryAddr(Arm64_SP_REG_NO, sp_esp);
                	//ldr x0,sp
                    //add x0, x0,#8
                    //str x0, [sp, #sp_esp]
                // 生成指令
                //auto & insts = func->getInterCode().getInsts();
/*
                Instruction * assignInst = new MoveInstruction(func, PlatformArm64::intRegVal[0], PlatformArm64::intRegVal[31]);
                insts.insert(insts.end(),assignInst);
                Instruction * assignInst3 = new MoveInstruction(func, PlatformArm64::intRegVal[1], new ConstInt(8));
                insts.insert(insts.end(),assignInst3);
                Instruction * addInst = new BinaryInstruction(func,IRInstOperator::IRINST_OP_ADD_I, PlatformArm64::intRegVal[0],PlatformArm64::intRegVal[1],IntegerType::getTypeInt());
                //addInst->setLoadRegId(0);
                insts.insert(insts.end(),addInst);
                Instruction * assignInst2 = new MoveInstruction(func, var, addInst);
                insts.insert(insts.end(),assignInst2);
 */               //iloc.inst("ldr", PlatformArm64::regName[Arm64_TMP_REG_NO], PlatformArm64::regName[Arm64_SP_REG_NO]);
                //iloc.inst("add", PlatformArm64::regName[Arm64_TMP_REG_NO], PlatformArm64::regName[Arm64_TMP_REG_NO], std::to_string(size));
                //iloc.inst("str", PlatformArm64::regName[Arm64_TMP_REG_NO], PlatformArm64::regName[Arm64_SP_REG_NO], std::to_string(sp_esp));
                //iloc.inst("str", PlatformArm64::regName[Arm64_SP_REG_NO], PlatformArm64::regName[Arm64_SP_REG_NO], std::to_string(size));
                //std::cerr<<"前"<<sp_esp<<std::endl;
                sp_esp += size;
                //std::cerr<<sp_esp<<std::endl;
            }else{
            int32_t size = var->getType()->getSize();
            
            // ARMv8要求16字节对齐
            //size = (size + 15) & ~15;
            size = 8;
            // 设置栈偏移
            var->setMemoryAddr(Arm64_SP_REG_NO, sp_esp);
            sp_esp += size;
            }
        }
    }

    // 处理指令中的临时变量
    for (auto inst: func->getInterCode().getInsts()) {
        if (inst->hasResultValue() && inst->getRegId() == -1 && !inst->getMemoryAddr()) {
            Type * type = inst->getType();
            
            int32_t size = type->getSize();
            //size = (size + 15) & ~15;
            if(type->isArrayType()){
                ///std::cerr<<"大小"<<((ArrayType*)arrType)->getNumElements()<<std::endl;
                //size = 8 * arrType->getNumElements();
                //std::cerr<<"ss"<<arrType->getElementType()<<std::endl;
                size = 8;//0 * arrType->getSize();
                inst->setMemoryAddr(Arm64_SP_REG_NO, sp_esp);
                //std::cerr<<"前:"<<sp_esp<<std::endl;
                sp_esp += size;
                //std::cerr<<sp_esp<<std::endl;
            }else{
                size = 8;
                inst->setMemoryAddr(Arm64_SP_REG_NO, sp_esp);
                //std::cerr<<"前"<<sp_esp<<std::endl;
                sp_esp += size;
                //std::cerr<<sp_esp<<std::endl;
            }
        }
    }

    // 设置函数的最大栈帧深度，确保16字节对齐
    //printf("Function %s stack depth: %d\n", func->getName().c_str(), sp_esp);
    func->setMaxDep(align16(sp_esp));
}