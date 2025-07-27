#pragma once

#include <string>
#include <stack>
#include <vector>

///
/// @brief 标签管理器类，用于生成和管理标签
///
class LabelManager {
public:
    /// @brief 构造函数
    LabelManager();

    /// @brief 析构函数
    ~LabelManager();

    /// @brief 生成一个新的标签
    /// @param prefix 标签前缀
    /// @return 生成的标签名
    std::string newLabel(const std::string & prefix = "");

    /// @brief 为if语句生成标签
    /// @param trueLabel 真分支标签
    /// @param falseLabel 假分支标签
    /// @param exitLabel 出口标签
    void genIfLabels(std::string & trueLabel, std::string & falseLabel, std::string & exitLabel);

    /// @brief 为while循环生成标签
    /// @param entryLabel 循环入口标签
    /// @param bodyLabel 循环体标签
    /// @param exitLabel 循环出口标签
    void genWhileLabels(std::string & entryLabel, std::string & bodyLabel, std::string & exitLabel);

    /// @brief 获取当前循环的break标签
    /// @return break标签
    std::string getBreakLabel();

    /// @brief 获取当前循环的continue标签
    /// @return continue标签
    std::string getContinueLabel();

    /// @brief 进入新的循环作用域
    void enterLoop();

    /// @brief 退出循环作用域
    void exitLoop();

private:
    /// @brief 标签计数器
    int labelCounter;

    /// @brief 循环作用域栈，每个作用域包含break和continue标签
    struct LoopScope {
        std::string breakLabel;
        std::string continueLabel;
    };
    std::stack<LoopScope> loopScopes;
};