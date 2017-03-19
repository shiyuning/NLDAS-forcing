#!/bin/sh

# Bash script to create meteorological forcing from NLDAS-2 forcing data
# Author: Yuning Shi (yshi.at.psu.edu)

# Read configuration file
CONFIG_FILE=./forcing.config
chmod 755 $CONFIG_FILE
. $CONFIG_FILE


# Create a .netrc file in your home directory
touch ${HOME}/.netrc
echo "machine urs.earthdata.nasa.gov login $USER_NAME password $PASSWORD" > ${HOME}/.netrc
chmod 0600 ${HOME}/.netrc

# Create a cookie file
# This file will be used to persist sessions across calls to Wget or Curl
touch ${HOME}/.urs_cookies
