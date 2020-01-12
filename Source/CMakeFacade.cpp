#include "CMakeFacade.hpp"

#include "cmCustomCommandLines.h"
#include "cmGlobalGenerator.h"
#include "cmInstallCommandArguments.h"
#include "cmInstallTargetGenerator.h"
#include "cmMakefile.h"
#include "cmTest.h"
#include "cmTestGenerator.h"
#include "cmVersion.h"
#include "cmake.h"
#include "cmsys/SystemInformation.hxx"

#include "exec/instance/instance.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>

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
{
  m_makefiles.push(&makefile);
}

CMakeFacade::~CMakeFacade() = default;

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
  makefile().SetProjectName(name);

  std::string bindir = name;
  bindir += "_BINARY_DIR";
  std::string srcdir = name;
  srcdir += "_SOURCE_DIR";

  makefile().AddCacheDefinition(
    bindir, makefile().GetCurrentBinaryDirectory().c_str(),
    "Value Computed by CMake", cmStateEnums::STATIC);
  makefile().AddCacheDefinition(
    srcdir, makefile().GetCurrentSourceDirectory().c_str(),
    "Value Computed by CMake", cmStateEnums::STATIC);

  bindir = "PROJECT_BINARY_DIR";
  srcdir = "PROJECT_SOURCE_DIR";

  makefile().AddDefinition(bindir,
                           makefile().GetCurrentBinaryDirectory().c_str());
  makefile().AddDefinition(srcdir,
                           makefile().GetCurrentSourceDirectory().c_str());

  makefile().EnableLanguage(std::vector<std::string>{ "C", "CXX" }, false);
}

std::string CMakeFacade::get_current_binary_dir() const
{
  return makefile().GetCurrentBinaryDirectory();
}

std::string CMakeFacade::get_current_source_dir() const
{
  return makefile().GetCurrentSourceDirectory();
}

std::string CMakeFacade::get_root_source_dir() const
{
  return get_current_source_dir();
}

void CMakeFacade::add_executable(const std::string& name,
                                 const std::vector<std::string>& sources)
{
  makefile().AddExecutable(name, convert_to_full_paths(sources));
}

void CMakeFacade::add_library(const std::string& name,
                              const std::vector<std::string>& sources)
{
  makefile().AddLibrary(name, cmStateEnums::TargetType::STATIC_LIBRARY,
                        convert_to_full_paths(sources));
}

void CMakeFacade::target_link_library(const std::string& target_name,
                                      cmsl::facade::visibility v,
                                      const std::string& library_name)
{
  auto target =
    makefile().GetCMakeInstance()->GetGlobalGenerator()->FindTarget(
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
    target->AddLinkLibrary(makefile(), library_name,
                           cmTargetLinkLibraryType::GENERAL_LibraryType);
  }
}

std::string CMakeFacade::current_directory() const
{
  return makefile().GetCurrentSourceDirectory();

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
  //  m_directories.emplace_back(dir);
}

void CMakeFacade::go_directory_up()
{
  //  m_directories.pop_back();
  m_makefiles.pop();
}

void CMakeFacade::install(const std::string& target_name,
                          const std::string& destination)
{
  makefile().GetGlobalGenerator()->EnableInstallTarget();

  cmTarget* target = makefile().FindLocalNonAliasTarget(target_name);
  if (!target) {
    cmTarget* const global_target =
      makefile().GetGlobalGenerator()->FindTarget(target_name, true);
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
    target->GetName(), destination.c_str(), false, "", {}, "Unspecified",
    message, false, false, makefile().GetBacktrace());

  makefile().AddInstallGenerator(generator);
  makefile().GetGlobalGenerator()->AddInstallComponent("Unspecified");
}

void CMakeFacade::target_include_directories(
  const std::string& target_name, cmsl::facade::visibility v,
  const std::vector<std::string>& dirs)
{
  auto target =
    makefile().GetCMakeInstance()->GetGlobalGenerator()->FindTarget(
      target_name);
  const auto joined = join_paths(dirs);

  if (v == cmsl::facade::visibility::interface) {
    target->AppendProperty("INTERFACE_INCLUDE_DIRECTORIES", joined.c_str());
  } else {
    cmListFileBacktrace lfbt = makefile().GetBacktrace();
    target->InsertInclude(joined, lfbt, /*before=*/false);
  }
}

std::string CMakeFacade::join_for_compile_definitions(
  const std::vector<std::string>& content) const
{
  std::string defs;
  std::string sep;
  for (std::string const& it : content) {
    if (cmHasLiteralPrefix(it, "-D")) {
      defs += sep + it.substr(2);
    } else {
      defs += sep + it;
    }
    sep = ";";
  }
  return defs;
}

void CMakeFacade::target_compile_definitions(
  const std::string& target_name, cmsl::facade::visibility v,
  const std::vector<std::string>& definitions)
{
  auto target =
    makefile().GetCMakeInstance()->GetGlobalGenerator()->FindTarget(
      target_name);
  const auto joined = join_for_compile_definitions(definitions);

  target->AppendProperty("COMPILE_DEFINITIONS", joined.c_str());
}

