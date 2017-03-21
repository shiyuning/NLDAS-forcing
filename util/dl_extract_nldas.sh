#!/bin/sh

# Function to convert from Julian day to date
Jul()
{
    date -d "$1-01-01 +$2 days -1 day" "+%Y-%m-%d";
}

operation=$1

# File extension to download/convert
if [ "$operation" == "download" ]; then
    ext=".grb"
elif [ "$operation" == "extract" ] ; then
    ext=".grb.dat"
else
    echo "ERROR!"
fi

# Loop through the years to download/convert data
start_date="$START_YEAR-$(printf "%2.2d" "$START_MONTH")-01"
end_date=$(date -d "$END_YEAR-$(printf "%2.2d" "$END_MONTH")-01 +1 month -1 day" "+%Y-%m-%d")
nod=$(( (`date -d $end_date +%s` - `date -d $start_date +%s`) / (24*3600) ))

for (( d=0; d<=$nod; d++ ))
do
    cyear=$(date -d "$start_date +$d days" "+%Y")
    cjday=$(date -d "$start_date +$d days" "+%j")

    nof=$(ls Data/$cyear/$cjday/NLDAS_FORA0125_H.A$cyear*.002$ext 2>/dev/null | wc -l)
    if [ $cyear -eq 1979 -a $cjday -eq 001 ] ; then
        nof_avail=11
    else
        nof_avail=24
    fi

    if [ $nof -ne $nof_avail ] ; then
        if [ "$operation" == "download" ] ; then
            echo "Downloading $(Jul $cyear $cjday) data..."
            wget --load-cookies $HOME/.urs_cookies --save-cookies $HOME/.urs_cookies --keep-session-cookies -r -c -nH -nd -np -A grb "https://hydro1.gesdisc.eosdis.nasa.gov/data/NLDAS/NLDAS_FORA0125_H.002/$cyear/$cjday/" -P Data/$cyear/$cjday &>/dev/null
        else
            echo "Interpreting $(Jul $cyear $cjday) data..."
            files=$(ls Data/$cyear/$cjday/NLDAS_FORA0125_H.A*.002*.grb | sort -d)
            for x in $files
              do
                ./util/wgrib -v $x | egrep "(:APCP:|:SPFH:|:TMP:|:UGRD:|:VGRD:|:DLWRF:|:DSWRF:|:PRES:sfc:)" | ./util/wgrib -i -nh $x -o "$x.dat" &>/dev/null 
            done
        fi
    fi

done
#for (( year=$START_YEAR; year<=$END_YEAR; year++ ))
#do
#    if [ $year -eq $cyear ] ; then
#        nod=`expr $(date +%j) - 1`
#        nod=${nod#"${nod%%[!0]*}"}
#    else
#        nod=$(date -d "$year-12-31" +%j)
#    fi
#
#    nof=$(ls Data/$year/*/NLDAS_FORA0125_H.A$year*.002$ext 2>/dev/null | wc -l)
#
#    if [ $nof -ne `expr $nod \* 24` ] ; then
#        for (( jday=1; jday<$nod; jday++ ))
#        do
#            nof=$(ls Data/$year/$(printf "%3.3d" "$jday")/NLDAS_FORA0125_H.A$year*.002$ext 2>/dev/null | wc -l)
#            if [ $nof -ne 24 ] ; then
#                echo "Download data from $(Jul $year $jday)..."
#                wget --load-cookies $HOME/.urs_cookies --save-cookies $HOME/.urs_cookies --keep-session-cookies -r -c -nH -nd -np -A grb "https://hydro1.gesdisc.eosdis.nasa.gov/data/NLDAS/NLDAS_FORA0125_H.002/$year/$(printf "%3.3d" "$jday")/" -P Data/$year/$(printf "%3.3d" "$jday") &>/dev/null
#            fi
#        done
#    fi
#done


