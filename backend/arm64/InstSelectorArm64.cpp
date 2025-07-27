///
/// @file InstSelectorArm64.cpp
/// @brief 指令选择器-Arm64的实现
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
#include <cstdio>

#include "Common.h"
#include "ILocArm64.h"
#include "InstSelectorArm64.h"
#include "PlatformArm64.h"

#include "PointerType.h"
#include "RegVariable.h"
#include "Function.h"

#include "LabelInstruction.h"
#include "GotoInstruction.h"
#include "FuncCallInstruction.h"
#include "MoveInstruction.h"
#include "LoadInstruction.h"
/// @brief 构造函数
/// @param _irCode 指令
/// @param _iloc ILoc
/// @param _func 函数
InstSelectorArm64::InstSelectorArm64(vector<Instruction *> & _irCode,
                                     ILocArm64 & _iloc,
                                     Function * _func,
                                     SimpleRegisterAllocatorArm64 & allocator)
    : ir(_irCode), iloc(_iloc), func(_func), simpleRegisterAllocator(allocator)
{
    translator_handlers[IRInstOperator::IRINST_OP_ENTRY] = &InstSelectorArm64::translate_entry;
    translator_handlers[IRInstOperator::IRINST_OP_EXIT] = &InstSelectorArm64::translate_exit;

    translator_handlers[IRInstOperator::IRINST_OP_LABEL] = &InstSelectorArm64::translate_label;
    translator_handlers[IRInstOperator::IRINST_OP_GOTO] = &InstSelectorArm64::translate_goto;

    translator_handlers[IRInstOperator::IRINST_OP_ASSIGN] = &InstSelectorArm64::translate_assign;

    translator_handlers[IRInstOperator::IRINST_OP_ADD_I] = &InstSelectorArm64::translate_add_int32;
    translator_handlers[IRInstOperator::IRINST_OP_ADD_F] = &InstSelectorArm64::translate_add_float;
    translator_handlers[IRInstOperator::IRINST_OP_NEG_I] = &InstSelectorArm64::translate_neg_operator;
    translator_handlers[IRInstOperator::IRINST_OP_NEG_F] = &InstSelectorArm64::translate_neg_float_operator;
    translator_handlers[IRInstOperator::IRINST_OP_SUB_I] = &InstSelectorArm64::translate_sub_int32;
    translator_handlers[IRInstOperator::IRINST_OP_SUB_F] = &InstSelectorArm64::translate_sub_float;
    translator_handlers[IRInstOperator::IRINST_OP_MUL_I] = &InstSelectorArm64::translate_mul_int32;
    translator_handlers[IRInstOperator::IRINST_OP_MUL_F] = &InstSelectorArm64::translate_mul_float;
    translator_handlers[IRInstOperator::IRINST_OP_DIV_I] = &InstSelectorArm64::translate_div_int32;
    translator_handlers[IRInstOperator::IRINST_OP_DIV_F] = &InstSelectorArm64::translate_div_float;
    translator_handlers[IRInstOperator::IRINST_OP_MOD_I] = &InstSelectorArm64::translate_mod_int32;
    translator_handlers[IRInstOperator::IRINST_OP_XOR_I] = &InstSelectorArm64::translate_xor_int32;
    //比较指令
    translator_handlers[IRInstOperator::IRINST_OP_GT_I] = &InstSelectorArm64::translate_cmp_int32;
    translator_handlers[IRInstOperator::IRINST_OP_EQ_I] = &InstSelectorArm64::translate_cmp_int32;
    translator_handlers[IRInstOperator::IRINST_OP_GE_I] = &InstSelectorArm64::translate_cmp_int32;
    translator_handlers[IRInstOperator::IRINST_OP_NE_I] = &InstSelectorArm64::translate_cmp_int32;
    translator_handlers[IRInstOperator::IRINST_OP_LE_I] = &InstSelectorArm64::translate_cmp_int32;
    translator_handlers[IRInstOperator::IRINST_OP_LT_I] = &InstSelectorArm64::translate_cmp_int32;
    //逻辑指令？
    translator_handlers[IRInstOperator::IRINST_OP_LOGICAL_AND_I] = &InstSelectorArm64::translate_cmp_int32;
    translator_handlers[IRInstOperator::IRINST_OP_LOGICAL_OR_I] = &InstSelectorArm64::translate_cmp_int32;
    translator_handlers[IRInstOperator::IRINST_OP_LOGICAL_NOT_I] = &InstSelectorArm64::translate_cmp_int32;
    
    //类型转换指令
    translator_handlers[IRInstOperator::IRINST_OP_ITOF] = &InstSelectorArm64::translate_itof;

    //处理数组
    translator_handlers[IRInstOperator::IRINST_OP_GEP] = &InstSelectorArm64::translate_gep;
    translator_handlers[IRInstOperator::IRINST_OP_GEP_FormalParam] = &InstSelectorArm64::translate_gep_formalparam;
    translator_handlers[IRInstOperator::IRINST_OP_STORE] = &InstSelectorArm64::translate_store;
    translator_handlers[IRInstOperator::IRINST_OP_LOAD] = &InstSelectorArm64::translate_load;
    
    translator_handlers[IRInstOperator::IRINST_OP_FUNC_CALL] = &InstSelectorArm64::translate_call;
    translator_handlers[IRInstOperator::IRINST_OP_ARG] = &InstSelectorArm64::translate_arg;
}

///
/// @brief 析构函数
///
InstSelectorArm64::~InstSelectorArm64()
{}

/// @brief 指令选择执行
void InstSelectorArm64::run()
{
    for (auto inst: ir) {

        // 逐个指令进行翻译
        if (!inst->isDead()) {
            translate(inst);
        }
    }
}

/// @brief 指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate(Instruction * inst)
{
    // 操作符
    IRInstOperator op = inst->getOp();
    //printf("Translate: Operator(%d) %s\n", (int) op, inst->getName().c_str());
    map<IRInstOperator, translate_handler>::const_iterator pIter;
    pIter = translator_handlers.find(op);
    if (pIter == translator_handlers.end()) {
        // 没有找到，则说明当前不支持
        printf("Translate: Operator(%d) not support", (int) op);
        return;
    }

    // 开启时输出IR指令作为注释
    if (showLinearIR) {
        outputIRInstruction(inst);
    }

    (this->*(pIter->second))(inst);
}

///
/// @brief 输出IR指令
///
void InstSelectorArm64::outputIRInstruction(Instruction * inst)
{
    std::string irStr;
    inst->toString(irStr);
    if (!irStr.empty()) {
        iloc.comment(irStr);
    }
}

/// @brief NOP翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_nop(Instruction * inst)
{
    (void) inst;
    iloc.nop();
}

/// @brief Label指令指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_label(Instruction * inst)
{
    Instanceof(labelInst, LabelInstruction *, inst);

    iloc.label(labelInst->getName());
}

