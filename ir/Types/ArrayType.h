///
/// @file ArrayType.h
/// @brief 数组类型定义
/// @author
/// @version 1.0
/// @date 2024-11-23
///
/// @copyright Copyright (c) 2024
///

#pragma once

#include <cstdint>
#include <vector>

#include "Type.h"

///
/// @brief 数组类型
///
class ArrayType : public Type {
private:
    /// @brief 元素类型
    Type * elementType;

    /// @brief 数组维度
    std::vector<uint32_t> dimensions;

    /// @brief 私有构造函数，防止外部直接创建
    ArrayType(Type * elementType, const std::vector<uint32_t> & dims);

public:
    /// @brief 获取数组元素类型
    /// @return 数组元素类型
    [[nodiscard]] Type * getElementType() const
    {
        return elementType;
    }

    /// @brief 获取数组维度
    /// @return 数组维度向量
    [[nodiscard]] const std::vector<uint32_t> & getDimensions() const
    {
        return dimensions;
    }

    /// @brief 获取指定维度的大小
    /// @param dim 维度索引
    /// @return 该维度的大小
    [[nodiscard]] uint32_t getDimension(size_t dim) const;

    /// @brief 获取数组维度数量
    /// @return 数组维度数量
    [[nodiscard]] size_t getNumDimensions() const
    {
        return dimensions.size();
    }

    /// @brief 获取数组元素总数
    /// @return 数组元素总数
    [[nodiscard]] uint32_t getNumElements() const;

    /// @brief 获取数组类型的总大小（字节数）
    /// @return 数组类型的总大小
    [[nodiscard]] int32_t getSize() const override
    {
        // 计算数组元素总数
        uint32_t numElements = getNumElements();

        // 如果是数组形参（第一维为0），则只返回指针大小（4字节）
        if (isArrayParam() && dimensions[0] == 0) {
            return 4; // ARM32平台指针大小为4字节
        }

        // 返回元素总数乘以元素类型大小
        return numElements * elementType->getSize();
    }

    /// @brief 获取数组类型
    /// @param elementType 元素类型
    /// @param dims 维度列表
    /// @return 数组类型
    static ArrayType * get(Type * elementType, const std::vector<uint32_t> & dims);

    /// @brief 获取作为函数参数的数组类型（第一维为0表示指针）
    /// @param elementType 元素类型
    /// @param dims 维度列表
    /// @return 数组参数类型
    static ArrayType * getAsParam(Type * elementType, const std::vector<uint32_t> & dims);

    /// @brief 判断是否是数组参数类型（第一维为0）
    /// @return 是否是数组参数类型
    [[nodiscard]] bool isArrayParam() const;

    /// @brief 判断是否是数组类型
    /// @return 是否是数组类型
    [[nodiscard]] bool isArrayType() const
    {
        return true;
    }

    /// @brief 获取数组类型的字符串表示
    /// @return 字符串表示
    [[nodiscard]] std::string toString() const override;
};