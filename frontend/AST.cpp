///
/// @file AST.cpp
/// @brief 抽象语法树AST管理的实现
/// @author zenglj (zenglj@live.com)
/// @version 1.1
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///
/// @par 修改日志:
/// <table>
/// <tr><th>Date       <th>Version <th>Author  <th>Description
/// <tr><td>2024-11-21 <td>1.0     <td>zenglj  <td>新做
/// <tr><td>2024-11-23 <td>1.1     <td>zenglj  <td>表达式版增强
/// </table>
///
#include <cstdarg>
#include <cstdint>
#include <string>
#include <vector>

#include "AST.h"
#include "AttrType.h"
#include "Types/IntegerType.h"
#include "Types/VoidType.h"
#include "Types/ArrayType.h"
#include "Types/FloatType.h"

/* 整个AST的根节点 */
ast_node * ast_root = nullptr;

/// @brief 创建指定节点类型的节点
/// @param _node_type 节点类型
/// @param _line_no 行号
ast_node::ast_node(ast_operator_type _node_type, Type * _type, int64_t _line_no)
    : node_type(_node_type), line_no(-1), type(_type)
{}

/// @brief 构造函数
/// @param _type 节点值的类型
/// @param line_no 行号
ast_node::ast_node(Type * _type) : ast_node(ast_operator_type::AST_OP_LEAF_TYPE, _type, -1)
{}

/// @brief 针对无符号整数字面量的构造函数
/// @param attr 无符号整数字面量
ast_node::ast_node(digit_int_attr attr)
    : ast_node(ast_operator_type::AST_OP_LEAF_LITERAL_UINT, IntegerType::getTypeInt(), attr.lineno)
{
    integer_val = attr.val;
}

/// @brief 针对浮点数字面量的构造函数
/// @param attr 浮点数字面量
ast_node::ast_node(digit_real_attr attr) : ast_node(ast_operator_type::AST_OP_LEAF_LITERAL_FLOAT, FloatType::getType(), attr.lineno)
{
    this->float_val = (float)attr.val;
}

/// @brief 针对标识符ID的叶子构造函数
/// @param attr 字符型字面量
ast_node::ast_node(var_id_attr attr) : ast_node(ast_operator_type::AST_OP_LEAF_VAR_ID, VoidType::getType(), attr.lineno)
{
    name = attr.id;
}

/// @brief 针对标识符ID的叶子构造函数
/// @param _id 标识符ID
/// @param _line_no 行号
ast_node::ast_node(std::string _id, int64_t _line_no)
    : ast_node(ast_operator_type::AST_OP_LEAF_VAR_ID, VoidType::getType(), _line_no)
{
    name = _id;
}

/// @brief 判断是否是叶子节点
/// @return true：是叶子节点 false：内部节点
bool ast_node::isLeafNode()
{
    bool is_leaf;

    switch (this->node_type) {
        case ast_operator_type::AST_OP_LEAF_LITERAL_UINT:
        case ast_operator_type::AST_OP_LEAF_LITERAL_FLOAT:
        case ast_operator_type::AST_OP_LEAF_VAR_ID:
        case ast_operator_type::AST_OP_LEAF_TYPE:
            is_leaf = true;
            break;
        default:
            is_leaf = false;
            break;
    }

    return is_leaf;
}

/// @brief 创建指定节点类型的节点，请注意在指定有效的孩子后必须追加一个空指针nullptr，表明可变参数结束
/// @param type 节点类型
/// @param son_num 孩子节点的个数
/// @param ...
/// 可变参数，可支持插入若干个孩子节点，自左往右的次序，最后一个孩子节点必须指定为nullptr。如果没有孩子，则指定为nullptr
/// @return 创建的节点
ast_node * ast_node::New(ast_operator_type type, ...)
{
    ast_node * parent_node = new ast_node(type);

    va_list valist;

    /* valist指向传入的第一个可选参数 */
    va_start(valist, type);

    for (;;) {

        // 获取节点对象。如果最后一个对象为空指针，则说明结束
        ast_node * node = va_arg(valist, ast_node *);
        if (nullptr == node) {
            break;
        }

        // 插入到父节点中
        parent_node->insert_son_node(node);
    }

    /* 清理为 valist 保留的内存 */
    va_end(valist);

    return parent_node;
}