/// @brief goto指令指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_goto(Instruction * inst)
{
    Instanceof(gotoInst, GotoInstruction *, inst);

    // 无条件跳转
    //iloc.jump(gotoInst->getTarget()->getName());
    if (!gotoInst->getCondition()){//gotoInst->getTrueTarget()  == gotoInst->getFalseTarget()) {
        //无条件跳转
        std::string toLabel = gotoInst->getTarget() ->getName();
        iloc.inst("b", "" + toLabel);
    } else {
        //有条件跳转

        //生成跳转指令
        std::string condop;
        if (gotoInst->fore_cmp_inst_op == IRInstOperator::IRINST_OP_GT_I) {
            condop = "bgt";
        } else if (gotoInst->fore_cmp_inst_op == IRInstOperator::IRINST_OP_GE_I) {
            condop = "bge";
        } else if (gotoInst->fore_cmp_inst_op == IRInstOperator::IRINST_OP_LT_I) {
            condop = "blt";
        } else if (gotoInst->fore_cmp_inst_op == IRInstOperator::IRINST_OP_LE_I) {
            condop = "ble";
        } else if (gotoInst->fore_cmp_inst_op == IRInstOperator::IRINST_OP_NE_I) {
            condop = "bne";
        } else if (gotoInst->fore_cmp_inst_op == IRInstOperator::IRINST_OP_EQ_I) {
            condop = "beq";
        } else {
            //赋值或call函数，比较返回值和0
            //出错
            //printf("不支持的比较类别:%d\n", (int)gotoInst->fore_cmp_inst_op);
            //printf("%s",PlatformArm64::regName[gotoInst->fore_instId].c_str());
            //iloc.inst("cmp", PlatformArm64::regName[gotoInst->fore_instId], "#0");
            //iloc.inst("指令类型不支持", std::to_string((int)gotoInst->fore_cmp_inst_op));
            
            int32_t base = -1;
            int64_t offset = 0;
            
            //printf("condition: %s, base: %d, offset: %ld\n",
            //       gotoInst->getCondition()->getName().c_str(), base, offset);
            if(gotoInst->getCondition()->getMemoryAddr(&base, &offset)){//gotoInst->fore_inst->getMemoryAddr(&base, &offset)) {
                // 如果是内存变量
                iloc.inst("ldr", "x0", "[" + PlatformArm64::regName[base] + ", #" + std::to_string(offset) + "]");
            } else {
                // 如果是寄存器变量
                //iloc.inst("mov", "x0", PlatformArm64::regName[gotoInst->fore_inst->getRegId()]);
            }
            //iloc.leaStack(0,gotoInst->fore_inst);
            iloc.inst("cmp", "x0", "#0");
            condop = "bne";
            //printf("不支持的比较类别:%d\n", (int)gotoInst->fore_cmp_inst_op);
        }

        //两个跳转位置
        std::string truelabel = gotoInst->getTrueTarget() ->getName();
        std::string falselabel = gotoInst->getFalseTarget()->getName();
        iloc.inst(condop, "" + truelabel);
        iloc.inst("b", "" + falselabel);
    }
}

/// @brief 函数入口指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_entry(Instruction * inst)
{
    auto & protectedRegNo = func->getProtectedReg();
    auto & protectedRegStr = func->getProtectedRegStr();
    int funcCallArgCnt = func->getMaxFuncCallArgCnt() - 8;
    if (funcCallArgCnt < 0) {
        funcCallArgCnt = 0;
    }
    //printf("Func %s, MaxFuncCallArgCnt: %d\n", func->getName().c_str(), funcCallArgCnt);
    if(funcCallArgCnt > 0) {
        // 确保16字节对齐
        funcCallArgCnt = (funcCallArgCnt)*8; // 每个参数8字节
        funcCallArgCnt = (funcCallArgCnt + 15) & ~15;
    //    iloc.inst("sub", "sp", "sp", "#" + std::to_string(funcCallArgCnt));
        //iloc.inst("add", "x16", "sp", "#" + std::to_string(0)); // 设置参数基址寄存器
        //printf("Alloc stack size: %d\n", funcCallArgCnt);
    }
    // 1. 首先保存帧指针和返回地址 (x29, x30)
    //iloc.inst("stp", "x29, x30", "[sp, #-16]!");
    //protectedRegStr = "x29, x30";  // 必须记录
    bool first = false; // 已添加第一对

    // 2. 计算需要额外保存的寄存器
    int regCount = protectedRegNo.size();
    int saveSize = (regCount % 2 == 0) ? regCount/2*16 : (regCount/2+1)*16;

    // 3. 一次性分配所有寄存器空间
    if (saveSize > 0) {
        iloc.inst("sub", "sp", "sp", "#" + std::to_string(saveSize));
    }

    // 4. 成对保存寄存器（从低地址向高地址存储）
    for (int i = 0; i < regCount; i += 2) {
        std::string offset = "#" + std::to_string(i/2 * 16);
        if (i + 1 < regCount) {
            iloc.inst("stp", 
                PlatformArm64::regName[protectedRegNo[i]] + ", " + 
                PlatformArm64::regName[protectedRegNo[i+1]],
                "[sp, " + offset + "]");
            
            if (first) {
                protectedRegStr += PlatformArm64::regName[protectedRegNo[i]] + ", " + 
                                  PlatformArm64::regName[protectedRegNo[i+1]];
            } else {
                protectedRegStr = ", " +PlatformArm64::regName[protectedRegNo[i]] + ", " + 
                                  PlatformArm64::regName[protectedRegNo[i+1]];
                first = true;
            }
        } else {
            iloc.inst("str", 
                PlatformArm64::regName[protectedRegNo[i]],
                "[sp, " + offset + "]");
            
            if (first) {
                protectedRegStr +=  PlatformArm64::regName[protectedRegNo[i]];
            } else {
                protectedRegStr = ", " + PlatformArm64::regName[protectedRegNo[i]];
            }
        }
    }
    iloc.inst("add", "x29", "sp", to_string(saveSize));  // mov x29, sp

    iloc.allocStack(func, Arm64_TMP_REG_NO);
    /*
    // 5. 设置帧指针
    iloc.inst("add", "x29", "sp", "#0");  // mov x29, sp

    // 6. 分配局部变量空间和函数调用参数空间
    // 计算函数调用参数空间 (Arm64中超过8个参数需栈传参)
    int funcCallArgCnt = func->getMaxFuncCallArgCnt();
    int argSpillSize = (funcCallArgCnt > 8) ? (funcCallArgCnt - 8) * 8 : 0;
    
    // 计算局部变量空间
    int localSize = func->getMaxDep();
    
    // 总栈空间大小
    int totalSize = localSize + argSpillSize;
    
    if (totalSize > 0) {
        // 确保16字节对齐
        int alignedSize = (totalSize + 15) & ~15;
        iloc.inst("sub", "sp", "sp", "#" + std::to_string(alignedSize));
        
        // 设置函数调用参数基址 (sp指向参数区的起始位置)
        if (argSpillSize > 0) {
            // 计算参数区在栈中的偏移：局部变量区大小
            iloc.inst("add", "x16", "sp", "#" + std::to_string(localSize));
            // 保存到特定寄存器或栈位置，供后续使用
            // (这里使用x16作为临时寄存器，实际应根据调用约定选择)
            func->setCallArgBaseReg("x16");
        }
    }
        */
}

