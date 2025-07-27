///
/// @file IRGenerator.h
/// @brief AST遍历产生线性IR的头文件
/// @author zenglj (zenglj@live.com)
/// @version 1.1
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-09-29 <td>1.0     <td>zenglj  <td>新建
/// <tr><td>2024-11-23 <td>1.1     <td>zenglj  <td>表达式版增强
/// </table>
///
#pragma once

#include <unordered_map>
#include "LabelManager.h"
#include "AST.h"
#include "Module.h"
#include "LabelInstruction.h"
#include <set>
#include <map>
/// @brief AST遍历产生线性IR类
class IRGenerator {

public:
    /// @brief 构造函数
    /// @param _root AST根节点
    /// @param _module 要填充的模块
    IRGenerator(ast_node * _root, Module * _module);

    /// @brief 析构函数
    ~IRGenerator() = default;

    /// @brief 运行产生IR
    bool run();

protected:
    /// @brief 编译单元AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_compile_unit(ast_node * node);

    /// @brief 函数定义AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_define(ast_node * node);

    /// @brief 形式参数AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_formal_params(ast_node * node);

    /// @brief 函数调用AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_function_call(ast_node * node);

    /// @brief 计算常量表达式（只包含常量符号、字面量）
    bool calcConstExpr(ast_node * node, void * ret);

    /// @brief 语句块（含函数体）AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_block(ast_node * node);

    /// @brief 整数加法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_add(ast_node * node);

    /// @brief 整数减法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_sub(ast_node * node);

    /// @brief 赋值AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_assign(ast_node * node);

    /// @brief 单目运算符 - 的翻译
    /// @param node AST节点
    bool ir_neg(ast_node * node);

    /// @brief 整数乘法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_mul(ast_node * node);

    /// @brief 整数除法AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_div(ast_node * node);

    /// @brief 整数求余AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_mod(ast_node * node);

    bool ir_logical_and(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);
    bool ir_logical_or(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);
    bool ir_logical_not(ast_node * node, LabelInstruction * trueLabel, LabelInstruction * falseLabel);
    bool ir_relational_op(ast_node * node); // 处理所有关系运算符（<, <=, >, >=, ==, !=）
    bool ir_if(ast_node * node);
    bool ir_if_else(ast_node * node);
    bool ir_while(ast_node * node);
    bool ir_break(ast_node * node);
    bool ir_continue(ast_node * node);

    // 添加标签管理器
    LabelManager labelManager;

    /// @brief return节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_return(ast_node * node);

    /// @brief 类型叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_type(ast_node * node);

    /// @brief 标识符叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_var_id(ast_node * node);

    /// @brief 无符号整数字面量叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_uint(ast_node * node);

    /// @brief float数字面量叶子节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_leaf_node_float(ast_node * node);

    /// @brief 变量声明语句节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_declare_statment(ast_node * node);

    /// @brief 变量定声明节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_variable_declare(ast_node * node);

    /// @brief 变量定义并初始化节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_variable_declare_init(ast_node * node);

    /// @brief 数组定义AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_array_def(ast_node * node);

    /// @brief 数组访问AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_array_access(ast_node * node);

    /// @brief 数组初始化列表AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_array_init_list(ast_node * node);

    /// @brief 数组初始化元素AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_array_init_elem(ast_node * node);

    /// @brief 空数组初始化AST节点翻译成线性中间IR
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_array_init_empty(ast_node * node);

    /// @brief 收集数组初始化元素
    /// @param init_node 初始化节点
    /// @param elements 存储收集到的元素的数组
    /// @param dims 数组的维度信息
    /// @param current_dim 当前处理的维度（默认为0，即最外层维度）
    void collectInitElements(ast_node* init_node, std::vector<ast_node*>& elements, const std::vector<uint32_t>& dims, size_t current_dim = 0);

    /// @brief 未知节点类型的节点处理
    /// @param node AST节点
    /// @return 翻译是否成功，true：成功，false：失败
    bool ir_default(ast_node * node);

    /// @brief 根据AST的节点运算符查找对应的翻译函数并执行翻译动作
    /// @param node AST节点
    /// @param trueLabel 真出口标签
    /// @param falseLabel 假出口标签
    /// @return 成功返回node节点，否则返回nullptr
    ast_node *
    ir_visit_ast_node(ast_node * node, LabelInstruction * trueLabel = nullptr, LabelInstruction * falseLabel = nullptr);

    /// @brief AST的节点操作函数（不带标签）
    typedef bool (IRGenerator::*ast2ir_handler_t)(ast_node *);

    /// @brief AST的节点操作函数（带标签）
    typedef bool (IRGenerator::*ast2ir_handler_with_labels_t)(ast_node *, LabelInstruction *, LabelInstruction *);

    /// @brief AST节点运算符与动作函数关联的映射表
    std::unordered_map<ast_operator_type, ast2ir_handler_t> ast2ir_handlers;

    /// @brief AST节点运算符与带标签动作函数关联的映射表
    std::unordered_map<ast_operator_type, ast2ir_handler_with_labels_t> ast2ir_handlers_with_labels;

    /// @brief 确保标签非空
    LabelInstruction * ensureLabel(LabelInstruction * label);

    /// @brief 处理类型转换，如果需要转换则创建转换指令并返回转换后的值，否则返回原值
    /// @param node 当前AST节点，用于添加指令
    /// @param value 需要转换的值
    /// @param targetType 目标类型
    /// @return 转换后的值
    Value * handleTypeConversion(ast_node * node, Value * value, Type * targetType);


private:
    /// @brief 抽象语法树的根
    ast_node * root;

    /// @brief 符号表:模块
    Module * module;
    Value * ensureConditionIsRegister(ast_node * instructionNode, Value * condValue);
    void addConditionalGoto(ast_node * instructionNode,
                            Value * condValue,
                            LabelInstruction * trueLabel,
                            LabelInstruction * falseLabel);
    /// @brief 已经处理过的全局变量名，用于防止重复声明
    std::set<std::string> processedGlobalVars;

    /// @brief 存储全局常量变量的名称和值
    std::map<std::string, int32_t> globalConstVars;
};