/// @brief 向父节点插入一个节点
/// @param parent 父节点
/// @param node 节点
ast_node * ast_node::insert_son_node(ast_node * node)
{
    if (node) {

        // 孩子节点有效时加入，主要为了避免空语句等时会返回空指针
        node->parent = this;
        this->sons.push_back(node);
    }

    return this;
}

/// @brief 创建无符号整数的叶子节点
/// @param attr 无符号整数字面量
ast_node * ast_node::New(digit_int_attr attr)
{
    ast_node * node = new ast_node(attr);

    return node;
}

/// @brief 创建浮点数的叶子节点
/// @param attr 浮点数字面量
ast_node * ast_node::New(digit_real_attr attr)
{
    return new ast_node(attr);
}

/// @brief 创建标识符的叶子节点
/// @param attr 字符型字面量
ast_node * ast_node::New(var_id_attr attr)
{
    ast_node * node = new ast_node(attr);

    return node;
}

/// @brief 创建标识符的叶子节点
/// @param id 词法值
/// @param line_no 行号
ast_node * ast_node::New(std::string id, int64_t lineno)
{
    ast_node * node = new ast_node(id, lineno);

    return node;
}

/// @brief 创建具备指定类型的节点
/// @param type 节点值类型
/// @param line_no 行号
/// @return 创建的节点
ast_node * ast_node::New(Type * type)
{
    ast_node * node = new ast_node(type);

    return node;
}

/// @brief 递归清理抽象语法树
/// @param node AST的节点
void ast_node::Delete(ast_node * node)
{
    if (node) {

        for (auto child: node->sons) {
            ast_node::Delete(child);
        }

        // 这里没有必要清理孩子，由于下面就要删除该节点
        // node->sons.clear();
    }

    // 清理node资源
    delete node;
}

///
/// @brief AST资源清理
///
void free_ast(ast_node * root)
{
    ast_node::Delete(root);
}

/// @brief 创建函数定义类型的内部AST节点
/// @param type_node 类型节点
/// @param name_node 函数名字节点
/// @param block_node 函数体语句块节点
/// @param params_node 函数形参，可以没有参数
/// @return 创建的节点
ast_node * create_func_def(ast_node * type_node, ast_node * name_node, ast_node * block_node, ast_node * params_node)
{
    ast_node * node = new ast_node(ast_operator_type::AST_OP_FUNC_DEF, type_node->type, name_node->line_no);

    // 设置函数名
    node->name = name_node->name;

    // 如果没有参数，则创建参数节点
    if (!params_node) {
        params_node = new ast_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAMS);
    }

    // 如果没有函数体，则创建函数体，也就是语句块
    if (!block_node) {
        block_node = new ast_node(ast_operator_type::AST_OP_BLOCK);
    }

    (void) node->insert_son_node(type_node);
    (void) node->insert_son_node(name_node);
    (void) node->insert_son_node(params_node);
    (void) node->insert_son_node(block_node);

    return node;
}

/// @brief 创建函数定义类型的内部AST节点
/// @param type 返回值类型
/// @param id 函数名字
/// @param block_node 函数体语句块节点
/// @param params_node 函数形参，可以没有参数
/// @return 创建的节点
ast_node * create_func_def(type_attr & type, var_id_attr & id, ast_node * block_node, ast_node * params_node)
{
    // 创建整型类型节点的终结符节点
    ast_node * type_node = create_type_node(type);

    // 创建标识符终结符节点
    ast_node * id_node = ast_node::New(id.id, id.lineno);

    // 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
    free(id.id);
    id.id = nullptr;

    return create_func_def(type_node, id_node, block_node, params_node);
}

/// @brief 创建AST的内部节点
/// @param node_type 节点类型
/// @param first_child 第一个孩子节点
/// @param second_child 第一个孩子节点
/// @param third_child 第一个孩子节点
/// @return 创建的节点
ast_node * create_contain_node(ast_operator_type node_type,
                               ast_node * first_child,
                               ast_node * second_child,
                               ast_node * third_child)
{
    ast_node * node = new ast_node(node_type);

    if (first_child) {
        (void) node->insert_son_node(first_child);
    }

    if (second_child) {
        (void) node->insert_son_node(second_child);
    }

    if (third_child) {
        (void) node->insert_son_node(third_child);
    }

    return node;
}

