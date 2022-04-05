library(tidyverse)
library(magrittr)
library(stringr)

toolsFolder <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/../src')
buildFile <- str_glue('{dirname(rstudioapi::getSourceEditorContext()$path)}/files.cmake')

files <- str_glue("src/{list.files(path=toolsFolder, pattern='*.cpp', recursive=TRUE)}")
# remove easyexif's demo.cpp file
files <- files[grepl("[^(demo.cpp)]", files)]
write_file(str_glue("# Automatically generated from 'Build CMake Files List.R'
# DO NOT MODIFY MANUALLY!

SET(WISTERIA_SRC\n    {paste(files, collapse='\n    ')})"),
file=buildFile)