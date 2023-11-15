if (!require("pacman")) install.packages("pacman")
library(pacman)
pacman::p_load(tidyverse, magrittr, stringr, this.path)

srcFolder <- str_glue('{this.path::this.dir()}/../src')
testFolder <- str_glue('{this.path::this.dir()}/../tests')
guiTestFolder <- str_glue('{this.path::this.dir()}/../tests/gui-tests')

buildLibFile <- str_glue('{this.path::this.dir()}/libfiles.cmake')
buildFile <- str_glue('{this.path::this.dir()}/files.cmake')
buildFileTestGuiLib <- str_glue('{this.path::this.dir()}/guilibfiles_testing.cmake')
buildFileTests <- str_glue('{this.path::this.dir()}/testfiles.cmake')
buildFileTestsGui <- str_glue('{this.path::this.dir()}/guitestfiles.cmake')

# Files for library
################################################
files <- str_glue("src/{list.files(path=srcFolder, pattern='(*[.]cpp|cJSON[.]c)', recursive=TRUE)}")
# remove test, sample files from submodules, and other
# files not relevant to the core library
files <- files[!grepl("(rc_file_review|xml_format|rtf_extract|postscript_extract|cpp_extract|odt_odp_extract|doc_extract|docx_extract|ui/app.cpp|codeeditor.cpp|htmltablewin.cpp|infobarex.cpp|listctrlex.cpp|listctrlexdataprovider.cpp|searchpanel.cpp|warningmanager.cpp|screenshot.cpp|resource_manager.cpp|idhelpers.cpp|mainframe.cpp|warningmessagesdlg.cpp|variableselectdlg.cpp|startpage.cpp|htmltablepanel.cpp|htmltablewinprintout.cpp|listctrlitemviewdlg.cpp|listdlg.cpp|listctrlsortdlg.cpp|artmetro.cpp|filelistdlg.cpp|functionbrowserdlg.cpp|getdirdlg.cpp|gridexportdlg.cpp|archivedlg.cpp|edittextdlg.cpp|excelpreviewdlg.cpp|sidebar.cpp|sidebarbook.cpp|downloadfile.cpp|formattedtextctrl.cpp|gtktextview-helper.cpp|demo.cpp|main.cpp|i18n-check/samples|i18n-check/tests/|cpp_i18n_review.cpp|i18n_review.cpp|utfcpp/tests/|utfcpp/samples/|utfcpp/extern|cxxopts)", files)]
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
           file=buildLibFile)


# Files for full application and testing library
################################################
files <- str_glue("src/{list.files(path=srcFolder, pattern='(*[.]cpp|cJSON[.]c)', recursive=TRUE)}")
# remove test and sample files from submodules
files <- files[!grepl("(rc_file_review.cpp|gtktextview-helper.cpp|codeeditor.cpp|functionbrowserdlg.cpp|demo.cpp|main.cpp|i18n-check/samples|i18n-check/tests/|cpp_i18n_review.cpp|i18n_review.cpp|utfcpp/tests/|utfcpp/samples/|utfcpp/extern|cxxopts)", files)]
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
           file=buildFile)


# Library files for GUI testing library
################################################
files <- stringr::str_replace(files, "src/", "../../src/")
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