/// @brief 函数出口指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_exit(Instruction * inst)
{
    if (inst->getOperandsNum()) {
        // 存在返回值
        Value * retVal = inst->getOperand(0);

        // 赋值给寄存器x0
        iloc.load_var(0, retVal);
    }

    // 恢复栈指针
    //iloc.inst("mov", "sp", "x29");
    
    // 恢复帧指针和链接寄存器
    //iloc.inst("ldp", "x29, x30", "[sp], #16");
        // 恢复栈空间
    //iloc.leaStack(Arm64_FP_REG_NO, Arm64_FP_REG_NO, func->getMaxDep());
    /**/
    // 超过8个的函数调用参数个数，多余8个，则需要栈传值
    int funcCallArgCnt = func->getMaxFuncCallArgCnt() - 8;
    if (funcCallArgCnt < 0) {
        funcCallArgCnt = 0;
    }
    if (funcCallArgCnt > 0) {
        funcCallArgCnt = funcCallArgCnt * 8; // 每个参数8字节
        funcCallArgCnt = (funcCallArgCnt + 15) & ~15;
    }
    // ARMv8 栈帧分配
    int stackSize = func->getMaxDep();
    
    stackSize += funcCallArgCnt ; // 函数调用参数空间
    // 确保16字节对齐
    if (stackSize % 16 != 0) {
        stackSize = (stackSize + 15) & ~15;
    }
    int off = stackSize; // 偏移量
    int rs_reg_no = Arm64_SP_REG_NO; // 
    int base_reg_no = Arm64_SP_REG_NO; // 帧指针寄存
    if (off == 0) {
        // add x8, x29, #0 可以直接 mov
        iloc.mov_reg(rs_reg_no, base_reg_no);
    } else if (PlatformArm64::isDisp(off)) {
        // add x8, x29, #16
        iloc.inst("add", PlatformArm64::regName[Arm64_SP_REG_NO], PlatformArm64::regName[Arm64_SP_REG_NO], "#" + std::to_string(off));
    } else {
        // 大偏移
        iloc.load_imm(Arm64_TMP_REG_NO ,off);
        // add x8, x29, x8
        iloc.inst("add", PlatformArm64::regName[Arm64_SP_REG_NO], PlatformArm64::regName[Arm64_SP_REG_NO], PlatformArm64::regName[Arm64_TMP_REG_NO]);
    }
    //iloc.inst("add",
    //          PlatformArm64::regName[Arm64_FP_REG_NO],
    //          PlatformArm64::regName[Arm64_FP_REG_NO],
    //          iloc.toStr(func->getMaxDep()));

    //iloc.inst("mov", "sp", PlatformArm64::regName[Arm64_FP_REG_NO]);
    // 恢复保护的寄存器
    auto & protectedRegNo = func->getProtectedReg();
    //bool first = true; // 是否是第一次恢复寄存器
    //fp一定会被恢复
    for (size_t i = 0; i < protectedRegNo.size(); i += 2) {//for (int i = protectedRegNo.size() - 1; i >= 0; i -= 2) {
        if(i+1 >= protectedRegNo.size()) {
                // 奇数个寄存器
                iloc.inst("ldr", 
                      PlatformArm64::regName[protectedRegNo[i]],
                      "[sp,#0]");
                //iloc.comment("Restore protected registers: " + PlatformArm64::regName[protectedRegNo[i]]);
        }else{
            // 成对恢复
            iloc.inst("ldp", 
                      PlatformArm64::regName[protectedRegNo[i]] + ", " + 
                      PlatformArm64::regName[protectedRegNo[i+1]],
                      "[sp,#0]");
        }
        iloc.inst("add", "sp", "sp", "#16");
    }
    
    // 返回指令
    iloc.inst("ret");
}

