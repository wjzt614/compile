#include "LabelManager.h"

LabelManager::LabelManager() : labelCounter(0)
{}

LabelManager::~LabelManager()
{}

std::string LabelManager::newLabel(const std::string & prefix)
{
    return ".L" + prefix + std::to_string(labelCounter++);
}

void LabelManager::genIfLabels(std::string & trueLabel, std::string & falseLabel, std::string & exitLabel)
{
    trueLabel = newLabel("if_true_");
    falseLabel = newLabel("if_false_");
    exitLabel = newLabel("if_exit_");
}

void LabelManager::genWhileLabels(std::string & entryLabel, std::string & bodyLabel, std::string & exitLabel)
{
    entryLabel = newLabel("while_entry_");
    bodyLabel = newLabel("while_body_");
    exitLabel = newLabel("while_exit_");
}

void LabelManager::enterLoop()
{
    LoopScope scope;
    scope.breakLabel = newLabel("break_");
    scope.continueLabel = newLabel("continue_");
    loopScopes.push(scope);
}

void LabelManager::exitLoop()
{
    if (!loopScopes.empty()) {
        loopScopes.pop();
    }
}

std::string LabelManager::getBreakLabel()
{
    if (loopScopes.empty()) {
        return "";
    }
    return loopScopes.top().breakLabel;
}

std::string LabelManager::getContinueLabel()
{
    if (loopScopes.empty()) {
        return "";
    }
    return loopScopes.top().continueLabel;
}