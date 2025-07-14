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
#include "evaluator/evaluator.h"
#include "generator/generator.h"

static const std::string DEFAULT_GENERATED_DIR_NAME = "test_cases";
static const std::string DEFAULT_GENERATED_PATH = "../" + DEFAULT_GENERATED_DIR_NAME;

static std::string generated_path;
static bool do_evaluate = false;
static bool do_evaluate_prebuilt = false;
static bool do_generate = false;
static bool do_compile = false;
static bool remove_dir = false;
static bool print_table_summary = false;
static bool verbose = false;
static bool evaluate_baseline = false;
static bool run_all_variants = false;
static bool keep_binaries = false;
static std::string sanitizer_config_path;

const std::vector<std::tuple< std::string, ArgParser::Argument>> accepted_arguments
{
  // arg name,                                                          has_value, value_name,              default_value,              description,   [hidden]
  std::make_tuple( "--evaluate",                    ArgParser::Argument{true,      "<SANITIZER_CONFIG>",    "",                         "\t\t\tEvaluate the sanitizer configured in <SANITIZER_CONFIG> using the test case files in <TEST_CASE_DIR>."} ),
  std::make_tuple( "--evaluate-baseline",           ArgParser::Argument{false ,    "",                      "",                         "\t\t\t\tEvaluate the baseline alongside the sanitizer. This option is applicable only when --evaluate is specified."} ),
  std::make_tuple( "--generate",                    ArgParser::Argument{false,     "",                      "",                         "\t\t\t\t\tRegenerate test cases. The generated files will be placed in <TEST_CASE_DIR>. If the directory is not empty and --clean-test-cases is not specified, the test case generation is aborted."} ),
  std::make_tuple( "--test-case-dir",               ArgParser::Argument{true,      "<TEST_CASE_DIR>",       DEFAULT_GENERATED_DIR_NAME, "\t\tSpecify <TEST_CASE_DIR> as the location for the generated test case files. Default: \"../" + DEFAULT_GENERATED_DIR_NAME + "/\"."} ),
  std::make_tuple( "--clean-test-cases",            ArgParser::Argument{false,     "",                      "",                         "\t\t\t\tRemove all contents from <TEST_CASE_DIR> before generating test cases. This option is applicable only when --generate is specified."} ),
  std::make_tuple( "--verbose",                     ArgParser::Argument{false ,    "",                      "",                         "\t\t\t\t\tPrint detailed evaluation results."} ),
  std::make_tuple( "--run-all-variants",            ArgParser::Argument{false ,    "",                      "",                         "\t\t\t\tRun all variants of a test case, even if one has already been successful. This option is applicable only when --evaluate is specified."} ),
  std::make_tuple( "--keep-binaries",               ArgParser::Argument{false ,    "",                      "",                         "\t\t\t\tKeep the test case binaries. This option is applicable only when --evaluate is specified."} ),
  std::make_tuple( "--evaluate-prebuilt-binaries",  ArgParser::Argument{true ,     "<SANITIZER_CONFIG>",    "",                         "\t\t\t\tEvaluate the sanitizer configured in <SANITIZER_CONFIG> using the pre-built binaries in <TEST_CASE_DIR>."} ),
  std::make_tuple( "--compile",                     ArgParser::Argument{true,      "<SANITIZER_CONFIG>",    "",                         "\t\t\tCompile all the test case files in <TEST_CASE_DIR> using the sanitizer configured in <SANITIZER_CONFIG>."} ),
  std::make_tuple( "--help",                        ArgParser::Argument{false,     "",                      "",                         "\t\t\t\t\tShow this help message and exit."} ),
  // hidden options:
  std::make_tuple( "--print-table-summary",         ArgParser::Argument{false ,    "",                      "",                         "", true } )
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
  do_evaluate = parser->check("--evaluate");
  do_evaluate_prebuilt = parser->check("--evaluate-prebuilt-binaries");
  do_compile = parser->check("--compile");
  remove_dir = parser->check_and_consume("--clean-test-cases");

  if ( !do_generate && !do_evaluate && !do_compile && !do_evaluate_prebuilt && !remove_dir  )
  {
    std::cerr << "You must either specify --generate, --evaluate, --evaluate-prebuilt-binaries, or --compile." << std::endl;
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

  print_table_summary = parser->check_and_consume("--print-table-summary");
  run_all_variants = parser->check_and_consume("--run-all-variants");
  keep_binaries = parser->check_and_consume("--keep-binaries");
  evaluate_baseline = parser->check_and_consume("--evaluate-baseline");
  verbose = parser->check_and_consume("--verbose");
  if ( do_evaluate )
  {
    if ( do_compile )
    {
      std::cerr << "--evaluate cannot be used with --compile." << std::endl;
      return false;
    }
    if ( do_evaluate_prebuilt )
    {
      std::cerr << "--evaluate cannot be used with --evaluate-prebuilt-binaries." << std::endl;
      return false;
    }
    std::unique_ptr<std::string> sanitizer_config_path_ptr = parser->get_value_and_consume("--evaluate");
    if ( !sanitizer_config_path_ptr )
    {
      std::cerr << "--evaluate requires a path to the sanitizer configuration file." << std::endl;
      print_usage();
      return false;
    }
    sanitizer_config_path = *sanitizer_config_path_ptr;
  }
  else
  {
    if ( !do_evaluate_prebuilt )
    {
      if ( print_table_summary )
      {
        std::cerr << "WARNING: --print-table-summary used when not evaluating.\n";
      }
      if ( run_all_variants )
      {
        std::cerr << "WARNING: --run-all-variants used when not evaluating.\n";
      }
      if ( evaluate_baseline )
      {
        std::cerr << "WARNING: --evaluate-baseline used when not evaluating.\n";
      }
      if ( keep_binaries )
      {
        std::cerr << "WARNING: --keep-binaries used when not evaluating.\n";
      }
    }
    else
    {
      if ( keep_binaries )
      {
        std::cerr << "WARNING: --keep-binaries ignores when evaluating prebuilt binaries.\n";
      }
    }
  }

  if ( !do_generate )
  {
    if ( remove_dir )
    {
      std::cerr << "WARNING: --clean-test-cases used when not generating (--generate).\n";
    }
  }

  if ( do_compile )
  {
    std::unique_ptr<std::string> sanitizer_config_path_ptr = parser->get_value_and_consume("--compile");
    if ( !sanitizer_config_path_ptr )
    {
      std::cerr << "--compile requires a path to the sanitizer configuration file." << std::endl;
      print_usage();
      return false;
    }
    if ( do_evaluate_prebuilt )
    {
      std::cerr << "--compile cannot be used with --evaluate-prebuilt-binaries." << std::endl;
      return false;
    }
    sanitizer_config_path = *sanitizer_config_path_ptr;
  }

  if ( do_evaluate_prebuilt )
  {
    std::unique_ptr<std::string> sanitizer_config_path_ptr = parser->get_value_and_consume("--evaluate-prebuilt-binaries");
    if ( !sanitizer_config_path_ptr )
    {
      std::cerr << "--evaluate-prebuilt-binaries requires a path to the sanitizer configuration file." << std::endl;
      print_usage();
      return false;
    }
    sanitizer_config_path = *sanitizer_config_path_ptr;
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

  if ( do_compile || do_evaluate || do_evaluate_prebuilt )
  {
    if ( !directory_exists( generated_path ) )
    {
      std::cerr << "Error: The directory '" << generated_path << "' does not exist. Aborting.\n";
      return 1;
    }
    if ( is_directory_empty( generated_path ) )
    {
      std::cerr << "Error: The directory '" << generated_path << "' is empty. Aborting.\n";
      return 1;
    }
  }

  std::string test_case_binaries_path = generated_path + "/" + TEST_CASE_BINARIES_DIR_NAME;
  bool test_cases_binaries_dir_exists = directory_exists( test_case_binaries_path );
  if ( do_compile )
  {
    std::cout << "Compiling test cases from: '" << generated_path << "'" << std::endl;
    if ( !test_cases_binaries_dir_exists )
    {
      create_directory( test_case_binaries_path );
    }
    compile_all( generated_path, sanitizer_config_path, verbose );
  }
  else if ( do_evaluate_prebuilt )
  {
    if ( !test_cases_binaries_dir_exists )
    {
      std::cerr << "Error: The directory '" << test_case_binaries_path << "' does not exist. Aborting.\n";
      return 1;
    }
    std::cout << "Evaluating pre-built binaries from: '" << generated_path << "'" << std::endl;
    evaluate_prebuilt_binaries( generated_path, sanitizer_config_path, print_table_summary, run_all_variants, verbose, evaluate_baseline );
  }
  else if ( do_evaluate )
  {
    if ( !test_cases_binaries_dir_exists )
    {
      create_directory( test_case_binaries_path );
    }
    std::cout << "Evaluating test cases from: '" << generated_path << "'" << std::endl;
    compile_and_evaluate( generated_path, sanitizer_config_path, print_table_summary, run_all_variants, verbose, evaluate_baseline, keep_binaries );
  }

  return 0;
}