/// @brief 赋值指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_assign(Instruction * inst)
{
    Value * result = inst->getOperand(0);
    Value * arg1 = inst->getOperand(1);
    //转化inst为moveInstruction类型的指令
    MoveInstruction * moveInst = dynamic_cast<MoveInstruction *>(inst);
    int opType = moveInst->getOpType();
    //opType = 0;
    //printf("Translate assign: %s = %s, opType=%d\n", result->getName().c_str(), arg1->getName().c_str(), opType);
    //printf("Translate assign: %d , %d\n", result->getRegId(), arg1->getRegId());
    int32_t arg1_regId = arg1->getRegId();
    int32_t result_regId = result->getRegId();
    if(((Instruction*)arg1)->getOp()==IRInstOperator::IRINST_OP_LOAD){
        //arg1_regId = Arm64_TMP_REG_NO;
        
    }
    if (arg1_regId != -1) {
        // 寄存器 => 内存/寄存器
        iloc.store_var(arg1_regId, result, Arm64_TMP_REG_NO);
        
    } else if (result_regId != -1) {
        // 内存变量 => 寄存器
        if(opType == 3){//直接加载地址
            int32_t var_baseRegId = -1;
            int64_t var_offset = -1;
            bool result1 = arg1->getMemoryAddr(&var_baseRegId, &var_offset);
            if (!result1) {
                minic_log(LOG_ERROR, "BUG");
            }
            //printf("Load address: %s, baseRegId=%d, offset=%ld\n", result->getName().c_str(), var_baseRegId, var_offset);
            std::string addr_reg_name = PlatformArm64::regName[var_baseRegId];
            iloc.inst("mov", PlatformArm64::regName[result_regId],   addr_reg_name );
            //iloc.inst("add", PlatformArm64::regName[result_regId], PlatformArm64::regName[result_regId]  ,"#" + std::to_string(var_offset) );

            //emit("ldr", result_regId, "[" + base + "]");
            //load_base(result_regId, var_baseRegId, var_offset);
        }else{
            iloc.load_var(result_regId, arg1);
        }
    } else if (opType == 2) {
    // 数组取值 - 使用寄存器分配器
    int32_t addr_reg = simpleRegisterAllocator.Allocate();
    iloc.load_var(addr_reg, arg1);  // 加载数组元素地址
    
    // 从地址加载实际值到同一寄存器
    std::string addr_reg_name = PlatformArm64::regName[addr_reg];
    iloc.inst("ldr", addr_reg_name, "[" + addr_reg_name + "]");
    
    // 存储结果值
    iloc.store_var(addr_reg, result, Arm64_TMP_REG_NO);
    simpleRegisterAllocator.free(addr_reg);
}else if (opType == 1) {
    // 数组赋值 - 使用两个临时寄存器
    int32_t val_reg = simpleRegisterAllocator.Allocate();
    int32_t addr_reg = simpleRegisterAllocator.Allocate();
    
    iloc.load_var(val_reg, arg1);  // 加载要存储的值
    iloc.load_var(addr_reg, result); // 加载数组元素地址
    
    std::string val_reg_name = PlatformArm64::regName[val_reg];
    std::string addr_reg_name = PlatformArm64::regName[addr_reg];
    iloc.inst("str", val_reg_name, "[" + addr_reg_name + "]");
    
    simpleRegisterAllocator.free(val_reg);
    simpleRegisterAllocator.free(addr_reg);
}else if (opType == 3) {
        // 存储地址操作
        // 直接存储地址到结果变量
        int32_t var_baseRegId = -1;
        int64_t var_offset = -1;
        // 直接存储地址到结果变量
        int32_t var_baseRegId2 = -1;
        int64_t var_offset2 = -1;
        bool result1 = arg1->getMemoryAddr(&var_baseRegId, &var_offset);
        bool result2 = result->getMemoryAddr(&var_baseRegId2, &var_offset2);
        if(result1&&result2){
            //iloc.inst("mov", PlatformArm64::regName[Arm64_TMP_REG_NO], PlatformArm64::regName[Arm64_SP_REG_NO]);
            //iloc.inst("add", PlatformArm64::regName[Arm64_TMP_REG_NO], PlatformArm64::regName[Arm64_TMP_REG_NO], std::to_string(var_offset));
            iloc.inst("str", PlatformArm64::regName[Arm64_TMP_REG_NO], "["+PlatformArm64::regName[Arm64_SP_REG_NO]+",#"+std::to_string(var_offset2)+"]");
        }//iloc.inst("str", PlatformArm64::regName[Arm64_SP_REG_NO], PlatformArm64::regName[Arm64_SP_REG_NO], std::to_string(size));
        //iloc.store_var(arg1->getRegId(), result, Arm64_TMP_REG_NO);
}else {
        // 内存变量 => 内存变量
        int32_t temp_regno = simpleRegisterAllocator.Allocate();

        // arg1 -> 临时寄存器
        iloc.load_var(temp_regno, arg1);

        // 临时寄存器 -> result
        iloc.store_var(temp_regno, result, Arm64_TMP_REG_NO);

        simpleRegisterAllocator.free(temp_regno);
    }
}
/// @brief neg操作指令翻译成Arm64汇编
/// @param inst IR指令
/// @param operator_name 操作码
/// @param rs_reg_no 结果寄存器号
/// @param op1_reg_no 源操作数1寄存器号
void InstSelectorArm64::translate_neg_operator(Instruction * inst)
{
    Value * rs = inst;
    Value * arg2 = inst->getOperand(0);
    Value * arg1 = new ConstInt(0);
    //printf("Translate neg: %s = -%s\n", rs->getName().c_str(), arg2->getName().c_str());
    int rs_reg_no = REG_ALLOC_SIMPLE_DST_REG_NO;
    int op1_reg_no = REG_ALLOC_SIMPLE_SRC1_REG_NO;
    int op2_reg_no = REG_ALLOC_SIMPLE_SRC2_REG_NO;
    std::string arg1_reg_name, arg2_reg_name;
    int arg1_reg_no = arg1->getRegId(), arg2_reg_no = arg2->getRegId();

    // 看arg1是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg1_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op1_reg_no, arg1);
    } else if (arg1_reg_no != op1_reg_no) {
        // 已分配的操作数1的寄存器和操作数2的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  3    -1   有问题
        // 实际寄存器  3    3    有问题
        // 实际寄存器  3    4    无问题
        if ((arg1_reg_no == op2_reg_no) && ((arg2_reg_no == -1) || (arg2_reg_no == op2_reg_no))) {
            iloc.mov_reg(op1_reg_no, arg1_reg_no);
        } else {
            op1_reg_no = arg1_reg_no;
        }
    }

    arg1_reg_name = PlatformArm64::regName[op1_reg_no];

    // 看arg2是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg2_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op2_reg_no, arg2);
    } else if (arg2_reg_no != op2_reg_no) {
        // 已分配的操作数2的寄存器和操作数1的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  -1   2   有问题
        // 实际寄存器  2    2    有问题
        // 实际寄存器  4    2    无问题
        if ((arg2_reg_no == op1_reg_no) && ((arg1_reg_no == -1) || (arg1_reg_no == op1_reg_no))) {
            iloc.mov_reg(op2_reg_no, arg2_reg_no);
        } else {
            op2_reg_no = arg2_reg_no;
        }
    }

    arg2_reg_name = PlatformArm64::regName[op2_reg_no];

    // 看结果变量是否是寄存器，若不是则采用参数指定的寄存器rs_reg_name
    if (rs->getRegId() != -1) {
        rs_reg_no = rs->getRegId();
    } else {//if (rs->isTemp()) {
        // 临时变量
        rs->setLoadRegId(rs_reg_no);
    }

    std::string rs_reg_name = PlatformArm64::regName[rs_reg_no];

    iloc.inst("sub", rs_reg_name, arg1_reg_name, arg2_reg_name);

    // 结果不是寄存器，则需要把rs_reg_name保存到结果变量中
    if (rs->getRegId() == -1) {
        // r8 -> rs 可能用到r9
        iloc.store_var(rs_reg_no, rs, op2_reg_no);
    }
}
/// @brief 浮点数取负操作指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_neg_float_operator(Instruction * inst)
{
    Value * rs = inst;
    Value * arg = inst->getOperand(0);
    
    int rs_reg_no = REG_ALLOC_SIMPLE_DST_REG_NO;
    int op_reg_no = REG_ALLOC_SIMPLE_SRC1_REG_NO;
    
    std::string arg_reg_name;
    int arg_reg_no = arg->getRegId();

    // 看arg是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg_reg_no == -1) {
        iloc.load_var(op_reg_no, arg);
    } else {
        op_reg_no = arg_reg_no;
    }

    arg_reg_name = PlatformArm64::regName[op_reg_no];

    // 看结果变量是否是寄存器，若不是则采用参数指定的寄存器rs_reg_name
    if (rs->getRegId() != -1) {
        rs_reg_no = rs->getRegId();
    } else {
        // 临时变量
        rs->setLoadRegId(rs_reg_no);
    }

    std::string rs_reg_name = PlatformArm64::regName[rs_reg_no];

    // 使用fneg指令对浮点数取负
    iloc.inst("fneg", rs_reg_name, arg_reg_name);

    // 结果不是寄存器，则需要把rs_reg_name保存到结果变量中
    if (rs->getRegId() == -1) {
        iloc.store_var(rs_reg_no, rs, op_reg_no);
    }
}
/// @brief 二元操作指令翻译成Arm64汇编
/// @param inst IR指令
/// @param operator_name 操作码
/// @param rs_reg_no 结果寄存器号
/// @param op1_reg_no 源操作数1寄存器号
/// @param op2_reg_no 源操作数2寄存器号
void InstSelectorArm64::translate_two_operator(Instruction * inst, string operator_name)
{
    Value * result = inst;
    Value * arg1 = inst->getOperand(0);
    Value * arg2 = inst->getOperand(1);

    int32_t arg1_reg_no = arg1->getRegId();
    int32_t arg2_reg_no = arg2->getRegId();
    int32_t result_reg_no = inst->getRegId();
    int32_t load_result_reg_no, load_arg1_reg_no, load_arg2_reg_no;

    // 看arg1是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg1_reg_no == -1) {
        // 分配一个寄存器
        load_arg1_reg_no = simpleRegisterAllocator.Allocate(arg1);
        iloc.load_var(load_arg1_reg_no, arg1);
    } else {
        load_arg1_reg_no = arg1_reg_no;
    }

    // 看arg2是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg2_reg_no == -1) {
        // 分配一个寄存器
        load_arg2_reg_no = simpleRegisterAllocator.Allocate(arg2);
        iloc.load_var(load_arg2_reg_no, arg2);
    } else {
        load_arg2_reg_no = arg2_reg_no;
    }

    // 看结果变量是否是寄存器，若不是则需要分配一个新的寄存器来保存运算的结果
    if (result_reg_no == -1) {
        // 分配一个寄存器，用于暂存结果
        load_result_reg_no = simpleRegisterAllocator.Allocate(result);
    } else {
        load_result_reg_no = result_reg_no;
    }

    // 执行二元操作
    iloc.inst(operator_name,
              PlatformArm64::regName[load_result_reg_no],
              PlatformArm64::regName[load_arg1_reg_no],
              PlatformArm64::regName[load_arg2_reg_no]);

    // 结果不是寄存器，则需要把结果保存到内存变量中
    if (result_reg_no == -1) {
        iloc.store_var(load_result_reg_no, result, Arm64_TMP_REG_NO);
    }

    // 释放寄存器
    simpleRegisterAllocator.free(arg1);
    simpleRegisterAllocator.free(arg2);
    simpleRegisterAllocator.free(result);
}

/// @brief 整数加法指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_add_int32(Instruction * inst)
{
    translate_two_operator(inst, "add");
}

/// @brief 浮点数加法指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_add_float(Instruction * inst)
{
    translate_two_operator(inst, "fadd");
}

/// @brief 整数减法指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_sub_int32(Instruction * inst)
{
    translate_two_operator(inst, "sub");
}

/// @brief 浮点数减法指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_sub_float(Instruction * inst)
{
    translate_two_operator(inst, "fsub");
}

/// @brief 整数乘法指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_mul_int32(Instruction * inst)
{
    translate_two_operator(inst, "mul");
}

