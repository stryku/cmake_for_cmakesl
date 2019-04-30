#pragma once

#include "ScriptExecutionStrategy.hpp"

class CMakeSLScriptExecutionStrategy : public ScriptExecutionStrategy
{
public:
  int execute(cmGlobalGenerator& globalGenerator,
              cmStateSnapshot& snapshot) override;
};
