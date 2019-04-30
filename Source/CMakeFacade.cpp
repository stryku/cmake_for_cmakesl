#include "CMakeFacade.hpp"

#include "cmGlobalGenerator.h"
#include "cmMakefile.h"
#include "cmVersion.h"
#include "cmake.h"

CMakeFacade::CMakeFacade(cmMakefile& makefile)
  : m_makefile{ &makefile }
{
}

cmsl::facade::cmake_facade::version CMakeFacade::get_cmake_version() const
{
  return { cmVersion::GetMajorVersion(), cmVersion::GetMinorVersion(),
           cmVersion::GetPatchVersion(), cmVersion::GetTweakVersion() };
}

void CMakeFacade::fatal_error(const std::string&) const
{
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
                                      const std::string& library_name)
{
  auto target =
    m_makefile->GetCMakeInstance()->GetGlobalGenerator()->FindTarget(
      target_name);
  if (target == nullptr) {
    return;
  }

  target->AddLinkLibrary(*m_makefile, library_name,
                         cmTargetLinkLibraryType::GENERAL_LibraryType);
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
