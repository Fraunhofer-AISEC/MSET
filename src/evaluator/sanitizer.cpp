/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include "sanitizer.h"
#include "config.h"

#include <iostream>
#include <sstream>
#include <tuple>
#include <unistd.h>
#include <sys/wait.h>
#include <csignal>

#include "evaluator/tinyxml2.h"

Sanitizer::Sanitizer(const std::string &config_path)
{
  tinyxml2::XMLDocument doc;
  if (tinyxml2::XML_SUCCESS != doc.LoadFile( config_path.c_str() ))
  {
    std::cerr << "Failed to parse the configuration file at " << config_path << ": " << doc.ErrorStr() << '\n';
    exit(EXIT_FAILURE);
  }
  tinyxml2::XMLElement *root = doc.FirstChildElement("sanitizer");
  if (!root)
  {
    std::cerr << "Mandatory root element 'sanitizer' is missing! Cannot continue.\n";
    std::cerr << doc.ErrorStr() << '\n';
    exit(EXIT_FAILURE);
  }

  tinyxml2::XMLElement *setup_element = root->FirstChildElement("setup");
  if (!setup_element)
  {
    std::cerr << "Mandatory 'setup' element not found. Cannot continue.\n";
    exit(EXIT_FAILURE);
  }

  // at least the compile command is expected
  tinyxml2::XMLElement *compile_cmd_elem = setup_element->FirstChildElement("compile_cmd");
  if (!compile_cmd_elem)
  {
    std::cerr << "The setup 'compile_cmd' is mandatory. Cannot continue.\n";
    exit(EXIT_FAILURE);
  }
  compile_command = compile_cmd_elem->GetText();

  tinyxml2::XMLElement *command = compile_cmd_elem->NextSiblingElement("cmd");
  while (command) // optional
  {
    setup_commands.emplace_back( command->GetText() );
    command = command->NextSiblingElement("cmd");
  }


  tinyxml2::XMLElement *baseline_setup_element = root->FirstChildElement("setup_baseline");
  if (!baseline_setup_element)
  {
    std::cerr << "The baseline setup 'compile_cmd' is mandatory. Cannot continue.\n";
    exit(EXIT_FAILURE);
  }

  // at least the compile_cmd is expected
  compile_cmd_elem = baseline_setup_element->FirstChildElement("compile_cmd");
  if (!compile_cmd_elem)
  {
    std::cerr << "At least one 'cmd' is expected. Cannot continue.\n";
    exit(EXIT_FAILURE);
  }
  baseline_compile_command = compile_cmd_elem->GetText();

  command = compile_cmd_elem->NextSiblingElement("cmd");
  while (command) // optional
  {
    baseline_setup_commands.emplace_back(command->GetText() );
    command = command->NextSiblingElement("cmd");
  }

  tinyxml2::XMLElement *run = root->FirstChildElement("run");
  if (!run)
  {
    std::cerr << "model 'run' element is missing! Cannot continue.\n";
    std::cerr << doc.ErrorStr() << '\n';
    exit(EXIT_FAILURE);
  }
  tinyxml2::XMLElement *run_env_args = root->FirstChildElement("run_env_args");
  if (run_env_args) // optional
  {
    tinyxml2::XMLElement *env_var = run_env_args->FirstChildElement("env_var");
    while (env_var)
    {
      const char *name = env_var->Attribute("name");
      const char *value = env_var->GetText();
      env_var = env_var->NextSiblingElement("env_var");
      exec_env_vars.push_back( std::make_tuple<std::string, std::string>(name, value) );
    }
  }

  tinyxml2::XMLElement *run_baseline = root->FirstChildElement("run_baseline");
  if (!run_baseline)
  {
    std::cerr << "model 'run_baseline' element is missing! Cannot continue.\n";
    std::cerr << doc.ErrorStr() << '\n';
    exit(EXIT_FAILURE);
  }
  tinyxml2::XMLElement *name_elem = root->FirstChildElement("name");
  if (!name_elem)
  {
    std::cerr << "model 'name' element is missing! Cannot continue.\n";
    std::cerr << doc.ErrorStr() << '\n';
    exit(EXIT_FAILURE);
  }

  char *endptr;
  // test case failed => bug detected
  tinyxml2::XMLElement *elem = root->FirstChildElement("bug_detected_exit_values");
  if (elem) // optional
  {
    elem = elem->FirstChildElement("value");
    while (elem)
    {
      errno = 0;
      long value = std::strtol( elem->GetText(), &endptr, 10 );
      if (errno)
      {
        std::perror("Value of bug_detected_exit_values must be an integer");
        exit(EXIT_FAILURE);
      }
      test_case_failed_exit_values.insert( static_cast<int>(value) );
      elem = elem->NextSiblingElement("value");
    }
    if ( test_case_failed_exit_values.empty() )
    {
      std::cerr << "Values of 'bug_detected_exit_values' are missing.\n";
      exit(EXIT_FAILURE);
    }
  }
  else
  {
    // not configured
    test_case_failed_exit_values = {DEFAULT_TEST_CASE_FAILED_EXIT_VALUE};
  }

  // test case successful => bug not detected
  elem = root->FirstChildElement("bug_not_detected_exit_value");
  if (elem) // optional
  {
    errno = 0;
    long value = std::strtol( elem->GetText(), &endptr, 10 );
    if (errno)
    {
      std::perror("Value of 'bug_not_detected_exit_value' must be an integer");
      exit(EXIT_FAILURE);
    }
    test_case_successful_exit_value = static_cast<int>(value);
  }
  else
  {
    // not configured
    test_case_successful_exit_value = DEFAULT_TEST_CASE_SUCCESSFUL_EXIT_VALUE;
  }

  elem = root->FirstChildElement("precondition_not_met_exit_value");
  if (elem) // optional
  {
    errno = 0;
    long value = std::strtol( elem->GetText(), &endptr, 10 );
    if (errno)
    {
      std::perror("Value of 'precondition_not_met_exit_value' must be an integer");
      exit(EXIT_FAILURE);
    }
    preconditions_not_met_exit_value = static_cast<int>(value);
  }
  else
  {
    // not configured
    preconditions_not_met_exit_value = DEFAULT_PRECONDITIONS_NOT_MET_EXIT_VALUE;
  }

  elem = root->FirstChildElement("timeout_exit_value");
  if (elem) // optional
  {
    errno = 0;
    long value = std::strtol( elem->GetText(), &endptr, 10 );
    if (errno)
    {
      std::perror("Value of 'timeout_exit_value' must be an integer");
      exit(EXIT_FAILURE);
    }
    timeout_exit_value = static_cast<int>(value);
  }
  else
  {
    // not configured
    timeout_exit_value = DEFAULT_TIMEOUT_EXIT_VALUE;
  }

  elem = root->FirstChildElement("timeout_seconds");
  if (elem) // optional
  {
    errno = 0;
    long value = std::strtol( elem->GetText(), &endptr, 10 );
    if (errno)
    {
      std::perror("Value of 'timeout_seconds' must be an integer");
      exit(EXIT_FAILURE);
    }
    timeout_in_secs = static_cast<int>(value);
  }
  else
  {
    // not configured
    timeout_in_secs = DEFAULT_TIMEOUT_IN_SECS;
  }

  execute_command = run->GetText();
  baseline_execute_command = run_baseline->GetText();
  sanitizer_name = name_elem->GetText();

  defines = "-DTEST_CASE_SUCCESSFUL_VALUE=" + std::to_string(test_case_successful_exit_value) + " ";
  defines += "-DPRECONDITIONS_FAILED_VALUE=" + std::to_string(preconditions_not_met_exit_value);


  elem = root->FirstChildElement("address_mask");
  if (elem) // optional
  {
    errno = 0;
    (void)std::strtol( elem->GetText(), &endptr, 16 );
    if (errno)
    {
      std::perror("Value of 'address_mask' must be an hexadecimal integer");
      exit(EXIT_FAILURE);
    }
    defines += " -DADDR_MASK=" + std::string(elem->GetText());
  }
}

