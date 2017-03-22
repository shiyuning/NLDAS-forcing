#!/bin/sh

# Bash script to create meteorological forcing from NLDAS-2 forcing data
# Author: Yuning Shi (yshi.at.psu.edu)

echo "##############################################"
echo "# NLDAS-forcing                              #"
echo "# https://github.com/shiyuning/NLDAS-forcing #"
echo "# Contact: Yuning Shi (yshi.at.psu.edu)      #"
echo "##############################################"

# Read configuration file
CONFIG_FILE=./forcing.config
chmod 700 $CONFIG_FILE
. $CONFIG_FILE

if [ $START_YEAR -lt 1979 ] ; then
    echo
    echo "Error: NLDAS-2 forcing data range from 01 Jan 1979 to present."
    echo "Please specify a valid START_YEAR in forcing.config and try again."
    echo
    exit
fi
if [ $START_MONTH -lt 1 -o $START_MONTH -gt 12 ] ; then
    echo
    echo "Error: Please specify a valid START_MONTH in forcing.config and try again."
    echo
    exit
fi
if [ $END_YEAR -lt $START_YEAR ] ; then
    echo
    echo "Error: END_YEAR is smaller than START_YEAR."
    echo "Please specify a valid END_YEAR in forcing.config and try again."
    echo
    exit
fi
if [ $END_MONTH -lt 1 -o $END_MONTH -gt 12 ] ; then
    echo
    echo "Error: Please specify a valid END_MONTH in forcing.config and try again."
    echo
    exit
fi
if [ $START_YEAR -eq $END_YEAR -a $END_MONTH -lt $START_MONTH ] ; then
    echo
    echo "Error: End date is earlier than start date."
    echo "Please specify a valid END_MONTH in forcing.config and try again."
    echo
    exit
fi

# Create a .netrc file in your home directory
touch ${HOME}/.netrc
echo "machine urs.earthdata.nasa.gov login $USER_NAME password $PASSWORD" > ${HOME}/.netrc
chmod 0600 ${HOME}/.netrc

# Create a cookie file
# This file will be used to persist sessions across calls to Wget or Curl
touch ${HOME}/.urs_cookies

# Run download script
echo
echo "Download starts."
. ./util/dl_extract_nldas.sh download
echo "Download completed."

# Run extract script
echo
echo "Interpretion starts."
if [ "$DECODE" == "yes" ] ; then
    . ./util/dl_extract_nldas.sh extract
fi
echo "Interpretion completed."

# Run read script
echo
echo "Generate forcing."
echo
if [ "$MODEL" != "no" ] ; then
    ./util/read_nldas --start $start_date --end $end_date --model $MODEL
fi
echo "Job completed."
