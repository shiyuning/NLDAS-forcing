#!/bin/sh

Jul()
{
    date -d "$1-01-01 +$2 days -1 day" "+%Y-%m-%d";
}

# File extension to download/convert
ext=".grb"

# For current year, will download/convert until yesterday
cyear=$(date +"%Y")

# Loop through the years to download/convert data
for (( year=$START_YEAR; year<=$END_YEAR; year++ ))
do
    if [ $year -eq $cyear ] ; then
        nod=`expr $(date +%j) - 1`
        nod=${nod#"${nod%%[!0]*}"}
    else
        nod=$(date -d "$year-12-31" +%j)
    fi

    nof=$(ls Data/$year/*/NLDAS_FORA0125_H.A$year*.002$ext 2>/dev/null | wc -l)

    if [ $nof -ne `expr $nod \* 24` ] ; then
        for (( jday=1; jday<$nod; jday++ ))
        do
            nof=$(ls Data/$year/$(printf "%3.3d" "$jday")/NLDAS_FORA0125_H.A$year*.002$ext 2>/dev/null | wc -l)
            if [ $nof -ne 24 ] ; then
                echo "Download data from $(Jul $year $jday)..."
                wget --load-cookies $HOME/.urs_cookies --save-cookies $HOME/.urs_cookies --keep-session-cookies -r -c -nH -nd -np -A grb "https://hydro1.gesdisc.eosdis.nasa.gov/data/NLDAS/NLDAS_FORA0125_H.002/$year/$(printf "%3.3d" "$jday")/" -P Data/$year/$(printf "%3.3d" "$jday") &>/dev/null
            fi
        done
    fi
done