void CMakeFacade::target_compile_options(
  const std::string& target_name, cmsl::facade::visibility v,
  const std::vector<std::string>& options)
{
  auto target =
    makefile().GetCMakeInstance()->GetGlobalGenerator()->FindTarget(
      target_name);
  const auto joined = join_for_compile_definitions(options);

  target->AppendProperty("COMPILE_OPTIONS", joined.c_str());
}

void CMakeFacade::enable_ctest() const
{
  makefile().AddDefinition("CMAKE_TESTING_ENABLED", "1");
  const auto name = makefile().GetModulesFile("CTest.cmake");
  std::string listFile = cmSystemTools::CollapseFullPath(
    name, makefile().GetCurrentSourceDirectory());
  makefile().ReadDependentFile(listFile, /*noPolicyScope=*/false);
}

void CMakeFacade::add_test(const std::string& test_executable_name)
{
  // Collect the command with arguments.
  std::vector<std::string> command{ test_executable_name };

  // Create the test but add a generator only the first time it is
  // seen.  This preserves behavior from before test generators.
  cmTest* test = makefile().GetTest(test_executable_name);
  if (test) {
    // If the test was already added by a new-style signature do not
    // allow it to be duplicated.
    if (!test->GetOldStyle()) {
      std::ostringstream e;
      e << " given test name \"" << test_executable_name
        << "\" which already exists in this directory.";
      fatal_error(e.str());
      return;
    }
  } else {
    test = makefile().CreateTest(test_executable_name);
    test->SetOldStyle(true);
    makefile().AddTestGenerator(new cmTestGenerator(test));
  }

  test->SetCommand(command);
}

