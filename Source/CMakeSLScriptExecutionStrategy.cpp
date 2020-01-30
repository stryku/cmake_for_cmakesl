#include "CMakeSLScriptExecutionStrategy.hpp"

#include "CMakeFacade.hpp"
#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmState.h"

#include "cmake_facade.hpp"

#include "cmakesl/source/common/string.hpp"
#include "cmakesl/source/exec/global_executor.hpp"

#include <fstream>

int CMakeSLScriptExecutionStrategy::execute(cmGlobalGenerator& globalGenerator,
                                            cmStateSnapshot& snapshot)
{
  cmMakefile* dirMf = new cmMakefile(&globalGenerator, snapshot);
  globalGenerator.AddMakefile(dirMf);
  globalGenerator.IndexMakefile(dirMf);

  CMakeFacade facade{ *dirMf };

  cmsl::errors::errors_observer errs{ &facade };

  cmsl::exec::global_executor executor{
    snapshot.GetState()->GetSourceDirectory(), facade, errs
  };

  const auto result = executor.execute_based_on_root_path();
  if (facade.did_fatal_error_occure()) {
    return 1;
  }

  return result;
}
