# NLDAS-forcing [![Build Status](https://travis-ci.org/shiyuning/NLDAS-forcing.svg?branch=master)](https://travis-ci.org/shiyuning/NLDAS-forcing)

NLDAS-forcing can **download** NLDAS-2 forcing data from NASA GES DISC archive, **decode** NLDAS-2 grib data into regular binary files using [Wgrib](ftp://ftp.cpc.ncep.noaa.gov/wd51we/wgrib/wgrib.c), and **generate** meteorological forcing for MM-PIHM and Cycles models.

## Usage

1. Download the code from the [release page](https://github.com/shiyuning/NLDAS-forcing/releases).

2. To compile, use

   ```shell
   make
   ```

   and executable will be created.

3. Next, edit the configuration file `forcing.config`.
   Specify desired `START_YEAR`, `START_MONTH`, `END_YEAR`, and `END_MONTH`.
   If you just want to download the NLDAS-2 forcing data but do not want to decode them into binary forms, set `DECODE` to `no`, otherwise, set to `yes`.
   If you don't want to generate forcing data for PIHM or Cycles, set `MODEL` to `no`, otherwise, choose from `PIHM` or `Cycles`.

4. Then edit the location file `location.txt` to add your desired locations.
   A maximum of 1024 locations can be added, by specifying the latitude and longitude of each location.
   You can provide an optional name for each site, which will be used to name generated forcing files.
   If a name is not provided, the generate forcing file will be named using the latitude and longitude.
   Please do not use white spaces in your site names.
   One forcing file will be generated for each added location.

5. To run the script, use

   ```shell
   $ ./nldas-forcing
   ```

   When multiple locations are specified, OpenMP is used to optimize efficiency.
   Be sure to request for multiple cores (`ppn`) and use `NUM_OMP_THREADS` parameter to specify the correct number of threads to accelerate the code.
   A sample `NLDAS.pbs` is provided for users who use PBS job systems.

**Note:**
The script will detect whether `.grib` and binary files already exist so it only downloads when necessary.
Downloaded `.grib` files and interpreted `.girb.dat` files will be stored in the `Data` directory, organized by year, and Julian day of year.

### ACKNOWLEDGMENT:
[Wgrib](http://www.cpc.ncep.noaa.gov/products/wesley/wgrib.html) is an operational NCEP program to manipulate, inventory and decode GRIB files.
The source file `wgrib.c` is downloaded from [ftp://ftp.cpc.ncep.noaa.gov/wd51we/wgrib/wgrib.c](ftp://ftp.cpc.ncep.noaa.gov/wd51we/wgrib/wgrib.c).
