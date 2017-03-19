#!/bin/sh

# Bash script to create meteorological forcing from NLDAS-2 forcing data
# Author: Yuning Shi (yshi.at.psu.edu)

IsLeapYear()
{
    year=$1

    if [ `expr $year % 400` -eq 0 ]; then
        return 1
    elif [ `expr $year % 100` -eq 0 ]; then
        return 0
    elif [ `expr $year % 4` -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

Eom()
{
    normal=(0 31 59 90 120 151 181 212 243 273 304 334 365)
    leap=(0 31 60 91 121 152 182 213 244 274 305 335 366)

    year=$1
    month=$2

    IsLeapYear $year
    isleap=$?

    if [ $isleap -eq 0 ] ; then
        return ${normal[$month]}
    else
        return ${leap[$month]}
    fi
}

##########################
# Main script starts here
##########################

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
