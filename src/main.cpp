/*
 * This file is distributed under the Apache License, Version 2.0; refer to
 * LICENSE for details.
 *
 * Initial author: Emanuel Vintila
 */

#include <iostream>
#include <vector>

#include "arg_parser.h"
#include "config.h"
#include "misc.h"
#include "generator/generator.h"

static const std::string DEFAULT_GENERATED_DIR_NAME = "test_cases_mlir";
static const std::string DEFAULT_GENERATED_PATH = "../" + DEFAULT_GENERATED_DIR_NAME;

static std::string generated_path;
static bool do_generate = false;
static bool remove_dir = false;

const std::vector<std::tuple< std::string, ArgParser::Argument>> accepted_arguments
{
  // arg name,                                                          has_value, value_name,              default_value,              description,   [hidden]
  std::make_tuple( "--generate",                    ArgParser::Argument{false,     "",                      "",                         "\t\t\t\tRegenerate test cases. The generated files will be placed in <TEST_CASE_DIR>. If the directory is not empty and --clean-test-cases is not specified, the test case generation is aborted."} ),
  std::make_tuple( "--test-case-dir",               ArgParser::Argument{true,      "<TEST_CASE_DIR>",       DEFAULT_GENERATED_DIR_NAME, "\t\tSpecify <TEST_CASE_DIR> as the location for the generated test case files. Default: \"../" + DEFAULT_GENERATED_DIR_NAME + "/\"."} ),
  std::make_tuple( "--clean-test-cases",            ArgParser::Argument{false,     "",                      "",                         "\t\t\t\tRemove all contents from <TEST_CASE_DIR> before generating test cases. This option is applicable only when --generate is specified."} ),
  std::make_tuple( "--help",                        ArgParser::Argument{false,     "",                      "",                         "\t\t\t\t\tShow this help message and exit."} )
};

static void print_usage()
{
  std::cout << "Usage: ./mset\n";
  for ( const auto& accepted_arg : accepted_arguments )
  {
    ArgParser::Argument arg = std::get<1>(accepted_arg);
    if ( arg.hidden )
    {
      continue;
    }
    std::cout << "[" << std::get<0>(accepted_arg);
    if (arg.has_value) std::cout << " " << arg.value_name;
    std::cout << "]";
    std::cout << arg.description << std::endl;
  }
}

static bool parse_arguments(int argc, char **argv)
{
  std::unique_ptr<ArgParser> parser = ArgParser::construct( argc, const_cast<const char **>(argv), accepted_arguments );

  if ( !parser )
  {
    print_usage();
    return false;
  }

  if ( parser->check_and_consume("--help") )
  {
    print_usage();
    return false;
  }

  do_generate = parser->check_and_consume("--generate");
  remove_dir = parser->check_and_consume("--clean-test-cases");

  if ( !do_generate && !remove_dir  )
  {
    std::cerr << "You must either specify --generate or --clean-test-cases." << std::endl;
    print_usage();
    return false;
  }

  std::unique_ptr<std::string> test_case_dir = parser->get_value_and_consume("--test-case-dir");
  if ( test_case_dir )
  {
    generated_path = *test_case_dir;
  }
  else
  {
    generated_path = DEFAULT_GENERATED_PATH;
  }
  if ( generated_path.back() != '/' )
  {
    generated_path += "/";
  }

  if ( !do_generate )
  {
    if ( remove_dir )
    {
      std::cerr << "WARNING: --clean-test-cases used when not generating (--generate).\n";
    }
  }

  if ( !parser->consumed_everything() )
  {
    std::list<std::string> args = parser->get_unconsumed();
    for ( std::string &arg: args )
    {
      std::cerr << "Unknown argument: " << arg << std::endl;
    }
    print_usage();
    return false;
  }
  return true;
}

int main( int argc, char **argv )
{
  if ( !parse_arguments( argc, argv ) )
  {
    return 1;
  }

  if ( remove_dir )
  {
    if ( directory_exists( generated_path ) )
    {
      remove_all_files_from_directory( generated_path );
    }
  }

  if ( do_generate )
  {
    if ( directory_exists( generated_path ) )
    {
      if ( !is_directory_empty( generated_path ) )
      {
        std::cerr << "Error: The directory '" << generated_path << "' is not empty. To regenerate test cases, use the --clean-test-cases option to remove existing ones. Alternatively, use the --test-case-dir option to specify a different directory for generating test cases. Aborting." << std::endl;
        return 1;
      }
    }
    else
    {
      create_directory( generated_path );
    }
    std::cout << "Generating test cases in: '" << generated_path << "'" << std::endl;
    generate( generated_path );
  }

  return 0;
}
