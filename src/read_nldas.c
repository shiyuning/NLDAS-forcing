#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <math.h>

/* NLDAS-2 grid information */
#define NLDAS_SIZE  103936
#define LA1	    25.0625
#define LO1	    -124.9375
#define LA2	    52.9375
#define LO2	    -67.0625
#define DI	    0.125
#define DJ	    0.125
#define NI	    464
#define NJ	    224

#define NVRBL	    8
enum vrbl {TMP, SPFH, PRES, UGRD, VGRD, DLWRF, APCP, DSWRF};

int IsLeapYear (int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int doy (int year, int month, int mday)
{
    static const int days[2][13] = {
        {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334},
        {0, 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335}
    };

    int             leap;

    leap = IsLeapYear (year);

    return days[leap][month] + mday;
}

int main (int argc, char *argv[])
{
    double	    value[NVRBL];

    int             ind_i, ind_j;
    int             ind;

    time_t          time_start, time_end;
    time_t          rawtime;
    struct tm       timeinfo_start = { 0, 0, 0, 0, 0, 0 };
    struct tm       timeinfo_end = { 0, 0, 0, 0, 0, 0 };
    struct tm      *timeinfo;

    int             i, j, k;
    double          lon = -999.0, lat = -999.0;

    double          prcp;
    double          tmp;
    double          spfh;
    double          wind;
    double          dlwrf;
    double          dswrf;
    double          pres;
    double	    rh;
    double          e_s, w_s, w;
    double	    tx, tn;
    double	    daily_solar;
    double	    daily_prcp;
    double	    rhx, rhn;
    double	    daily_wind;

    float           temp;

    char	    filename[1024];
    FILE	   *input_file;

    int             data_year = -999;
    int             year, month, mday;
    int             jday;
    int		    day_counter;
    int		    hour;

    FILE           *output_file;

    int             c;

    enum model	    { PIHM, CYCLES };
    enum model	    mode;
    /*
     * Get command line options
     */
    while (1)
    {
        static struct option long_options[] = {
            {"start",	required_argument, 0, 'a'},
            {"end",	required_argument, 0, 'b'},
            {"lat",	required_argument, 0, 'c'},
            {"lon",	required_argument, 0, 'd'},
            {"year",	required_argument, 0, 'e'},
	    {"model",	required_argument, 0, 'f'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int             option_index = 0;

        c = getopt_long (argc, argv, "a:b:c:d:e:f:", long_options, &option_index);

        /* Detect the end of the options. */
        if (c == -1)
            break;

        switch (c)
        {
            case 'a':
                sscanf (optarg, "%d-%d-%d", &year, &month, &mday);
                timeinfo_start.tm_year = year - 1900;
                timeinfo_start.tm_mon = month - 1;
                timeinfo_start.tm_mday = mday;
                break;
            case 'b':
                sscanf (optarg, "%d-%d-%d", &year, &month, &mday);
                timeinfo_end.tm_year = year - 1900;
                timeinfo_end.tm_mon = month - 1;
                timeinfo_end.tm_mday = mday;
                break;
            case 'c':
                sscanf (optarg, "%lf", &lat);
                break;
            case 'd':
                sscanf (optarg, "%lf", &lon);
                if (lon > 180)
                    lon = lon - 360.0;
                break;
            case 'e':
                sscanf (optarg, "%d", &data_year);
                break;
	    case 'f':
		if (strcasecmp ("PIHM", optarg) == 0)
		    mode = PIHM;
		else if (strcasecmp ("CYCLES", optarg) == 0)
		    mode = CYCLES;
		else
		{
		    printf ("Model %s is not recognised\n", optarg);
		    abort ();
		}
		break;
            case '?':
                /* getopt_long already printed an error message. */
                break;
            default:
                abort ();
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf ("non-option ARGV-elements: ");
        while (optind < argc)
            printf ("%s ", argv[optind++]);
        putchar ('\n');
    }

    /*
     * Check command line options
     */
    if ((timeinfo_start.tm_year == 0 || timeinfo_end.tm_year == 0) && data_year == -999)
    {
        printf ("ERROR: Please either specify desired start and end times using --start and --end, or specify desired year using --year.\n");
        exit (1);
    }
    else if ((timeinfo_start.tm_year == 0 || timeinfo_end.tm_year == 0) && data_year > -999)
    {
        timeinfo_start.tm_year = data_year - 1900;
        timeinfo_start.tm_mon = 0;
        timeinfo_start.tm_mday = 1;
        timeinfo_end.tm_year = data_year + 1 - 1900;
        timeinfo_end.tm_mon = 0;
        timeinfo_end.tm_mday = 1;
    }
    if (lat == -999 || lon == -999)
    {
        printf ("ERROR: Please specify desired location using --lat and --lon.\n");
        exit (1);
    }
    else if (lat < LA1 || lat > LA2)
    {
        printf ("ERROR: latitude out of range (25.0625N ~ 52.9375N)\n");
        exit (1);
    }
    else if (lon < LO1 || lon > LO2)
    {
        printf ("ERROR: longitude out of range (124.9375W ~ 67.0625W)\n");
        exit (1);
    }

    printf ("Read NLDAS data for %lf N, %lf W, from %2.2d/%2.2d/%4.4d to %2.2d/%2.2d/%4.4d\n", lat, lon, timeinfo_start.tm_mon + 1, timeinfo_start.tm_mday, timeinfo_start.tm_year + 1900, timeinfo_end.tm_mon + 1, timeinfo_end.tm_mday, timeinfo_end.tm_year + 1900);
    if (mode == PIHM)
	printf ("Generate forcing file for PIHM.\n\n");
    else if (mode == CYCLES)
	printf ("Generate weather file for Cycles.\n\n");

    sleep (2);

    /* Open output file */
    sprintf (filename, "Data/met%.4lfNx%.4lfW.txt", lat, -lon);
    output_file = fopen (filename, "w");
    if (mode == PIHM)
    {
	fprintf (output_file, "%-16s\t", "TIME");
	fprintf (output_file, "%-11s\t", "PRCP");
	fprintf (output_file, "%-6s\t", "SFCTMP");
	fprintf (output_file, "%-5s\t", "RH");
	fprintf (output_file, "%-6s\t", "SFCSPD");
	fprintf (output_file, "%-6s\t", "SOLAR");
	fprintf (output_file, "%-6s\t", "LONGWV");
	fprintf (output_file, "%-9s\n", "PRES");

	fprintf (output_file, "%-16s\t", "TS");
	fprintf (output_file, "%-11s\t", "kg/m2/s");
	fprintf (output_file, "%-6s\t", "K");
	fprintf (output_file, "%-5s\t", "%");
	fprintf (output_file, "%-6s\t", "m/s");
	fprintf (output_file, "%-6s\t", "W/m2");
	fprintf (output_file, "%-6s\t", "W/m2");
	fprintf (output_file, "%-9s\n", "Pa");
    }
    else if (mode == CYCLES)
    {
	fprintf (output_file, "%-20s\t%-lf\n", "LATITUDE", lat);
	fprintf (output_file, "%-20s\t%-lf\n", "ALTITUDE", 0.0);
	fprintf (output_file, "%-20s\t10.0\n", "SCREENING_HEIGHT");
	fprintf (output_file, "%-7s\t", "YEAR");
	fprintf (output_file, "%-7s\t", "DOY");
	fprintf (output_file, "%-7s\t", "PP");
	fprintf (output_file, "%-7s\t", "TX");
	fprintf (output_file, "%-7s\t", "TN");
	fprintf (output_file, "%-7s\t", "SOLAR");
	fprintf (output_file, "%-7s\t", "RHX");
	fprintf (output_file, "%-7s\t", "RHN");
	fprintf (output_file, "%-7s\n", "WIND");
    }
    fflush (output_file);

    time_start = timegm (&timeinfo_start);
    time_end = timegm (&timeinfo_end);


    /* Find the nearest NLDAS grid */
    ind_i = rint ((lon - LO1) / DI) + 1;
    ind_j = rint ((lat - LA1) / DJ) + 1;

    ind = 1 + (ind_i - 1) + (ind_j - 1) * NI;

    /* Read NLDAS files */
    for (day_counter = time_start; day_counter <= time_end; day_counter = day_counter + 24 * 60 * 60)
    {
	tx = -999.0;
	tn = 999.0;
	daily_solar = 0.0;
	daily_prcp = 0.0;
	rhx = -999.0;
	rhn = 999.0;
	daily_wind = 0.0;

	for (rawtime = day_counter; rawtime <= day_counter + 23 * 60 * 60; rawtime = rawtime + 60 * 60)
	{
	    timeinfo = gmtime (&rawtime);
	    jday = timeinfo->tm_yday + 1;

	    printf ("%2.2d/%2.2d/%4.4d %2.2d:00Z\n", timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_year + 1900, timeinfo->tm_hour);

	    /* Open input file */
	    sprintf (filename, "Data/%4.4d/%3.3d/NLDAS_FORA0125_H.A%4.4d%2.2d%2.2d.%2.2d00.002.grb.dat", timeinfo->tm_year + 1900, jday, timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour);
	    input_file = fopen (filename, "rb");
	    if (input_file == NULL)
	    {
		printf ("Error when opening %s.\n", filename);
		exit (1);
	    }

	    for (i = 0; i < NVRBL; i++)
	    {
		/* Skip to locate the nearest NLDAS grid */
		fseek (input_file, (long int)((i * NLDAS_SIZE + ind - 1) * 4), SEEK_SET);
		/* Read in forcing */
		fread (&temp, 4L, 1, input_file);

		value[i] = (double)temp;

	    }
	    fclose (input_file);

	    tmp = value[TMP];
	    /* Convert from total precipitation to precipitation rate */
	    prcp = value[APCP] / 3600.;
	    wind = sqrt (value[UGRD] * value[UGRD] + value[VGRD] * value[VGRD]);
	    dlwrf = value[DLWRF];
	    dswrf = value[DSWRF];
	    pres = value[PRES];
	    spfh = value[SPFH];

	    /* Calculate relative humidity from specific humidity */
	    e_s = 611.2 * exp (17.67 * (tmp - 273.15) / (tmp - 273.15 + 243.5));
	    w_s = 0.622 * e_s / (pres - e_s);
	    w = spfh / (1.0 - spfh);
	    rh = w / w_s * 100.0;
	    rh = rh > 100.0 ? 100.0 : rh;

	    daily_prcp += prcp * 3600.0;
	    if (tmp > tx)
		tx = tmp;
	    if (tmp < tn)
		tn = tmp;
	    daily_solar = daily_solar + dswrf * 3600.0 / 1.0e6;
	    if (rh > rhx)
		rhx = rh;
	    if (rh < rhn)
		rhn = rh;
	    daily_wind = daily_wind + wind;
    
	    if (mode == PIHM)
	    {
		fprintf (output_file, "%4.4d-%2.2d-%2.2d %2.2d:00\t", timeinfo->tm_year + 1900, timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour);
		fprintf (output_file, "%-11.8lf\t", prcp);
		fprintf (output_file, "%-6.2lf\t", tmp);
		fprintf (output_file, "%-5.2lf\t", w / w_s * 100.0);
		fprintf (output_file, "%-5.2lf\t", wind);
		fprintf (output_file, "%-6.2lf\t", dswrf);
		fprintf (output_file, "%-6.2lf\t", dlwrf);
		fprintf (output_file, "%-9.2lf\n", pres);
		fflush (output_file);
	    }
	}
	if (mode == CYCLES)
	{
	    fprintf (output_file, "%-7.4d\t", timeinfo->tm_year + 1900);
	    fprintf (output_file, "%-7d\t", jday);
	    fprintf (output_file, "%-7.4lf\t", daily_prcp);
	    fprintf (output_file, "%-7.2lf\t", tx - 273.15);
	    fprintf (output_file, "%-7.2lf\t", tn - 273.15);
	    fprintf (output_file, "%-7.4lf\t", daily_solar);
	    fprintf (output_file, "%-7.2lf\t", rhx);
	    fprintf (output_file, "%-7.2lf\t", rhn);
	    fprintf (output_file, "%-7.3lf\n", daily_wind / 24.0);
	    fflush (output_file);
	}
    }

    fclose (output_file);
    return 0;
}
