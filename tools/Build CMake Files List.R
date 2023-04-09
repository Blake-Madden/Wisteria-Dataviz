library(tidyverse)
library(magrittr)
library(stringr)

srcFolder <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/../src')
testFolder <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/../tests')
buildFile <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/files.cmake')
buildFileTestLib <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/libfiles_testing.cmake')
buildFileTests <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/testfiles.cmake')

files <- str_glue("src/{list.files(path=srcFolder, pattern='(*[.]cpp|cJSON[.]c)', recursive=TRUE)}")
# remove test and sample files from submodules
files <- files[!grepl("(demo.cpp|main.cpp|i18n-check/samples|i18n-check/tests/|cpp_i18n_review.cpp|i18n_review.cpp|utfcpp/tests/|utfcpp/samples/|utfcpp/extern|cxxopts)", files)]
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
           file=buildFile)


files <- stringr::str_replace(files, "src/", "../src/")
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# This should be used for the unit test runner.
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
           file=buildFileTestLib)

files <- str_glue("{list.files(path=testFolder, pattern='(*[.]cpp)', recursive=FALSE)}")
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# This should be used for the unit test runner.
# DO NOT MODIFY MANUALLY!

SET(TEST_SRC_FILES\n    {paste(files, collapse='\n    ')})"),
           file=buildFileTests)