/// @brief 类型属性转换成Type
/// @param attr 词法属性
/// @return Type* 类型
///
Type * typeAttr2Type(type_attr & attr)
{
    Type * type = nullptr;

    switch (attr.type) {
        case BasicType::TYPE_INT:
            type = IntegerType::getTypeInt();
            break;
        case BasicType::TYPE_FLOAT:
            type = FloatType::getType();
            break;
        case BasicType::TYPE_VOID:
            type = VoidType::getType();
            break;
        default:
            type = VoidType::getType();
            break;
    }
    // 设置类型的常量属性
    if (attr.is_const) {
        // 对于const类型，创建新的类型实例而不是修改全局单例
        Type* constType = nullptr;
        switch (attr.type) {
            case BasicType::TYPE_INT:
                constType = new IntegerType(32);  // 创建新的int类型实例
                break;
            case BasicType::TYPE_FLOAT:
                constType = new FloatType();  // 创建新的float类型实例
                break;
            default:
                constType = type;  // 其他类型暂时沿用原实例
                break;
        }
        constType->setConst(true);
        return constType;
    }
    return type;
}

/// @brief 创建类型节点
/// @param type 类型信息
/// @return 创建的节点
ast_node * create_type_node(type_attr & attr)
{
    Type * type = typeAttr2Type(attr);

    ast_node * type_node = ast_node::New(type);

    return type_node;
}

/// @brief 创建函数调用的节点
/// @param funcname_node 函数名节点
/// @param params_node 实参节点
/// @return 创建的节点
ast_node * create_func_call(ast_node * funcname_node, ast_node * params_node)
{
    ast_node * node = new ast_node(ast_operator_type::AST_OP_FUNC_CALL);

    // 设置调用函数名
    node->name = funcname_node->name;

    // 如果没有参数，则创建参数节点
    if (!params_node) {
        params_node = new ast_node(ast_operator_type::AST_OP_FUNC_REAL_PARAMS);
    }

    (void) node->insert_son_node(funcname_node);
    (void) node->insert_son_node(params_node);

    return node;
}

///
/// @brief 根据第一个变量定义创建变量声明语句节点
/// @param first_child 第一个变量定义节点，其类型为AST_OP_VAR_DECL
/// @return ast_node* 变量声明语句节点
///
ast_node * create_var_decl_stmt_node(ast_node * first_child)
{
    // 创建变量声明语句
    ast_node * stmt_node = create_contain_node(ast_operator_type::AST_OP_DECL_STMT);

    if (first_child) {

        stmt_node->type = first_child->type;

        // 插入到变量声明语句
        (void) stmt_node->insert_son_node(first_child);
    }

    return stmt_node;
}

ast_node * createVarDeclNode(Type * type, var_id_attr & id)
{
    // 创建整型类型节点的终结符节点
    ast_node * type_node = ast_node::New(type);

    // 创建标识符终结符节点
    ast_node * id_node = ast_node::New(id.id, id.lineno);

    // 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
    free(id.id);
    id.id = nullptr;

    // 创建变量定义节点
    ast_node * decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DECL, type_node, id_node);

    // 暂存类型
    decl_node->type = type;

    return decl_node;
}

ast_node * createVarDeclNode(type_attr & type, var_id_attr & id)
{
    return createVarDeclNode(typeAttr2Type(type), id);
}

///
/// @brief 根据变量的类型和属性创建变量声明语句节点
/// @param type 变量的类型
/// @param id 变量的名字
/// @return ast_node* 变量声明语句节点
///
ast_node * create_var_decl_stmt_node(type_attr & type, var_id_attr & id)
{
    // 创建变量定义节点
    ast_node * decl_node = createVarDeclNode(type, id);

    // 创建变量声明语句
    ast_node * stmt_node = create_contain_node(ast_operator_type::AST_OP_DECL_STMT);

    stmt_node->type = decl_node->type;

    // 插入到变量声明语句
    (void) stmt_node->insert_son_node(decl_node);

    return stmt_node;
}

///
/// @brief 创建带初始化的变量定义节点
/// @param type 变量类型
/// @param id_node 变量ID节点
/// @param init_expr 初始化表达式节点
/// @return ast_node* 变量定义节点
///
ast_node * createVarDeclInitNode(Type * type, ast_node * id_node, ast_node * init_expr)
{
    // 创建变量定义节点
    ast_node * decl_node = create_contain_node(ast_operator_type::AST_OP_VAR_DEF_INIT, id_node, init_expr);

    // 设置类型
    decl_node->type = type;

    return decl_node;
}

