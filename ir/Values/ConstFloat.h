#pragma once

#include "Constant.h"
#include "IRConstant.h"
#include "FloatType.h"

///
/// @brief 浮点型常量类
///
class ConstFloat : public Constant {

public:
    ///
    /// @brief 指定值的常量
    /// \param val
    explicit ConstFloat(float val) : Constant(FloatType::getType())
    {
        name = std::to_string(val);
        floatVal = val;
    }

    /// @brief 获取名字
    /// @return 变量名
    [[nodiscard]] std::string getIRName() const override
    {
        return name;
    }

    ///
    /// @brief 获取值
    /// @return float
    ///
    float getVal()
    {
        return floatVal;
    }

    ///
    /// @brief 对该Value进行Load用的寄存器编号
    /// @return int32_t 寄存器编号
    ///
    int32_t getLoadRegId() override
    {
        return this->loadRegNo;
    }

    ///
    /// @brief 对该Value进行Load用的寄存器编号
    /// @return int32_t 寄存器编号
    ///
    void setLoadRegId(int32_t regId) override
    {
        this->loadRegNo = regId;
    }

private:
    ///
    /// @brief 浮点值
    ///
    float floatVal;

    ///
    /// @brief 变量加载到寄存器中时对应的寄存器编号
    ///
    int32_t loadRegNo = -1;
}; 
