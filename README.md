# NLDAS-forcing [![Build Status](https://travis-ci.org/shiyuning/NLDAS-forcing.svg?branch=travis)](https://travis-ci.org/shiyuning/NLDAS-forcing)

NLDAS-forcing can **download** NLDAS-2 forcing data from NASA GESDISC archive, **interpret** NLDAS-2 grib data into regular binary files using [Wgrib](ftp://ftp.cpc.ncep.noaa.gov/wd51we/wgrib/wgrib.c), and **generate** meteorological forcing for MM-PIHM and Cycles models.

## NASA Earthdata login for data access
NASA requires registration to access the GES DISC data.
Please follow [this link](https://wiki.earthdata.nasa.gov/display/EL/How+To+Register+With+Earthdata+Login) to register a new user in Earthdata login, and follow [this link](https://disc.gsfc.nasa.gov/registration/authorizing-gesdisc-data-access-in-earthdata_login) to authorize NASA GES DISC data archive in Earthdata login.

## Usage
1. Download the code from the [release page](https://github.com/shiyuning/NLDAS-forcing/releases).

2. To compile, use

   ```shell
   $ make
   ```

   and executables will be crated.

3. Next, edit the configuration file `forcing.config`.
   Specify desired `START_YEAR`, `START_MONTH`, `END_YEAR`, and `END_MONTH`.
   Change `USERNAME` and `PASSWORD` to your Earthdata username and password.
   If you just want to download the NLDAS-2 forcing data but do not want to interpret them into binary forms, set `EXTRACT` to `no`, otherwise, set to `yes`.
   If you don't want to generate forcing data for PIHM or Cycles, set `MODEL` to `no`, otherwise, choose from `PIHM` or `Cycles`.

4. Then edit the location file `location.txt` to add your desired locations.
   A maximum of 1024 locations can be added, by specifying the latitude and longitude of each location.
   One forcing file will be generated for each added location.

5. To run the script, use

   ```shell
   $ ./nldas-forcing
   ```

**Note:** the script will detect whether `.grib` and binary files already exist so it only downloads when necessary.
Downloaded `.grib` files and interpreted `.girb.dat` files will be stored in the `Data` directory, organized by year, and Julian day of year.

### ACKNOWLEDGMENT:
[Wgrib](http://www.cpc.ncep.noaa.gov/products/wesley/wgrib.html) is an operational NCEP program to manipulate, inventory and decode GRIB files.
The source file `wgrib.c` is downloaded from [ftp://ftp.cpc.ncep.noaa.gov/wd51we/wgrib/wgrib.c](ftp://ftp.cpc.ncep.noaa.gov/wd51we/wgrib/wgrib.c).
