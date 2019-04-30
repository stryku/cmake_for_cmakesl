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
  const auto rootSourceFile =
    snapshot.GetState()->GetSourceDirectory() + "/CMakeLists.cmsl";

  std::ifstream t(rootSourceFile);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());

  t.close();

  cmMakefile* dirMf = new cmMakefile(&globalGenerator, snapshot);
  globalGenerator.AddMakefile(dirMf);
  globalGenerator.IndexMakefile(dirMf);

  CMakeFacade facade{ *dirMf };

  cmsl::exec::global_executor executor{
    snapshot.GetState()->GetSourceDirectory(), facade
  };
  executor.execute(str);

  return 0;
}
