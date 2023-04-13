library(tidyverse)
library(magrittr)
library(stringr)

srcFolder <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/../src')
testFolder <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/../tests')
guiTestFolder <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/../tests/gui-tests')

buildLibFile <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/libfiles.cmake')
buildFile <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/files.cmake')
buildFileTestLib <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/libfiles_testing.cmake')
buildFileTestGuiLib <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/guilibfiles_testing.cmake')
buildFileTests <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/testfiles.cmake')
buildFileTestsGui <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/guitestfiles.cmake')

# Files for library
################################################
files <- str_glue("src/{list.files(path=srcFolder, pattern='(*[.]cpp|cJSON[.]c)', recursive=TRUE)}")
# remove test and sample files from submodules
files <- files[!grepl("(ui/app.cpp|codeeditor.cpp|htmltablewin.cpp|infobarex.cpp|listctrlex.cpp|listctrlexdataprovider.cpp|searchpanel.cpp|warningmanager.cpp|screenshot.cpp|resource_manager.cpp|idhelpers.cpp|mainframe.cpp|warningmessagesdlg.cpp|variableselectdlg.cpp|startpage.cpp|htmltablepanel.cpp|listctrlitemviewdlg.cpp|listdlg.cpp|listctrlsortdlg.cpp|artmetro.cpp|filelistdlg.cpp|functionbrowserdlg.cpp|getdirdlg.cpp|gridexportdlg.cpp|archivedlg.cpp|edittextdlg.cpp|excelpreviewdlg.cpp|sidebar.cpp|sidebarbook.cpp|downloadfile.cpp|formattedtextctrl.cpp|demo.cpp|main.cpp|i18n-check/samples|i18n-check/tests/|cpp_i18n_review.cpp|i18n_review.cpp|utfcpp/tests/|utfcpp/samples/|utfcpp/extern|cxxopts)", files)]
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
           file=buildLibFile)


# Files for full application and testing library
################################################
files <- str_glue("src/{list.files(path=srcFolder, pattern='(*[.]cpp|cJSON[.]c)', recursive=TRUE)}")
# remove test and sample files from submodules
files <- files[!grepl("(formattedtextctrl.cpp|demo.cpp|main.cpp|i18n-check/samples|i18n-check/tests/|cpp_i18n_review.cpp|i18n_review.cpp|utfcpp/tests/|utfcpp/samples/|utfcpp/extern|cxxopts)", files)]
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
           file=buildFile)


# Library files for testing library
################################################
files <- stringr::str_replace(files, "src/", "../src/")
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# This should be used for the unit test runner.
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
           file=buildFileTestLib)


# Library files for GUI testing library
################################################
files <- stringr::str_replace(files, "src/", "../src/")
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# This should be used for the unit test runner.
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
           file=buildFileTestGuiLib)


# Catch2 test files
################################################
files <- str_glue("{list.files(path=testFolder, pattern='(*[.]cpp)', recursive=FALSE)}")
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# This should be used for the unit test runner.
# DO NOT MODIFY MANUALLY!

SET(TEST_SRC_FILES\n    {paste(files, collapse='\n    ')})"),
           file=buildFileTests)


# GUI Catch2 test files
################################################
files <- str_glue("{list.files(path=guiTestFolder, pattern='(*[.]cpp)', recursive=FALSE)}")
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# This should be used for the unit test runner.
# DO NOT MODIFY MANUALLY!

SET(TEST_SRC_FILES\n    {paste(files, collapse='\n    ')})"),
           file=buildFileTestsGui)