/// @brief 浮点数乘法指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_mul_float(Instruction * inst)
{
    translate_two_operator(inst, "fmul");
}

/// @brief 整数除法指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_div_int32(Instruction * inst)
{
    translate_two_operator(inst, "sdiv");
}

/// @brief 浮点数除法指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_div_float(Instruction * inst)
{
    translate_two_operator(inst, "fdiv");
}

/// @brief 整数取模指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_mod_int32(Instruction * inst)
{
    // 取模
    Value * rs = inst;
    Value * arg1 = inst->getOperand(0);
    Value * arg2 = inst->getOperand(1);

    int rs_reg_no = REG_ALLOC_SIMPLE_DST_REG_NO;
    int op1_reg_no = REG_ALLOC_SIMPLE_SRC1_REG_NO;
    int op2_reg_no = REG_ALLOC_SIMPLE_SRC2_REG_NO;

    std::string arg1_reg_name, arg2_reg_name;
    int arg1_reg_no = arg1->getRegId(), arg2_reg_no = arg2->getRegId();

    // 看arg1是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg1_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op1_reg_no, arg1);
    } else if (arg1_reg_no != op1_reg_no) {
        // 已分配的操作数1的寄存器和操作数2的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  3    -1   有问题
        // 实际寄存器  3    3    有问题
        // 实际寄存器  3    4    无问题
        if ((arg1_reg_no == op2_reg_no) && ((arg2_reg_no == -1) || (arg2_reg_no == op2_reg_no))) {
            iloc.mov_reg(op1_reg_no, arg1_reg_no);
        } else {
            op1_reg_no = arg1_reg_no;
        }
    }

    arg1_reg_name = PlatformArm64::regName[op1_reg_no];

    // 看arg2是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg2_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op2_reg_no, arg2);
    } else if (arg2_reg_no != op2_reg_no) {
        // 已分配的操作数2的寄存器和操作数1的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  -1   2   有问题
        // 实际寄存器  2    2    有问题
        // 实际寄存器  4    2    无问题
        if ((arg2_reg_no == op1_reg_no) && ((arg1_reg_no == -1) || (arg1_reg_no == op1_reg_no))) {
            iloc.mov_reg(op2_reg_no, arg2_reg_no);
        } else {
            op2_reg_no = arg2_reg_no;
        }
    }

    arg2_reg_name = PlatformArm64::regName[op2_reg_no];

    // 看结果变量是否是寄存器，若不是则采用参数指定的寄存器rs_reg_name
    if (rs->getRegId() != -1) {
        rs_reg_no = rs->getRegId();
    } else {//if (rs->isTemp()) {
        // 临时变量
        rs->setLoadRegId(rs_reg_no);
    }

    //临时存一个
    std::string tmp_reg_name = PlatformArm64::regName[7];
    std::string rs_reg_name = PlatformArm64::regName[rs_reg_no];

    //除法、乘法、减法完成取模操作
    iloc.inst("sdiv", tmp_reg_name, arg1_reg_name, arg2_reg_name);
    iloc.inst("mul", tmp_reg_name, tmp_reg_name, arg2_reg_name);
    iloc.inst("sub", rs_reg_name, arg1_reg_name, tmp_reg_name);

    // 结果不是寄存器，则需要把rs_reg_name保存到结果变量中
    if (rs->getRegId() == -1) {
        // r8 -> rs 可能用到r9
        iloc.store_var(rs_reg_no, rs, op2_reg_no);
    }
}



void InstSelectorArm64::translate_xor_int32(Instruction* inst) {
    static const char *cmpMap[] = {"eq", "ne", "gt", "le", "ge", "lt"};
    auto* lhs = dynamic_cast<Instruction*>(inst->getOperand(0));
    auto* rhs = dynamic_cast<ConstInt*>(inst->getOperand(1));
    
    // 检查是否满足特殊优化条件：XOR 1 与比较指令
    if (rhs && lhs && rhs->getVal() == 1) {
        IRInstOperator op = lhs->getOp();
        
        // 检查是否是比较操作符 (LT到NE之间的操作码)
        const int minCmp = static_cast<int>(IRInstOperator::IRINST_OP_LT_I);
        const int maxCmp = static_cast<int>(IRInstOperator::IRINST_OP_NE_I);
        
        if ((int)op >= minCmp && (int)op <= maxCmp) {
            // 计算条件反转映射
            const int base = static_cast<int>(IRInstOperator::IRINST_OP_EQ_I);
            const int index = (static_cast<int>(op) - base) ^ 1;
            const char* condStr = cmpMap[index];
            
            // 处理寄存器分配
            const int regId = inst->getRegId();
            const int allocReg = (regId == -1) 
                                ? simpleRegisterAllocator.Allocate(inst) 
                                : regId;
            
            // 生成反转条件的cset指令
            iloc.inst("cset", PlatformArm64::regName[allocReg], condStr);
            
            // 处理寄存器存储
            if (regId == -1) {
                iloc.store_var(allocReg, inst, Arm64_FP_REG_NO);
            }
            
            simpleRegisterAllocator.free(inst);
            return;
        }
    }
    
    // 通用情况：生成标准eor指令
    translate_two_operator(inst, "eor");
}

// 已在上面定义了translate_sub_int32函数，这里不再重复定义

/// @brief cmp比较指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_cmp_int32(Instruction * inst)
{
    //一条比较指令

    //取出两个源操作数
    Value * arg1 = inst->getOperand(0);
    Value * arg2 = inst->getOperand(1);

    //默认的寄存器号
    int rs_reg_no = REG_ALLOC_SIMPLE_DST_REG_NO;
    int op1_reg_no = REG_ALLOC_SIMPLE_SRC1_REG_NO;
    int op2_reg_no = REG_ALLOC_SIMPLE_SRC2_REG_NO;

    //选择特定的寄存器号
    std::string arg1_reg_name, arg2_reg_name;
    int arg1_reg_no = arg1->getRegId(), arg2_reg_no = arg2->getRegId();

    // 看arg1是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg1_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op1_reg_no, arg1);
    } else if (arg1_reg_no != op1_reg_no) {
        // 已分配的操作数1的寄存器和操作数2的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  3    -1   有问题
        // 实际寄存器  3    3    有问题
        // 实际寄存器  3    4    无问题
        if ((arg1_reg_no == op2_reg_no) && ((arg2_reg_no == -1) || (arg2_reg_no == op2_reg_no))) {
            iloc.mov_reg(op1_reg_no, arg1_reg_no);
        } else {
            op1_reg_no = arg1_reg_no;
        }
    }

    arg1_reg_name = PlatformArm64::regName[op1_reg_no];

    // 看arg2是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg2_reg_no == -1) {
        // arg1 -> r8
        iloc.load_var(op2_reg_no, arg2);
    } else if (arg2_reg_no != op2_reg_no) {
        // 已分配的操作数2的寄存器和操作数1的缺省寄存器一致，这样会使得操作数2的值设置到一个寄存器上
        // 缺省寄存器  2    3
        // 实际寄存器  -1   2   有问题
        // 实际寄存器  2    2    有问题
        // 实际寄存器  4    2    无问题
        if ((arg2_reg_no == op1_reg_no) && ((arg1_reg_no == -1) || (arg1_reg_no == op1_reg_no))) {
            iloc.mov_reg(op2_reg_no, arg2_reg_no);
        } else {
            op2_reg_no = arg2_reg_no;
        }
    }

    arg2_reg_name = PlatformArm64::regName[op2_reg_no];

    std::string rs_reg_name = PlatformArm64::regName[rs_reg_no];

    iloc.inst("cmp", arg1_reg_name, arg2_reg_name);
}


