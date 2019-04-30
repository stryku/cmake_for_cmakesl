#pragma once

#include "cmake_facade.hpp"

#include <string>
#include <vector>

class cmMakefile;

class CMakeFacade : public cmsl::facade::cmake_facade
{
public:
  explicit CMakeFacade(cmMakefile& makefile);

  version get_cmake_version() const override;
  void fatal_error(const std::string& what) const override;
  void register_project(const std::string& name) override;

  std::string get_current_binary_dir() const override;
  std::string get_current_source_dir() const override;
  void add_executable(const std::string& name,
                      const std::vector<std::string>& sources) override;
  void add_library(const std::string& name,
                   const std::vector<std::string>& sources) override;

  void target_link_library(const std::string& target_name,
                           const std::string& library_name) override;

  std::string current_directory() const override;
  void go_into_subdirectory(const std::string& dir) override;
  void go_directory_up() override;

private:
  cmMakefile* m_makefile;
  std::vector<std::string> m_directories;
};
