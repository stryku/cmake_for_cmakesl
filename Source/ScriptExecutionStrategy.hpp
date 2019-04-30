#pragma once

class cmStateSnapshot;
class cmGlobalGenerator;

class ScriptExecutionStrategy
{
public:
  virtual ~ScriptExecutionStrategy() = default;

  virtual int execute(cmGlobalGenerator& globalGenerator,
                      cmStateSnapshot& snapshot) = 0;
};