/// @brief 函数调用指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_gep(Instruction *inst) {
    Value *arg1 = inst->getOperand(0);
    Value *arg2 = inst->getOperand(1);

    int32_t baseReg = -1;
    int64_t baseOff;
    if (!arg1->getMemoryAddr(&baseReg, &baseOff)) {
        baseReg = arg1->getRegId();
        baseOff = 0;
    }
    Instanceof(off, ConstInt*, arg2);
    uint32_t l = ((ArrayType*)(inst->getType()))->getElementType()->getSize();
    l = 8;//暂时默认为8
    if (off) {
        if (dynamic_cast<GlobalVariable*>(arg1)) {
            iloc.lea_var(Arm64_TMP_REG_NO, arg1);
            baseReg = Arm64_TMP_REG_NO;
            //iloc.inst("add", PlatformArm64::regName[baseReg], PlatformArm64::regName[baseReg], "#"+std::to_string(baseOff)+"");
            if(PlatformArm64::isDisp(off->getVal() * l)){
                //直接加偏移量
                iloc.inst("add", PlatformArm64::regName[baseReg], PlatformArm64::regName[baseReg], "#"+std::to_string(off->getVal() * l)+"");
            }else{
                //先加载偏移量到临时寄存器
                iloc.load_imm(Arm64_TMP_REG_NO4, off->getVal() * l);
                //再加偏移量
                iloc.inst("add", PlatformArm64::regName[baseReg], PlatformArm64::regName[baseReg], PlatformArm64::regName[Arm64_TMP_REG_NO4]);
            }
        }else{
            baseReg = Arm64_TMP_REG_NO;
            
            //iloc.inst("ldr",PlatformArm64::regName[baseReg],"[sp,#"+std::to_string(baseOff)+"]");
            iloc.inst("mov",PlatformArm64::regName[baseReg],"sp");
            if (PlatformArm64::isDisp(baseOff)){
                
                iloc.inst("add",PlatformArm64::regName[baseReg],PlatformArm64::regName[baseReg],"#"+std::to_string(baseOff)+"");
            }else{
                iloc.load_imm(Arm64_TMP_REG_NO4, baseOff);
                iloc.inst("add",PlatformArm64::regName[baseReg],PlatformArm64::regName[baseReg],PlatformArm64::regName[Arm64_TMP_REG_NO4]);
            }
            if(PlatformArm64::isDisp(off->getVal() * l)){
                //直接加偏移量
                iloc.inst("add",PlatformArm64::regName[baseReg],PlatformArm64::regName[baseReg],"#"+std::to_string(off->getVal() * l)+"");
            }else{
                //先加载偏移量到临时寄存器
                iloc.load_imm(Arm64_TMP_REG_NO4, off->getVal() * l);
                //再加偏移量
                iloc.inst("add",PlatformArm64::regName[baseReg],PlatformArm64::regName[baseReg],PlatformArm64::regName[Arm64_TMP_REG_NO4]);
            }
            //iloc.inst("add",PlatformArm64::regName[baseReg],PlatformArm64::regName[baseReg],"#"+std::to_string(off->getVal() * l)+"");
        }
        inst->setMemoryAddr(baseReg, off->getVal() * l);//baseOff + off->getVal() * l);
        inst->setLoadRegId(baseReg);
    } else {
        // TODO
        // %t1 = getelemptr [3xi32] %l0, %l1
        // // mul l1, l1, lx
        //printf("reg1: %d\n", baseReg);
        if (baseReg == -1) {
            baseReg = Arm64_TMP_REG_NO;
            if (dynamic_cast<GlobalVariable*>(arg1)) {
                iloc.lea_var(baseReg, arg1);
            } else {
                iloc.load_var(baseReg, arg1);
            }
        }
        int32_t reg2 = arg2->getRegId();
        //printf("reg2: %d\n", reg2);
        if (reg2 == -1) {
            reg2 = Arm64_TMP_REG_NO2;
            iloc.load_var(reg2, arg2);
        }
        if (__builtin_popcount(l) == 1) {
            if(baseReg!=Arm64_SP_REG_NO){
                iloc.inst("add", PlatformArm64::regName[Arm64_TMP_REG_NO2], PlatformArm64::regName[baseReg], PlatformArm64::regName[reg2]+",lsl "+to_string(__builtin_ctz(l)));
            }else{
                //iloc.inst("ldr",PlatformArm64::regName[Arm64_TMP_REG_NO],"[sp,#"+to_string(baseOff)+"]");
                iloc.inst("mov",PlatformArm64::regName[Arm64_TMP_REG_NO],"sp");
                if (PlatformArm64::isDisp(baseOff)){
                    iloc.inst("add", PlatformArm64::regName[Arm64_TMP_REG_NO], PlatformArm64::regName[Arm64_TMP_REG_NO], "#"+std::to_string(baseOff)+"");
                }else{
                    iloc.load_imm(Arm64_TMP_REG_NO4, baseOff);
                    iloc.inst("add",PlatformArm64::regName[Arm64_TMP_REG_NO],PlatformArm64::regName[Arm64_TMP_REG_NO],PlatformArm64::regName[Arm64_TMP_REG_NO4]);
                }
                
                //iloc.inst("add", PlatformArm64::regName[Arm64
                iloc.inst("add", PlatformArm64::regName[Arm64_TMP_REG_NO2], PlatformArm64::regName[Arm64_TMP_REG_NO], PlatformArm64::regName[reg2]+" ,lsl "+to_string(__builtin_ctz(l)));
            }
        } else {
            if(baseReg != Arm64_SP_REG_NO) {
                iloc.inst("mov", PlatformArm64::regName[Arm64_TMP_REG_NO], "#"+to_string(l));
                iloc.inst("madd", PlatformArm64::regName[Arm64_TMP_REG_NO2],
                        PlatformArm64::regName[reg2],
                        PlatformArm64::regName[Arm64_TMP_REG_NO]
                        +","+PlatformArm64::regName[baseReg]);
            } else {
                //iloc.inst("ldr",PlatformArm64::regName[Arm64_TMP_REG_NO2],"[sp,#"+to_string(baseOff)+"]");
                iloc.inst("mov",PlatformArm64::regName[Arm64_TMP_REG_NO],"sp");
                if (PlatformArm64::isDisp(baseOff)){
                    iloc.inst("add",PlatformArm64::regName[Arm64_TMP_REG_NO2],PlatformArm64::regName[Arm64_TMP_REG_NO],"#"+std::to_string(baseOff)+"");
                }else{
                    iloc.load_imm(Arm64_TMP_REG_NO4, baseOff);
                    iloc.inst("add",PlatformArm64::regName[Arm64_TMP_REG_NO2],PlatformArm64::regName[Arm64_TMP_REG_NO],PlatformArm64::regName[Arm64_TMP_REG_NO4]);
                }//iloc.inst("add", PlatformArm64::regName[Arm64
                iloc.inst("mov", PlatformArm64::regName[Arm64_TMP_REG_NO], "#"+to_string(l));
                iloc.inst("madd", PlatformArm64::regName[Arm64_TMP_REG_NO2],
                        PlatformArm64::regName[reg2],
                        PlatformArm64::regName[Arm64_TMP_REG_NO]
                        +","+PlatformArm64::regName[Arm64_TMP_REG_NO2]);
            }
            
        }
        inst->setMemoryAddr(Arm64_TMP_REG_NO2, baseOff);
        // add t1, l0, l1, lsl 2
    }
}