bool Sanitizer::compile(const std::string &src_file_path, const std::string &resulted_binary_path) const
{
  return _compile(src_file_path, resulted_binary_path, compile_command, setup_commands);
}

bool Sanitizer::compile_baseline(const std::string &src_file_path, const std::string &resulted_binary_path) const
{
  return _compile(src_file_path, resulted_binary_path, baseline_compile_command, baseline_setup_commands);
}

bool Sanitizer::_compile(const std::string &src_file_path, const std::string &resulted_binary_path,
  const std::string &compile_cmd, const std::vector<std::string> &cmds) const
{
  if ( setenv( "SOURCE_FILE", src_file_path.c_str(), 1 ) != 0 )
  {
    std::cerr << "Failed to set env SOURCE_FILE!\n";
    exit(EXIT_FAILURE);
  }

  if ( setenv( "GENERATED_BINARY", resulted_binary_path.c_str(), 1 ) != 0 )
  {
    std::cerr << "Failed to set env GENERATED_BINARY!\n";
    exit(EXIT_FAILURE);
  }

  int res;
  if ( (res = system( (compile_cmd + " " + defines).c_str() ) ) != 0)
  {
    std::cerr << "Command " << compile_cmd << " " << defines << " failed: " << res << "\n";
    return false;
  }

  for (const std::string &cmd: cmds)
  {
    if ( (res = system( cmd.c_str() ) ) != 0)
    {
      std::cerr << "Command " << cmd << " " << defines << " failed: " << res << "\n";
      return false;
    }
  }

  return true;
}