///
/// @brief 创建函数形参节点
/// @param type 形参类型
/// @param id 形参名
/// @return ast_node* 形参节点
///
ast_node * create_func_formal_param(type_attr & type, var_id_attr & id)
{
    // 创建类型节点
    ast_node * type_node = create_type_node(type);

    // 创建标识符终结符节点
    ast_node * id_node = ast_node::New(id.id, id.lineno);

    // 对于字符型字面量的字符串空间需要释放，因词法用到了strdup进行了字符串复制
    free(id.id);
    id.id = nullptr;

    // 创建形参节点
    ast_node * param_node = create_contain_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAM, type_node, id_node);

    // 设置形参节点的类型
    param_node->type = type_node->type;

    return param_node;
}

///
/// @brief 向变量声明语句中追加变量声明
/// @param stmt_node 变量声明语句
/// @param id 变量的名字
/// @return ast_node* 变量声明语句节点
///
ast_node * add_var_decl_node(ast_node * stmt_node, var_id_attr & id)
{
    // 创建变量定义节点
    ast_node * decl_node = createVarDeclNode(stmt_node->type, id);

    // 插入到变量声明语句
    (void) stmt_node->insert_son_node(decl_node);

    return stmt_node;
}

/// @brief 创建数组类型
/// @param elemType 元素类型
/// @param dims 维度列表
/// @return 数组类型
Type * createArrayType(Type * elemType, const std::vector<uint32_t> & dims)
{
    // 如果是函数形参数组，第一维度设为0
    return ArrayType::get(elemType, dims);
}

/// @brief 从AST节点中提取数组维度
/// @param dimsNode 数组维度节点
/// @param isParam 是否是函数形参
/// @return 维度列表
std::vector<uint32_t> extractArrayDimensions(ast_node * dimsNode, bool isParam = false)
{
    std::vector<uint32_t> dimensions;
    
    // 遍历维度节点的所有子节点
    for (size_t i = 0; i < dimsNode->sons.size(); i++) {
        ast_node * dimExpr = dimsNode->sons[i];

        // 如果是常量表达式，直接获取维度值
        if (dimExpr->node_type == ast_operator_type::AST_OP_LEAF_LITERAL_UINT) {
            uint32_t dim = dimExpr->integer_val;
            dimensions.push_back(dim);
        } else {
            // 非常量表达式，暂时设为1（实际应该在语义分析阶段处理）
            dimensions.push_back(1);
        }
    }

    // 如果是函数形参，第一维度设为0
    if (isParam && !dimensions.empty()) {
        dimensions[0] = 0;
    }

    return dimensions;
}

/// @brief 创建数组变量定义节点
/// @param type 基本类型
/// @param id_node 变量ID节点
/// @param dims_node 数组维度节点
/// @return 创建的节点
ast_node * createArrayDefNode(Type * baseType, ast_node * id_node, ast_node * dims_node)
{
    // 提取数组维度
    std::vector<uint32_t> dimensions = extractArrayDimensions(dims_node, false);

    // 创建数组类型
    Type * arrayType = createArrayType(baseType, dimensions);

    // 创建数组定义节点
    ast_node * node = create_contain_node(ast_operator_type::AST_OP_ARRAY_DEF, id_node, dims_node);
    node->type = arrayType;

    return node;
}

/// @brief 创建数组形参节点
/// @param type 基本类型
/// @param id_node 变量ID节点
/// @param dims_node 数组维度节点
/// @return 创建的节点
ast_node * createArrayParamNode(Type * baseType, ast_node * id_node, ast_node * dims_node)
{
    // 提取数组维度，设置第一维为0
    std::vector<uint32_t> dimensions = extractArrayDimensions(dims_node, true);

    // 创建数组类型
    Type * arrayType = createArrayType(baseType, dimensions);

    // 创建数组形参节点
    ast_node * node = create_contain_node(ast_operator_type::AST_OP_FUNC_FORMAL_PARAM_ARRAY, id_node, dims_node);
    node->type = arrayType;
    node->name = id_node->name;

    return node;
}

/// @brief 创建数组访问节点
/// @param array_node 数组变量节点
/// @param indices_node 索引节点
/// @return 创建的节点
ast_node * createArrayAccessNode(ast_node * array_node, ast_node * indices_node)
{
    // 创建数组访问节点
    ast_node * node = create_contain_node(ast_operator_type::AST_OP_ARRAY_ACCESS, array_node, indices_node);

    // 设置类型为数组元素类型
    // 注意：这里假设在语义分析阶段会正确设置类型
    node->type = IntegerType::getTypeInt(); // 暂时默认为int类型

    return node;
}