/// @brief 函数调用指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_gep_formalparam(Instruction *inst) {
    Value *arg1 = inst->getOperand(0);
    Value *arg2 = inst->getOperand(1);

    int32_t baseReg = -1;
    int64_t baseOff;
    //simpleRegisterAllocator.Allocate(arg1);
    if (!arg1->getMemoryAddr(&baseReg, &baseOff)) {
        
        baseReg = arg1->getRegId();
        baseOff = 0;
    }
    //printf("baseReg: %d, baseOff: %ld\n", baseReg, baseOff);
    //printf("LoadRegId: %d\n", arg1->getLoadRegId());
    Instanceof(off, ConstInt*, arg2);
    uint32_t l = ((ArrayType*)(inst->getType()))->getElementType()->getSize();
    l = 8;//暂时默认为8
    if (off) {
        if (dynamic_cast<GlobalVariable*>(arg1)) {
            iloc.lea_var(Arm64_TMP_REG_NO, arg1);
            baseReg = Arm64_TMP_REG_NO;
        }else{
            baseReg = Arm64_TMP_REG_NO;
            if (PlatformArm64::isDisp(baseOff)){
                iloc.inst("ldr",PlatformArm64::regName[baseReg],"["+PlatformArm64::regName[Arm64_SP_REG_NO],"#"+std::to_string(baseOff)+"]");
            }else{
                iloc.load_imm(Arm64_TMP_REG_NO4, baseOff);
                iloc.inst("add",PlatformArm64::regName[Arm64_TMP_REG_NO4],PlatformArm64::regName[Arm64_TMP_REG_NO4],PlatformArm64::regName[Arm64_SP_REG_NO]);
                iloc.inst("ldr",PlatformArm64::regName[baseReg],"["+PlatformArm64::regName[Arm64_TMP_REG_NO4]+"]");
            }
            if(PlatformArm64::isDisp(off->getVal() * l)){
                //直接加偏移量
                iloc.inst("add",PlatformArm64::regName[baseReg],PlatformArm64::regName[baseReg],"#"+std::to_string(off->getVal() * l)+"");
            }else{
                //先加载偏移量到临时寄存器
                iloc.load_imm(Arm64_TMP_REG_NO4, off->getVal() * l);
                //再加偏移量
                iloc.inst("add",PlatformArm64::regName[baseReg],PlatformArm64::regName[baseReg],PlatformArm64::regName[Arm64_TMP_REG_NO4]);
            }
            //iloc.inst("ldr",PlatformArm64::regName[baseReg],"[sp,#"+std::to_string(baseOff)+"]");
            //iloc.inst("add",PlatformArm64::regName[baseReg],PlatformArm64::regName[baseReg],"#"+std::to_string(off->getVal() * l)+"");
        }
        inst->setMemoryAddr(baseReg, off->getVal() * l);//baseOff + off->getVal() * l);
        inst->setLoadRegId(baseReg);
    } else {
        // TODO
        // %t1 = getelemptr [3xi32] %l0, %l1
        // // mul l1, l1, lx
        //printf("reg1: %d\n", baseReg);
        if (baseReg == -1) {
            baseReg = Arm64_TMP_REG_NO;
            if (dynamic_cast<GlobalVariable*>(arg1)) {
                iloc.lea_var(baseReg, arg1);
            } else {
                iloc.load_var(baseReg, arg1);
            }
        }
        int32_t reg2 = arg2->getRegId();
        //printf("reg2: %d\n", reg2);
        if (reg2 == -1) {
            reg2 = Arm64_TMP_REG_NO2;
            iloc.load_var(reg2, arg2);
        }
        if (__builtin_popcount(l) == 1) {
            if(baseReg!=Arm64_SP_REG_NO){
                iloc.inst("add", PlatformArm64::regName[Arm64_TMP_REG_NO2], PlatformArm64::regName[baseReg], PlatformArm64::regName[reg2]+",lsl "+to_string(__builtin_ctz(l)));
            }else{
                iloc.inst("ldr",PlatformArm64::regName[Arm64_TMP_REG_NO],"[sp,#"+to_string(baseOff)+"]");
                iloc.inst("add", PlatformArm64::regName[Arm64_TMP_REG_NO2], PlatformArm64::regName[Arm64_TMP_REG_NO], PlatformArm64::regName[reg2]+" ,lsl "+to_string(__builtin_ctz(l)));
            }
        } else {
            if(baseReg != Arm64_SP_REG_NO) {
                iloc.inst("mov", PlatformArm64::regName[Arm64_TMP_REG_NO], "#"+to_string(l));
                iloc.inst("madd", PlatformArm64::regName[Arm64_TMP_REG_NO2],
                        PlatformArm64::regName[reg2],
                        PlatformArm64::regName[Arm64_TMP_REG_NO]
                        +","+PlatformArm64::regName[baseReg]);
            } else {
                iloc.inst("ldr",PlatformArm64::regName[Arm64_TMP_REG_NO2],"[sp,#"+to_string(baseOff)+"]");
                iloc.inst("mov", PlatformArm64::regName[Arm64_TMP_REG_NO], "#"+to_string(l));
                iloc.inst("madd", PlatformArm64::regName[Arm64_TMP_REG_NO2],
                        PlatformArm64::regName[reg2],
                        PlatformArm64::regName[Arm64_TMP_REG_NO]
                        +","+PlatformArm64::regName[Arm64_TMP_REG_NO2]);
            }
            
        }
        inst->setMemoryAddr(Arm64_TMP_REG_NO2, baseOff);
        // add t1, l0, l1, lsl 2
    }
}

/// @brief 函数调用指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_store(Instruction *inst) {
    int64_t off = 0;
    Value *ptr = inst->getOperand(0),
          *src = inst->getOperand(1);
    int32_t basereg = ptr->getRegId(),
            loadreg = src->getRegId();
    if (loadreg == -1) {
        loadreg = Arm64_TMP_REG_NO3;
        iloc.load_var(loadreg, src);
    }
    if (basereg == -1) {
        ptr->getMemoryAddr(&basereg, &off);
    }
    off = 0;//只需要存即可
    iloc.store_base(loadreg, basereg, off, Arm64_TMP_REG_NO3);
}

/// @brief 函数调用指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_load(Instruction *inst) {
    int64_t off = 0;
    Value *addr = inst->getOperand(0);
    int32_t basereg = addr->getRegId(),
            loadreg = inst->getRegId();
    if (loadreg == -1) {
        loadreg = Arm64_TMP_REG_NO;
        //iloc.load_var(loadreg, addr);
        // 栈+偏移的寻址方式
        int32_t var_baseRegId = -1;
        int64_t var_offset = -1;
        bool result = addr->getMemoryAddr(&var_baseRegId, &var_offset);

        if (!result) {
            minic_log(LOG_ERROR, "BUG");
        }
        iloc.load_base(loadreg, var_baseRegId, 0);//var_offset);
        //加载后存到内存中
        int32_t var_baseRegId2 = -1;
        int64_t var_offset2 = -1;
        bool result2 = inst->getMemoryAddr(&var_baseRegId2, &var_offset2);
        if(result2){
            iloc.store_base(loadreg,var_baseRegId2, var_offset2, Arm64_TMP_REG_NO2);
        }
        }else if (basereg == -1) {
        addr->getMemoryAddr(&basereg, &off);
        //off = 0;//只需要取即可
        //inst->setLoadRegId(Arm64_TMP_REG_NO);
        iloc.load_base(loadreg, basereg, off);
        int32_t var_baseRegId2 = -1;
        int64_t var_offset2 = -1;
        bool result2 = inst->getMemoryAddr(&var_baseRegId2, &var_offset2);
        if(result2){
            iloc.store_base(loadreg,var_baseRegId2, var_offset2, Arm64_TMP_REG_NO2);
        }
    }else{
        off = 0;//只需要取即可
        //inst->setLoadRegId(Arm64_TMP_REG_NO);
        iloc.load_base(loadreg, basereg, off);
        int32_t var_baseRegId2 = -1;
        int64_t var_offset2 = -1;
        bool result2 = inst->getMemoryAddr(&var_baseRegId2, &var_offset2);
        if(result2){
            iloc.store_base(loadreg,var_baseRegId2, var_offset2, Arm64_TMP_REG_NO2);
        }
    }

}

