#!/bin/sh

# Bash script to create meteorological forcing from NLDAS-2 forcing data
# Author: Yuning Shi (yshi.at.psu.edu)

# Read configuration file
CONFIG_FILE=./forcing.config
chmod 755 $CONFIG_FILE
. $CONFIG_FILE

