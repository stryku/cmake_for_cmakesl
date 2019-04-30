#pragma once

#include "ScriptExecutionStrategy.hpp"

class cmGlobalGenerator;

class OldScriptExecutionStrategy : public ScriptExecutionStrategy
{
public:
  int execute(cmGlobalGenerator& globalGenerator,
              cmStateSnapshot& snapshot) override;
};
