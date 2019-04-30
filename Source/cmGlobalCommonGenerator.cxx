/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGlobalCommonGenerator.h"
#include "ScriptExecutionStrategy.hpp"

class cmake;

cmGlobalCommonGenerator::cmGlobalCommonGenerator(cmake* cm, ScriptExecutionStrategy* scriptExecution)
  : cmGlobalGenerator(cm, scriptExecution)
{
}

cmGlobalCommonGenerator::~cmGlobalCommonGenerator() = default;
