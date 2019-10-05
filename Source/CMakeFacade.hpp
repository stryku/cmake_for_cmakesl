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

  void message(const std::string& what) const override;
  void warning(const std::string& what) const override;
  void error(const std::string& what) const override;
  void fatal_error(const std::string& what) override;
  bool did_fatal_error_occure() const override;

  void install(const std::string& target_name) override;

  void register_project(const std::string& name) override;

  std::string get_current_binary_dir() const override;
  std::string get_current_source_dir() const override;
  std::string get_root_source_dir() const override;

  void add_custom_command(const std::vector<std::string>& command,
                          const std::string& output) const override;

  void make_directory(const std::string& dir) const override;

  void add_executable(const std::string& name,
                      const std::vector<std::string>& sources) override;
  void add_library(const std::string& name,
                   const std::vector<std::string>& sources) override;

  void target_link_library(const std::string& target_name,
                           cmsl::facade::visibility v,
                           const std::string& library_name) override;

  void target_include_directories(
    const std::string& target_name, cmsl::facade::visibility v,
    const std::vector<std::string>& dirs) override;

  std::string current_directory() const override;
  void go_into_subdirectory(const std::string& dir) override;
  void go_directory_up() override;

  void enable_ctest() const override;

  void add_test(const std::string& test_executable_name) override;

  cxx_compiler_info get_cxx_compiler_info() const override;

  std::optional<std::string> try_get_extern_define(
    const std::string& name) const override;

  void set_property(const std::string& property_name,
                    const std::string& property_value) const override;

  std::optional<bool> get_option_value(const std::string& name) const override;

  void register_option(const std::string& name, const std::string& description,
                       bool value) const override;

private:
  std::string join_paths(const std::vector<std::string>& paths) const;
  std::string adjust_property_to_cmake_interface(
    const std::string& name, std::string cmakesl_value) const;

  std::vector<std::string> convert_to_full_paths(
    std::vector<std::string> paths) const;

private:
  cmMakefile* m_makefile;
  std::vector<std::string> m_directories;
  bool m_did_fatal_error_occure{ false };
};