exec_result_t Sanitizer::execute(const std::string &binary_path) const
{
  if ( access(binary_path.c_str(), X_OK) != 0 )
  {
    std::cerr << "File is not executable: " << binary_path << "\n";
    exit(EXIT_FAILURE);
  }
  std::vector<std::string> binary_args;
  std::istringstream command_stream(execute_command);

  std::string token;
  std::string executable;
  if (command_stream >> token)
  {
    size_t index = token.find("$GENERATED_BINARY", 0);
    if (index != std::string::npos)
    {
      // reserved name, to be replaced
      token.replace(index, std::string("$GENERATED_BINARY").size(), binary_path);
    }
    executable = token;
  }
  if (executable.empty())
  {
    std::cerr << "Execute command not given!\n";
    exit(EXIT_FAILURE);
  }
  if ( access(executable.c_str(), X_OK) != 0 )
  {
    std::cerr << "File is not executable: " << executable << "\n";
    exit(EXIT_FAILURE);
  }
  while (command_stream >> token)
  {
    size_t index = token.find("$GENERATED_BINARY", 0);
    if (index != std::string::npos)
    {
      // reserved name, to be replaced
      token.replace(index, std::string("$GENERATED_BINARY").size(), binary_path);
    }
    binary_args.push_back(token);
  }


  return _execute(
    executable,
    binary_args,
    exec_env_vars,
    timeout_in_secs
  );
}


