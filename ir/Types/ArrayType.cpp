///
/// @file ArrayType.cpp
/// @brief 数组类型实现
/// @author
/// @version 1.0
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///

#include "ArrayType.h"
#include <numeric>
#include <sstream>

ArrayType::ArrayType(Type * elemType, const std::vector<uint32_t> & dims)
    : Type(ArrayTyID), elementType(elemType), dimensions(dims)
{}

uint32_t ArrayType::getDimension(size_t dim) const
{
    if (dim < dimensions.size()) {
        return dimensions[dim];
    }
    return 0;
}

uint32_t ArrayType::getNumElements() const
{
    if (dimensions.empty()) {
        return 0;
    }

    // 使用乘法计算元素总数
    uint32_t total = 1;
    for (uint32_t dim: dimensions) {
        total *= dim;
    }
    return total;
}

ArrayType * ArrayType::get(Type * elementType, const std::vector<uint32_t> & dims)
{
    return new ArrayType(elementType, dims);
}

ArrayType * ArrayType::getAsParam(Type * elementType, const std::vector<uint32_t> & dims)
{
    if (dims.empty()) {
        return nullptr;
    }

    // 创建一个新的维度向量，保留所有原始维度
    std::vector<uint32_t> paramDims = dims;

    // 将第一维设置为0，表示这是一个数组参数
    paramDims[0] = 0;

    return new ArrayType(elementType, paramDims);
}

bool ArrayType::isArrayParam() const
{
    // 检查第一维是否为0来判断是否是数组形参
    // 修复：确保我们正确识别数组参数
    return !dimensions.empty() && dimensions[0] == 0;
}

std::string ArrayType::toString() const
{
    std::stringstream ss;
    ss << elementType->toString();

    // 修改输出格式，将维度放在变量名后面
    // 例如：i32 而不是 i32[10]
    // 实际的维度会在变量声明时添加

    return ss.str();
}