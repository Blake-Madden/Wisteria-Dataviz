library(rvest)
library(tidyverse)
library(magrittr)
library(stringi)
library(stringr)
library(janitor)

# Add any colors that you want to Colors.txt (the name should be camel-cased
# and be sure to add a hex-encoded value next to it [tab between the color name
# and hex value].)
#
# After running this, copy the contents of ColorCode.txt into colorbrewer.h
# and colorbrewer.cpp.
# Also, copy the content of ColorReportMap.txt into the constant map in
# ReportBuilder::ConvertColor() (reportbuilder.cpp)

dataFolder <- dirname(rstudioapi::getSourceEditorContext()$path)

# colors from various runs and manual additions
previousColorData <- read_delim(str_glue("{dataFolder}/Colors.txt"), 
                          delim = "\t", escape_double = FALSE, 
                          trim_ws = TRUE, lazy = FALSE) %>%
  mutate("Hex"=stringi::stri_trans_toupper(`Hex`))

# uncomment this to re-read content from colorhexa.
# Not recommended though, as the current list removed a few colors that seemed redundant.
#colorData <- html_table(read_html("https://www.colorhexa.com/color-names"))[[1]] %>%
# dplyr::select(c("Color name", "Hex")) %>%
#  mutate("Color name"=stringi::stri_trans_general(`Color name`, 'Latin-ASCII')) %>%
#  mutate("Color name"=stringi::stri_trans_totitle(`Color name`)) %>%
#  mutate("Color name"=stringr::str_remove_all(`Color name`, "([/]Web| |[-]|'s)")) %>%
#  mutate("Color name"=stringr::str_replace(`Color name`, "^Cg", "CG")) %>%
#  mutate("Color name"=stringr::str_replace(`Color name`, "^Ua", "UA")) %>%
#  mutate("Color name"=stringr::str_replace(`Color name`, "^Ucla", "UCLA")) %>%
#  mutate("Color name"=stringr::str_replace(`Color name`, "^Ufo", "UFO")) %>%
#  mutate("Color name"=stringr::str_replace(`Color name`, "^Up", "UP")) %>%
#  mutate("Color name"=stringr::str_replace(`Color name`, "^Usc", "USC")) %>%
#  mutate("Color name"=stringr::str_replace(`Color name`, "^Msu", "MSU")) %>%
#  mutate("Hex"=stringi::stri_trans_toupper(`Hex`))
# colorData %<>% bind_rows(previousColorData) %>%

colorData <- previousColorData %>%
  # remove duplicates
  dplyr::distinct(`Color name`, .keep_all=T)

colorData %<>%
  arrange(`Color name`) %>%
  mutate(FullColorName = str_glue('Colors::Color::{`Color name`}')) %>%
  mutate("Values"=str_glue('L"{`Hex`}", ')) %>%
  mutate("Enum"=str_glue("{`Color name`},{strrep(' ', (max(nchar(`Color name`))+1)-nchar(`Color name`))}///< \\htmlonly <div style='background-color:{`Hex`}; width:50px;'>&nbsp;</div> \\endhtmlonly"))

valuesStr = ""
rowsPos = 1
while (rowsPos <= nrow(colorData))
  {
  diff = if_else(rowsPos+10 > nrow(colorData), (nrow(colorData)-rowsPos)+1, 10)
  valuesStr = str_glue("{valuesStr}\r\n{strrep(' ',4)}{str_flatten(colorData$Values[rowsPos:((rowsPos+diff)-1)])}")
  rowsPos = rowsPos+10
  }
# chop off trailing comma
valuesStr %<>% str_remove(", $")

enumsStr = str_glue("{strrep(' ',8)}",
  str_flatten(colorData$Enum, "\r\n{strrep(' ',8)}"),
  "\r\n{strrep(' ',8)}COLOR_COUNT\r\n")

# update known color list for next time
write_tsv(colorData %>% dplyr::select(c("Color name", "Hex")), str_glue("{dataFolder}/Colors.txt"))

# this is where the content for the ColorBrewer class will go
write_file(str_glue("{enumsStr}\r\n{valuesStr}"), str_glue("{dataFolder}/ColorCode.txt"))

# the map file ReportBuilder
write_file(str_flatten(str_glue('{ L"[str_to_lower(colorData$`Color name`)]", [colorData$FullColorName] },\r\n',
                    .open='[', .close=']')),
                    str_glue("{dataFolder}/ColorReportMap.txt"))