exec_result_t Sanitizer::execute_baseline(const std::string &binary_path) const
{
  if ( access(binary_path.c_str(), X_OK) != 0 )
  {
    std::cerr << "File is not executable: " << binary_path << "\n";
    exit(EXIT_FAILURE);
  }
  std::vector<std::string> binary_args;
  std::istringstream command_stream(baseline_execute_command);

  std::string token;
  std::string executable;
  if (command_stream >> token)
  {
    size_t index = token.find("$GENERATED_BINARY", 0);
    if (index != std::string::npos)
    {
      // reserved name, to be replaced
      token.replace(index, std::string("$GENERATED_BINARY").size(), binary_path);
    }
    executable = token;
  }
  if (executable.empty())
  {
    std::cerr << "Execute command not given!\n";
    exit(EXIT_FAILURE);
  }
  if ( access(executable.c_str(), X_OK) != 0 )
  {
    std::cerr << "File is not executable: " << executable << "\n";
    exit(EXIT_FAILURE);
  }
  while (command_stream >> token)
  {
    size_t index = token.find("$GENERATED_BINARY", 0);
    if (index != std::string::npos)
    {
      // reserved name, to be replaced
      token.replace(index, std::string("$GENERATED_BINARY").size(), binary_path);
    }
    binary_args.push_back(token);
  }

  return _execute(
    executable,
    binary_args,
    {},
    timeout_in_secs
  );
}

exec_result_t Sanitizer::_execute(
  const std::string &binary_path,
  const std::vector<std::string> &binary_args,
  const std::vector< std::tuple<std::string, std::string> > &env_vars,
  const int timeout_s
) const
{
  pid_t pid = fork();
  if (pid == -1)
  {
    perror("fork");
    exit(EXIT_FAILURE);
  }
  
  if (pid == 0)
  {
    pid_t test_case_pid = fork();
    if (test_case_pid == -1)
    {
      perror("fork");
      exit(EXIT_FAILURE);
    }
    if (test_case_pid == 0)
    {
      // set sanitizer specific environment variables for this test case
      for (auto env_var: env_vars)
      {
        setenv(std::get<0>(env_var).c_str(), std::get<1>(env_var).c_str(), /*overwrite=*/1);
      }
      // cast given args to C array of buffers
      const char **C_args = new const char *[binary_args.size() + 2]; // needs binary_path and NULL
      C_args[0] = binary_path.c_str();
      for (size_t index = 0; index < binary_args.size(); index++)
      {
        C_args[index + 1] = binary_args[index].c_str();
      }
      C_args[binary_args.size() + 1] = nullptr; // NULL terminated array
      // run test case
      extern char **environ;
      execve(binary_path.c_str(), (char **)C_args, environ);
    }

    pid_t timeout_pid = fork();
    if (timeout_pid == 0)
    {
      sleep(timeout_s);
      _exit(0);
    }

    int return_value = 0;
    int wstatus;
    int exit_pid = wait(&wstatus);
    if (exit_pid == test_case_pid)
    {
      // exit without timeout
      if ( WIFSIGNALED(wstatus) )
      {
        return_value = WTERMSIG(wstatus);
      }
      else if ( WIFEXITED(wstatus) )
      {
        return_value = WEXITSTATUS(wstatus);
      }
      kill(timeout_pid, SIGKILL); // stop waiting for timeout
      wait(nullptr); // collect timeout process
    }
    else
    {
      std::cerr << "Test case ended in timeout\n";
      kill(test_case_pid, SIGKILL); // kill test case
      wait(nullptr); // collect test case process
      return_value = timeout_exit_value; // notify timeout
    }
    exit(return_value); // notify parent of return value
  }

  int wstatus;
  int return_value = 0;
  waitpid(pid, &wstatus, 0);
  if ( WIFSIGNALED(wstatus) )
  {
    return_value = WTERMSIG(wstatus);
  }
  else if ( WIFEXITED(wstatus) )
  {
    return_value = WEXITSTATUS(wstatus);
  }

  if ( return_value == preconditions_not_met_exit_value) return PRECONDITIONS_FAILED;
  if ( return_value == SIGSEGV) return FAILED_SIGSEGV;
  if ( test_case_failed_exit_values.find(return_value) != test_case_failed_exit_values.end() ) return FAILED;
  if ( return_value == test_case_successful_exit_value) return SUCCESSFUL;
  if ( return_value == timeout_exit_value) return TIMEOUT;
  std::cerr << "WARNING: Execution return value " << return_value << " not configured. Assume bug detected\n";
  return FAILED;
}
