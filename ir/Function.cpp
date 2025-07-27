///
/// @file Function.cpp
/// @brief 函数实现
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
/// </table>
///

#include <cstdlib>
#include <string>

#include "IRConstant.h"
#include "Function.h"
#include "IRCode.h"
#include "ArrayType.h"
#include "MoveInstruction.h"

/// @brief 指定函数名字、函数类型的构造函数
/// @param _name 函数名称
/// @param _type 函数类型
/// @param _builtin 是否是内置函数
Function::Function(std::string _name, FunctionType * _type, bool _builtin)
    : GlobalValue(_type, _name), builtIn(_builtin)
{
    returnType = _type->getReturnType();

    // 设置对齐大小
    setAlignment(1);
}

///
/// @brief 析构函数
/// @brief 释放函数占用的内存和IR指令代码
/// @brief 注意：IR指令代码并未释放，需要手动释放
Function::~Function()
{
    Delete();
}

/// @brief 获取函数返回类型
/// @return 返回类型
Type * Function::getReturnType()
{
    return returnType;
}

/// @brief 获取函数的形参列表
/// @return 形参列表
std::vector<FormalParam *> & Function::getParams()
{
    return params;
}

/// @brief 获取函数内的IR指令代码
/// @return IR指令代码
InterCode & Function::getInterCode()
{
    return code;
}

/// @brief 判断该函数是否是内置函数
/// @return true: 内置函数，false：用户自定义
bool Function::isBuiltin()
{
    return builtIn;
}

/// @brief 函数指令信息输出
/// @param str 函数指令
void Function::toString(std::string & str)
{
    if (builtIn) {
        // 内置函数则什么都不输出
        return;
    }

    // 输出函数头
    str = "define " + getReturnType()->toString() + " " + getIRName() + "(";

    bool firstParam = false;
    for (auto & param: params) {

        if (!firstParam) {
            firstParam = true;
        } else {
            str += ", ";
        }

        // 检查参数是否是数组类型
        if (param->getType()->isArrayType()) {
            // 如果是数组类型，需要特殊处理
            ArrayType * arrayType = static_cast<ArrayType *>(param->getType());
            // 获取数组元素类型
            std::string elemTypeStr = arrayType->getElementType()->toString();

            // 获取所有维度并显示
            const std::vector<uint32_t> & dimensions = arrayType->getDimensions();
            std::string dimensions_str = "";
            for (auto dim: dimensions) {
                dimensions_str += "[" + std::to_string(dim) + "]";
            }

            // 构造正确的数组参数格式：i32 %t0[0][3]
            str += elemTypeStr + " " + param->getIRName() + dimensions_str;
        } else {
            // 普通参数
            std::string param_str = param->getType()->toString() + " " + param->getIRName();
            str += param_str;
        }
    }

    str += ")\n";

    str += "{\n";

    // 输出局部变量的名字与IR名字
    for (auto & var: this->varsVector) {

        // 局部变量和临时变量需要输出declare语句
        std::string typeStr = var->getType()->toString();
        std::string varName = var->getIRName();

        // 检查是否是数组类型
        if (var->getType()->isArrayType()) {
            // 如果是数组类型，需要特殊处理
            ArrayType * arrayType = static_cast<ArrayType *>(var->getType());
            // 获取数组元素类型
            std::string elemTypeStr = typeStr;

            // 获取所有维度并显示
            const std::vector<uint32_t> & dimensions = arrayType->getDimensions();
            std::string dimensions_str = "";
            for (auto dim: dimensions) {
                dimensions_str += "[" + std::to_string(dim) + "]";
            }

            // 构造正确的数组声明格式：declare i32 %l1[2][2]
            str += "\tdeclare " + elemTypeStr + " " + varName + dimensions_str;
        } else {
            // 普通变量声明
            str += "\tdeclare " + typeStr + " " + varName;
        }

        std::string extraStr;
        std::string realName = var->getName();
        if (!realName.empty()) {
            str += " ; " + std::to_string(var->getScopeLevel()) + ":" + realName;
        }

        str += "\n";
    }

    // 输出临时变量的declare形式
    // 遍历所有的线性IR指令，文本输出
    for (auto & inst: code.getInsts()) {
        if (inst->hasResultValue()) {
            // 构建声明字符串
            std::string declareStr = "\tdeclare ";

            // 检查这个指令的结果是否被用作指针
            bool isPointer = false;

            // 遍历所有IR指令，查找是否有使用 *inst 的情况
            for (auto & checkInst: code.getInsts()) {
                // 检查是否有 *inst 形式的指令
                std::string instStr;
                checkInst->toString(instStr);

                // 只有当指令是MoveInstruction并且是存储或加载操作时，才可能是指针
                if (checkInst->getOp() == IRInstOperator::IRINST_OP_ASSIGN &&
                    instStr.find("*" + inst->getIRName()) != std::string::npos) {
                    // 尝试将指令转换为MoveInstruction
                    MoveInstruction * moveInst = dynamic_cast<MoveInstruction *>(checkInst);
                    if (moveInst) {
                        int opType = moveInst->getOpType();

                        // opType为1表示存储操作(*ptr = val)，此时ptr是指针
                        // opType为2表示加载操作(val = *ptr)，此时ptr是指针
                        if ((opType == 1 && moveInst->getDst() == inst) ||
                            (opType == 2 && moveInst->getSrc() == inst)) {
                            isPointer = true;
                            break;
                        }
                    }
                }
            }

            // 处理数组类型
            if (inst->getType()->isArrayType()) {
                ArrayType * arrayType = static_cast<ArrayType *>(inst->getType());
                const std::vector<uint32_t> & dims = arrayType->getDimensions();

                // 添加基本类型
                declareStr += arrayType->getElementType()->toString() + " " + inst->getIRName();

                // 添加维度信息
                for (size_t i = 0; i < dims.size(); ++i) {
                    declareStr += "[" + std::to_string(dims[i]) + "]";
                }
            } else if (isPointer) {
                // 处理指针类型
                declareStr += inst->getType()->toString() + "* " + inst->getIRName();
            } else {
                // 处理普通类型
                declareStr += inst->getType()->toString() + " " + inst->getIRName();
            }

            // 检查是否有注释（通常是变量名）
            std::string comment = inst->getIRName();
            size_t pos = comment.find(" ; ");
            if (pos != std::string::npos) {
                declareStr += comment.substr(pos);
            }

            // 添加到输出
            str += declareStr + "\n";
        }
    }
    
    // 遍历所有的线性IR指令，文本输出
    for (auto & inst: code.getInsts()) {

        std::string instStr;
        inst->toString(instStr);

        if (!instStr.empty()) {

            // Label指令不加Tab键
            if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
                str += instStr + "\n";
            } else {
                str += "\t" + instStr + "\n";
            }
        }
    }

    // 输出函数尾部
    str += "}\n";
}

