#include "CMakeFacade.hpp"

#include "cmGlobalGenerator.h"
#include "cmInstallCommandArguments.h"
#include "cmInstallTargetGenerator.h"
#include "cmMakefile.h"
#include "cmVersion.h"
#include "cmake.h"

#include <iostream>

namespace {
cmInstallTargetGenerator* CreateInstallTargetGenerator(
  cmTarget& target, const cmInstallCommandArguments& args, bool impLib,
  cmListFileBacktrace const& backtrace, const std::string& destination,
  bool forceOpt = false, bool namelink = false)
{
  cmInstallGenerator::MessageLevel message =
    cmInstallGenerator::SelectMessageLevel(target.GetMakefile());
  target.SetHaveInstallRule(true);
  const char* component = namelink ? args.GetNamelinkComponent().c_str()
                                   : args.GetComponent().c_str();
  return new cmInstallTargetGenerator(
    target.GetName(), destination.c_str(), impLib,
    args.GetPermissions().c_str(), args.GetConfigurations(), component,
    message, args.GetExcludeFromAll(), args.GetOptional() || forceOpt,
    backtrace);
}

cmInstallTargetGenerator* CreateInstallTargetGenerator(
  cmTarget& target, const cmInstallCommandArguments& args, bool impLib,
  cmListFileBacktrace const& backtrace, bool forceOpt = false,
  bool namelink = false)
{
  return CreateInstallTargetGenerator(target, args, impLib, backtrace,
                                      args.GetDestination(), forceOpt,
                                      namelink);
}
}

CMakeFacade::CMakeFacade(cmMakefile& makefile)
  : m_makefile{ &makefile }
{
}

cmsl::facade::cmake_facade::version CMakeFacade::get_cmake_version() const
{
  return { cmVersion::GetMajorVersion(), cmVersion::GetMinorVersion(),
           cmVersion::GetPatchVersion(), cmVersion::GetTweakVersion() };
}

void CMakeFacade::error(const std::string& what) const
{
  std::cerr << what << "\n";
  cmSystemTools::SetErrorOccured();
}

void CMakeFacade::warning(const std::string& what) const
{
  std::cerr << what << "\n";
}

void CMakeFacade::message(const std::string& what) const
{
  std::cout << what << "\n";
}

void CMakeFacade::fatal_error(const std::string& what)
{
  std::cerr << what << "\n";
  m_did_fatal_error_occure = true;
  cmSystemTools::SetFatalErrorOccured();
}

bool CMakeFacade::did_fatal_error_occure() const
{
  return m_did_fatal_error_occure;
}

void CMakeFacade::register_project(const std::string& name)
{
  m_makefile->SetProjectName(name);
  m_makefile->EnableLanguage(std::vector<std::string>{ "C", "CXX" }, false);
}

std::string CMakeFacade::get_current_binary_dir() const
{
  return m_makefile->GetCurrentBinaryDirectory();
}

std::string CMakeFacade::get_current_source_dir() const
{
  return m_makefile->GetCurrentSourceDirectory();
}

void CMakeFacade::add_executable(const std::string& name,
                                 const std::vector<std::string>& sources)
{
  m_makefile->AddExecutable(name, sources);
}

void CMakeFacade::add_library(const std::string& name,
                              const std::vector<std::string>& sources)
{
  m_makefile->AddLibrary(name, cmStateEnums::TargetType::STATIC_LIBRARY,
                         sources);
}

void CMakeFacade::target_link_library(const std::string& target_name,
                                      cmsl::facade::visibility v,
                                      const std::string& library_name)
{
  auto target =
    m_makefile->GetCMakeInstance()->GetGlobalGenerator()->FindTarget(
      target_name);
  if (target == nullptr) {
    return;
  }

  if (v == cmsl::facade::visibility::interface) {
    target->AppendProperty(
      "INTERFACE_LINK_LIBRARIES",
      target
        ->GetDebugGeneratorExpressions(
          library_name, cmTargetLinkLibraryType::GENERAL_LibraryType)
        .c_str());
  } else {
    target->AddLinkLibrary(*m_makefile, library_name,
                           cmTargetLinkLibraryType::GENERAL_LibraryType);
  }
}

std::string CMakeFacade::current_directory() const
{
  std::string result;
  std::string separator;
  for (const auto& dir : m_directories) {
    result += separator + dir;
    separator = '/';
  }
  return result;
}

void CMakeFacade::go_into_subdirectory(const std::string& dir)
{
  m_directories.emplace_back(dir);
}

void CMakeFacade::go_directory_up()
{
  m_directories.pop_back();
}

void CMakeFacade::install(const std::string& target_name)
{
  m_makefile->GetGlobalGenerator()->EnableInstallTarget();

  cmTarget* target = m_makefile->FindLocalNonAliasTarget(target_name);
  if (!target) {
    cmTarget* const global_target =
      m_makefile->GetGlobalGenerator()->FindTarget(target_name, true);
    if (global_target && !global_target->IsImported()) {
      target = global_target;
    }
  }

  if (target->GetType() != cmStateEnums::EXECUTABLE) {
    // Todo: handle other targets
    return;
  }

  const auto message =
    cmInstallGenerator::SelectMessageLevel(target->GetMakefile());
  auto generator = new cmInstallTargetGenerator(
    target->GetName(), "bin", false, "", {}, "Unspecified", message, false,
    false, m_makefile->GetBacktrace());

  m_makefile->AddInstallGenerator(generator);
  m_makefile->GetGlobalGenerator()->AddInstallComponent("Unspecified");
}

void CMakeFacade::target_include_directories(
  const std::string& target_name, cmsl::facade::visibility v,
  const std::vector<std::string>& dirs)
{
  auto target =
    m_makefile->GetCMakeInstance()->GetGlobalGenerator()->FindTarget(
      target_name);
  const auto joined = join_paths(dirs);

  if (v == cmsl::facade::visibility::interface) {
    target->AppendProperty("INTERFACE_INCLUDE_DIRECTORIES", joined.c_str());
  } else {
    cmListFileBacktrace lfbt = m_makefile->GetBacktrace();
    target->InsertInclude(joined, lfbt, /*before=*/false);
  }
}

void CMakeFacade::enable_ctest() const
{
  // Todo: implement
}

void CMakeFacade::add_test(const std::string& test_executable_name)
{
  // Todo: implement
}

cmsl::facade::cmake_facade::cxx_compiler_info
CMakeFacade::get_cxx_compiler_info() const
{
  // Todo: implement
  cmsl::facade::cmake_facade::cxx_compiler_info info{};
  const auto cxx_compiler_id =
    m_makefile->GetDefinition("CMAKE_CXX_COMPILER_ID");
  if (cxx_compiler_id == std::string{ "Clang" }) {
    info.id = cmsl::facade::cmake_facade::cxx_compiler_id ::clang;
  }

  return info;
}

std::optional<std::string> CMakeFacade::try_get_extern_define(
  const std::string& name) const
{
  // Todo: implement
  return std::nullopt;
}

std::string CMakeFacade::join_paths(
  const std::vector<std::string>& paths) const
{
  std::string dirs;
  std::string sep;
  std::string prefix = m_makefile->GetCurrentSourceDirectory() + "/";
  for (const auto& path : paths) {
    if (cmSystemTools::FileIsFullPath(path) ||
        cmGeneratorExpression::Find(path) == 0) {
      dirs += sep + path;
    } else {
      dirs += sep + prefix + path;
    }
    sep = ";";
  }
  return dirs;
}