cmsl::facade::cmake_facade::cxx_compiler_info
CMakeFacade::get_cxx_compiler_info() const
{
  cmsl::facade::cmake_facade::cxx_compiler_info info{};
  const auto cxx_compiler_id =
    makefile().GetDefinition("CMAKE_CXX_COMPILER_ID");
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
  std::string prefix = makefile().GetCurrentSourceDirectory() + "/";
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

std::optional<bool> CMakeFacade::get_option_value(
  const std::string& name) const
{
  const char* existingValue = makefile().GetState()->GetCacheEntryValue(name);

  if (existingValue == nullptr) {
    return std::nullopt;
  }

  return cmSystemTools::IsOn(existingValue);
}

void CMakeFacade::register_option(const std::string& name,
                                  const std::string& description,
                                  bool value) const
{
  makefile().AddCacheDefinition(name, value ? "ON" : "OFF",
                                description.c_str(), cmStateEnums::BOOL);
}

void CMakeFacade::set_property(const std::string& property_name,
                               const std::string& property_value) const
{
  const auto adjusted_property =
    adjust_property_to_cmake_interface(property_name, property_value);
  makefile().AddDefinition(property_name, adjusted_property.c_str());
}

void CMakeFacade::add_custom_command(const std::vector<std::string>& command,
                                     const std::string& output) const
{
  cmCustomCommandLines command_lines;
  command_lines.push_back(cmCustomCommandLine{ command });

  makefile().AddCustomCommandToOutput(output, {}, "", command_lines, nullptr,
                                      "");
}

void CMakeFacade::make_directory(const std::string& dir) const
{
  cmSystemTools::MakeDirectory(dir);
}

std::string CMakeFacade::adjust_property_to_cmake_interface(
  const std::string& name, std::string cmakesl_value) const
{
  if (name == "CMAKE_CXX_STANDARD") {
    if (cmakesl_value == "cpp_11") {
      return "11";
    }
    if (cmakesl_value == "cpp_14") {
      return "14";
    }
    if (cmakesl_value == "cpp_17") {
      return "17";
    }
    if (cmakesl_value == "cpp_20") {
      return "20";
    }
  }

  return std::move(cmakesl_value);
}
std::vector<std::string> CMakeFacade::convert_to_full_paths(
  std::vector<std::string> paths) const
{
  for (auto& src : paths) {
    if (!cmSystemTools::FileIsFullPath(src)) {
      src = current_directory() + '/' + src;
    }
  }

  return std::move(paths);
}

void CMakeFacade::add_subdirectory_with_old_script(const std::string& dir)
{
  // Compute the full path to the specified source directory.
  // Interpret a relative path with respect to the current source directory.
  std::string srcPath;
  if (cmSystemTools::FileIsFullPath(dir)) {
    srcPath = dir;
  } else {
    srcPath = makefile().GetCurrentSourceDirectory();
    srcPath += "/";
    srcPath += dir;
  }
  if (!cmSystemTools::FileIsDirectory(srcPath)) {
    /* Todo: Handle error
    std::string error = "given source \"";
    error += dir;
    error += "\" which is not an existing directory.";
    this->SetError(error);
    return false;
     */
    return;
  }
  srcPath = cmSystemTools::CollapseFullPath(srcPath);

  // Compute the full path to the binary directory.
  std::string binPath;
  // No binary directory was specified.  If the source directory is
  // not a subdirectory of the current directory then it is an
  // error.
  if (!cmSystemTools::IsSubDirectory(srcPath,
                                     makefile().GetCurrentSourceDirectory())) {
    /* Todo: Handle error
    std::ostringstream e;
    e << "not given a binary directory but the given source directory "
      << "\"" << srcPath << "\" is not a subdirectory of \""
      << makefile().GetCurrentSourceDirectory() << "\".  "
      << "When specifying an out-of-tree source a binary directory "
      << "must be explicitly specified.";
    this->SetError(e.str());
    return false;
     */
    return;
  }

  // Remove the CurrentDirectory from the srcPath and replace it
  // with the CurrentOutputDirectory.
  const std::string& src = makefile().GetCurrentSourceDirectory();
  const std::string& bin = makefile().GetCurrentBinaryDirectory();
  size_t srcLen = src.length();
  size_t binLen = bin.length();
  if (srcLen > 0 && src.back() == '/') {
    --srcLen;
  }
  if (binLen > 0 && bin.back() == '/') {
    --binLen;
  }
  binPath = bin.substr(0, binLen) + srcPath.substr(srcLen);

  binPath = cmSystemTools::CollapseFullPath(binPath);

  // Add the subdirectory using the computed full paths.
  makefile().AddSubDirectory(srcPath, binPath, /*excludeFromAll=*/false, true);
}

void CMakeFacade::set_old_style_variable(const std::string& name,
                                         const std::string& value) const
{
  makefile().AddDefinition(name, value.c_str());
}

cmsl::facade::cmake_facade::system_info CMakeFacade::get_system_info() const
{
  cmsys::SystemInformation info;

  const auto id = info.GetOSIsWindows()
    ? cmsl::facade::cmake_facade::system_id::windows
    : cmsl::facade::cmake_facade::system_id ::unix_;

  return cmsl::facade::cmake_facade::system_info{ .id = id };
}

void CMakeFacade::add_custom_target(
  const std::string& name, const std::vector<std::string>& command) const
{
  cmCustomCommandLine line;
  std::copy(std::cbegin(command), std::cend(command),
            std::back_inserter(line));

  // Save all command lines.
  cmCustomCommandLines commandLines;
  commandLines.push_back(std::move(line));

  auto target = makefile().AddUtilityCommand(
    name, cmMakefile::TargetOrigin::Project, /*excludeFromAll=*/true,
    /*working_directory=*/"", /*byproducts=*/{}, /*depends=*/{}, commandLines,
    /*escapeOldStyle=*/true, /*comment=*/"", /*uses_terminal=*/false,
    /*command_expand_lists=*/false);

  // Todo: uncomment when accepting sources is implemented.
  //  target->AddSources(sources);
}

std::string CMakeFacade::ctest_command() const
{
  return makefile().GetDefinition("CMAKE_CTEST_COMMAND");
}

std::string CMakeFacade::get_old_style_variable(const std::string& name) const
{
  return makefile().GetDefinition(name);
}

cmMakefile& CMakeFacade::makefile()
{
  return *m_makefiles.top();
}

cmMakefile& CMakeFacade::makefile() const
{
  return *m_makefiles.top();
}

void CMakeFacade::prepare_for_add_subdirectory_with_cmakesl_script(
  const std::string& dir)
{

  // This is basically copied from cmAddSubdirectoryCommand. Maybe it could be
  // extracted to a common function.

  std::string binPath;
  // No binary directory was specified.  If the source directory is
  // not a subdirectory of the current directory then it is an
  // error.
  auto srcDir = makefile().GetCurrentSourceDirectory() + '/' + dir;
  if (!cmSystemTools::IsSubDirectory(srcDir,
                                     makefile().GetCurrentSourceDirectory())) {
    std::ostringstream e;
    e << "not given a binary directory but the given source directory "
      << "\"" << srcDir << "\" is not a subdirectory of \""
      << makefile().GetCurrentSourceDirectory() << "\".  "
      << "When specifying an out-of-tree source a binary directory "
      << "must be explicitly specified.";
    fatal_error(e.str());
    return;
  }

  // Remove the CurrentDirectory from the srcPath and replace it
  // with the CurrentOutputDirectory.
  const std::string& src = makefile().GetCurrentSourceDirectory();
  const std::string& bin = makefile().GetCurrentBinaryDirectory();
  size_t srcLen = src.length();
  size_t binLen = bin.length();
  if (srcLen > 0 && src.back() == '/') {
    --srcLen;
  }
  if (binLen > 0 && bin.back() == '/') {
    --binLen;
  }
  binPath = bin.substr(0, binLen) + srcDir.substr(srcLen);
  binPath = cmSystemTools::CollapseFullPath(binPath);

  auto new_makefile = makefile().CreateMakefileForAddSubdirectory(
    srcDir, binPath, /*excludeFromAll=*/false);
  new_makefile->InitializeFromParent(&makefile());

  m_makefiles.push(new_makefile);
}

void CMakeFacade::finalize_after_add_subdirectory_with_cmakesl_script()
{
  m_makefiles.pop();
}