/// @brief 设置函数出口指令
/// @param inst 出口Label指令
void Function::setExitLabel(Instruction * inst)
{
    exitLabel = inst;
}

/// @brief 获取函数出口指令
/// @return 出口Label指令
Instruction * Function::getExitLabel()
{
    return exitLabel;
}

/// @brief 设置函数返回值变量
/// @param val 返回值变量，要求必须是局部变量，不能是临时变量
void Function::setReturnValue(LocalVariable * val)
{
    returnValue = val;
}

/// @brief 获取函数返回值变量
/// @return 返回值变量
LocalVariable * Function::getReturnValue()
{
    return returnValue;
}

/// @brief 获取最大栈帧深度
/// @return 栈帧深度
int Function::getMaxDep()
{
    return maxDepth;
}

/// @brief 设置最大栈帧深度
/// @param dep 栈帧深度
void Function::setMaxDep(int dep)
{
    maxDepth = dep;

    // 设置函数栈帧被重定位标记，用于生成不同的栈帧保护代码
    relocated = true;
}

/// @brief 获取本函数需要保护的寄存器
/// @return 要保护的寄存器
std::vector<int32_t> & Function::getProtectedReg()
{
    return protectedRegs;
}

/// @brief 获取本函数需要保护的寄存器字符串
/// @return 要保护的寄存器
std::string & Function::getProtectedRegStr()
{
    return protectedRegStr;
}

/// @brief 获取函数调用参数个数的最大值
/// @return 函数调用参数个数的最大值
int Function::getMaxFuncCallArgCnt()
{
    return maxFuncCallArgCnt;
}

/// @brief 设置函数调用参数个数的最大值
/// @param count 函数调用参数个数的最大值
void Function::setMaxFuncCallArgCnt(int count)
{
    maxFuncCallArgCnt = count;
}

