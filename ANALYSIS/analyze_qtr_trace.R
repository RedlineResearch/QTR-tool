#=====================================================================
# Analyze the trace output from QTR Tool
# author: Raoul Veroy
# 2020 June
#=====================================================================
# library(ggplot2)
# library(lattice)
# library(reshape2)

cargs <- commandArgs(TRUE)
datafile <- cargs[1]
targetdir <- cargs[2]

xcsv <- read.table( datafile, sep = "", header = FALSE, fill = TRUE ) # "" means any length whitespace character as delimiter

print("======================================================================")
print("    DONE.")
flush.console()
