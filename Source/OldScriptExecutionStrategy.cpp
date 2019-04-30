#include "OldScriptExecutionStrategy.hpp"

#include "cmGlobalGenerator.h"
#include "cmMakefile.h"

int OldScriptExecutionStrategy::execute(cmGlobalGenerator& globalGenerator,
                                        cmStateSnapshot& snapshot)
{
  cmMakefile* dirMf = new cmMakefile(&globalGenerator, snapshot);
  globalGenerator.AddMakefile(dirMf);
  globalGenerator.IndexMakefile(dirMf);

  // now do it
  globalGenerator.setConfigureDoneCMP0026AndCMP0024(false);
  // this->ConfigureDoneCMP0026AndCMP0024 = false;
  dirMf->Configure();
  dirMf->EnforceDirectoryLevelRules();

  globalGenerator.setConfigureDoneCMP0026AndCMP0024(true);
  //    this->ConfigureDoneCMP0026AndCMP0024 = true;
  return 0;
}