/// @brief 函数内是否存在函数调用
/// @return 是否存在函调用
bool Function::getExistFuncCall()
{
    return funcCallExist;
}

/// @brief 设置函数是否存在函数调用
/// @param exist true: 存在 false: 不存在
void Function::setExistFuncCall(bool exist)
{
    funcCallExist = exist;
}

/// @brief 新建变量型Value。先检查是否存在，不存在则创建，否则失败
/// @param name 变量ID
/// @param type 变量类型
/// @param scope_level 局部变量的作用域层级
/// @param is_const 是否为常量（const）
LocalVariable * Function::newLocalVarValue(Type * type, std::string name, int32_t scope_level, bool is_const)
{
    // 创建变量并加入符号表
    LocalVariable * varValue = new LocalVariable(type, name, scope_level);
    
    // 设置变量是否为常量
    varValue->setConst(is_const);

    // varsVector表中可能存在变量重名的信息
    varsVector.push_back(varValue);

    return varValue;
}

/// @brief 新建一个内存型的Value，并加入到符号表，用于后续释放空间
/// \param type 变量类型
/// \return 临时变量Value
MemVariable * Function::newMemVariable(Type * type)
{
    // 肯定唯一存在，直接插入即可
    MemVariable * memValue = new MemVariable(type);

    memVector.push_back(memValue);

    return memValue;
}

/// @brief 清理函数内申请的资源
void Function::Delete()
{
    // 清理IR指令
    code.Delete();

    // 清理Value
    for (auto & var: varsVector) {
        delete var;
    }

    varsVector.clear();
}

///
/// @brief 函数内的Value重命名
///
void Function::renameIR()
{
    // 内置函数忽略
    if (isBuiltin()) {
        return;
    }

    int32_t nameIndex = 0;

    // 形式参数重命名
    for (auto & param: this->params) {
        param->setIRName(IR_TEMP_VARNAME_PREFIX + std::to_string(nameIndex++));
    }

    // 局部变量重命名
    for (auto & var: this->varsVector) {

        var->setIRName(IR_LOCAL_VARNAME_PREFIX + std::to_string(nameIndex++));
    }

    // 遍历所有的指令进行命名
    for (auto inst: this->getInterCode().getInsts()) {
        if (inst->getOp() == IRInstOperator::IRINST_OP_LABEL) {
            inst->setIRName(IR_LABEL_PREFIX + std::to_string(nameIndex++));
        } else if (inst->hasResultValue()) {
            inst->setIRName(IR_TEMP_VARNAME_PREFIX + std::to_string(nameIndex++));
        }
    }
}

///
/// @brief 获取统计的ARG指令的个数
/// @return int32_t 个数
///
int32_t Function::getRealArgcount()
{
    return this->realArgCount;
}

///
/// @brief 用于统计ARG指令个数的自增函数，个数加1
///
void Function::realArgCountInc()
{
    this->realArgCount++;
}

///
/// @brief 用于统计ARG指令个数的清零
///
void Function::realArgCountReset()
{
    this->realArgCount = 0;
}

///
/// @brief 将当前循环的标签压入栈中
/// @param startLabel 循环开始标签
/// @param endLabel 循环结束标签
///
void Function::pushLoopLabels(LabelInstruction * startLabel, LabelInstruction * endLabel)
{
    LoopLabels labels{startLabel, endLabel};
    loopLabels.push_back(labels);
}

/// @brief 弹出当前循环的标签
///
void Function::popLoopLabels()
{
    if (!loopLabels.empty()) {
        loopLabels.pop_back();
    }
}

///
/// @brief 获取当前循环的开始标签
/// @return 循环开始标签
///
LabelInstruction * Function::getCurrentLoopStartLabel()
{
    if (loopLabels.empty()) {
        return nullptr;
    }
    return loopLabels.back().startLabel;
}

///
/// @brief 获取当前循环的结束标签
/// @return 循环结束标签
///
LabelInstruction * Function::getCurrentLoopEndLabel()
{
    if (loopLabels.empty()) {
        return nullptr;
    }
    return loopLabels.back().endLabel;
}

/// @brief 新建局部数组变量
/// @param name 数组名
/// @param type 数组类型
/// @return 局部数组变量
LocalVariable * Function::createLocalArray(std::string name, Type * type)
{
    // 创建局部数组变量
    LocalVariable * array_var = newLocalVarValue(type, name);

    return array_var;
}