/// @brief 函数调用指令翻译成Arm64汇编
/// @param inst IR指令
void InstSelectorArm64::translate_call(Instruction * inst)
{
    FuncCallInstruction * callInst = dynamic_cast<FuncCallInstruction *>(inst);

    int32_t operandNum = callInst->getOperandsNum();

    if (operandNum != realArgCount) {
        if (realArgCount != 0) {
            minic_log(LOG_ERROR, "ARG指令的个数与调用函数个数不一致");
        }
    }

    // ARMv8 前8个参数使用 x0-x7 传递
    const int MAX_REG_ARGS = 8;

    if (operandNum) {
        // 保护参数传递寄存器
        for (int i = 0; i < MAX_REG_ARGS && i < operandNum; i++) {
            simpleRegisterAllocator.Allocate(i);
        }

        // 栈传递的参数（超过8个的部分）
        int stackOffset = 0;
        for (int k = MAX_REG_ARGS; k < operandNum; k++) {
            auto arg = callInst->getOperand(k);

            // 新建一个内存变量，用于栈传值到形参变量中
            MemVariable * newVal = func->newMemVariable((Type *) PointerType::get(arg->getType()));
            newVal->setMemoryAddr(Arm64_SP_REG_NO, stackOffset);
            stackOffset += 8;  // ARMv8 每个参数占8字节

            Instruction * assignInst ;
            //按照参数类型先进行翻译
            IRInstOperator op = ((Instruction*)arg)->getOp();
            //printf("Translate: Operator(%d) %s\n", (int) op, inst->getName().c_str());
            if(op == IRInstOperator::IRINST_OP_GEP || op == IRInstOperator::IRINST_OP_GEP_FormalParam){
                map<IRInstOperator, translate_handler>::const_iterator pIter;
                pIter = translator_handlers.find(op);
                if (pIter == translator_handlers.end()) {
                    // 没有找到，则说明当前不支持
                    printf("Translate: Operator(%d) not support", (int) op);
                    return;
                }

                // 开启时输出IR指令作为注释
                if (showLinearIR) {
                    outputIRInstruction((Instruction*)arg);
                }

                (this->*(pIter->second))((Instruction*)arg);
                //iloc.inst("str", "test");
                assignInst = new MoveInstruction(func, newVal, arg,3);
            //iloc.inst("str", "test1");
            }else{
                assignInst = new MoveInstruction(func, newVal, arg);
            //iloc.inst("str", "test3");
            }
            //iloc.inst("str", "test2");
            translate_assign(assignInst);
            // 释放临时寄存器
            //simpleRegisterAllocator.Allocate(arg);
            //simpleRegisterAllocator.free(arg);
            delete assignInst;
        }

        // 寄存器传递的参数（前8个）
        for (int k = 0; k < operandNum && k < MAX_REG_ARGS; k++) {
            auto arg = callInst->getOperand(k);
            Instruction * assignInst ;
            if(!arg->getType()->isArrayType()){
                assignInst = new MoveInstruction(func, PlatformArm64::intRegVal[k], arg);
            }else{
                assignInst = new MoveInstruction(func, PlatformArm64::intRegVal[k], arg,3);
            }
            IRInstOperator op = ((Instruction*)arg)->getOp();
            //printf("Translate: Operator(%d) %s\n", (int) op, inst->getName().c_str());
            if(op == IRInstOperator::IRINST_OP_GEP || op == IRInstOperator::IRINST_OP_GEP_FormalParam){
                map<IRInstOperator, translate_handler>::const_iterator pIter;
                pIter = translator_handlers.find(op);
                if (pIter == translator_handlers.end()) {
                    // 没有找到，则说明当前不支持
                    printf("Translate: Operator(%d) not support", (int) op);
                    return;
                }

                // 开启时输出IR指令作为注释
                if (showLinearIR) {
                    outputIRInstruction((Instruction*)arg);
                }

                (this->*(pIter->second))((Instruction*)arg);
            }
            translate_assign(assignInst);
            delete assignInst;
        }
    }

    iloc.call_fun(callInst->getName());

    // 释放参数寄存器
    for (int i = 0; i < MAX_REG_ARGS && i < operandNum; i++) {
        simpleRegisterAllocator.free(i);
    }

    // 处理返回值
    if (callInst->hasResultValue()) {
        Instruction * assignInst = new MoveInstruction(func, callInst, PlatformArm64::intRegVal[0]);
        translate_assign(assignInst);
        delete assignInst;
    }

    // 函数调用后清零，使得下次可正常统计
    realArgCount = 0;
}

///
/// @brief 实参指令翻译成Arm64汇编
/// @param inst
///
void InstSelectorArm64::translate_arg(Instruction * inst)
{
    Value * src = inst->getOperand(0);
    int32_t regId = src->getRegId();

    if (realArgCount < 8) {  // ARMv8 使用8个寄存器传递参数
        if (regId != -1) {
            if (regId != realArgCount) {
                minic_log(LOG_ERROR, "第%d个ARG指令对象寄存器分配有误: %d", argCount + 1, regId);
            }
        } else {
            minic_log(LOG_ERROR, "第%d个ARG指令对象不是寄存器", argCount + 1);
        }
    } else {
        int32_t baseRegId;
        bool result = src->getMemoryAddr(&baseRegId);
        if ((!result) || (baseRegId != Arm64_SP_REG_NO)) {
            minic_log(LOG_ERROR, "第%d个ARG指令对象不是SP寄存器寻址", argCount + 1);
        }
    }

    realArgCount++;
}

/// @brief 整数到浮点数转换指令翻译成ARM32汇编
/// @param inst IR指令
void InstSelectorArm64::translate_itof(Instruction * inst)
{
    Value * rs = inst;
    Value * arg = inst->getOperand(0);
    
    int rs_reg_no = REG_ALLOC_SIMPLE_DST_REG_NO;
    int op_reg_no = REG_ALLOC_SIMPLE_SRC1_REG_NO;
    
    std::string arg_reg_name;
    int arg_reg_no = arg->getRegId();

    // 看arg是否是寄存器，若是则寄存器寻址，否则要load变量到寄存器中
    if (arg_reg_no == -1) {
        iloc.load_var(op_reg_no, arg);
    } else {
        op_reg_no = arg_reg_no;
    }

    arg_reg_name = PlatformArm64::regName[op_reg_no];

    // 看结果变量是否是寄存器，若不是则采用参数指定的寄存器rs_reg_name
    if (rs->getRegId() != -1) {
        rs_reg_no = rs->getRegId();
    } else {
        // 临时变量
        rs->setLoadRegId(rs_reg_no);
    }

    std::string rs_reg_name = PlatformArm64::regName[rs_reg_no];

    // 使用scvtf指令进行整数到浮点数的转换
    iloc.inst("scvtf", rs_reg_name, arg_reg_name);

    // 结果不是寄存器，则需要把rs_reg_name保存到结果变量中
    if (rs->getRegId() == -1) {
        iloc.store_var(rs_reg_no, rs, op_reg_no);
    }
}