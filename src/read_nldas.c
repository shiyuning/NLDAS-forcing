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

#define MAXLOC      1024
#define MAXSTRING   1024
#define NVRBL	    8

enum vrbl {TMP, SPFH, PRES, UGRD, VGRD, DLWRF, APCP, DSWRF};

int Readable (char *cmdstr);
void NextLine (FILE *fid, char *cmdstr, int *lno);

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
    char            buffer[MAXSTRING];

    int             i, k;
    double          lon[MAXLOC], lat[MAXLOC];
    int             counter = 0;
    FILE           *loc_file;
    int             match;
    int             lno = 0;
    char            cmdstr[MAXSTRING];
    char            lon_char[MAXSTRING], lat_char[MAXSTRING];

    double          prcp;
    double          tmp;
    double          spfh;
    double          wind;
    double          dlwrf;
    double          dswrf;
    double          pres;
    double          avg_pres;
    int             pres_counter;
    double	    rh;
    double          e_s, w_s, w;
    double	    tx, tn;
    double	    daily_solar;
    double	    daily_prcp;
    double	    rhx, rhn;
    double	    daily_wind;

    float           temp;

    char	    filename[MAXSTRING];
    FILE	   *input_file;

    int             year, month, mday;
    int             jday;
    int		    day_counter;

    FILE           *output_file;
    FILE           *temp_file;

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
	    {"model",	required_argument, 0, 'c'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int             option_index = 0;

        c = getopt_long (argc, argv, "a:b:c:", long_options, &option_index);

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
		if (strcasecmp ("PIHM", optarg) == 0)
		    mode = PIHM;
		else if (strcasecmp ("CYCLES", optarg) == 0)
		    mode = CYCLES;
		else
		{
		    printf ("Error: Model %s is not recognised.\n", optarg);
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
     * Read location.txt
     */
    loc_file = fopen ("location.txt", "r");
    if (loc_file == NULL)
    {
        printf ("ERROR: Cannot find the required location.txt file.\n");
        exit (1);
    }

    NextLine (loc_file, cmdstr, &lno);
    match = sscanf (cmdstr, "%s %s", lat_char, lon_char);
    if (match != 2 || strcasecmp (lat_char, "LATITUDE") != 0 ||
        strcasecmp (lon_char, "LONGITUDE") != 0)
    {
        printf ("Error reading the header line of location.txt at"
            "Line %d.\n", lno);
        exit (1);
    }

    for (i = 0; i < MAXLOC; i++)
    {
        NextLine (loc_file, cmdstr, &lno);

        if (strcasecmp (cmdstr, "EOF") == 0)
        {
            break;
        }

        match = sscanf (cmdstr, "%lf %lf", &lat[i], &lon[i]);
        if (match != 2)
        {
            printf ("Error reading location.txt at Line %d.\n", lno);
            exit (1);
        }

        if (lon[i] > 180.0)
        {
            lon[i] -= 360.0;
        }

        if (lat[i] < LA1 || lat[i] > LA2)
        {
            printf ("ERROR: latitude out of range (25.0625N ~ 52.9375N)\n");
            exit (1);
        }

        if (lon[i] < LO1 || lon[i] > LO2)
        {
            printf ("ERROR: longitude out of range (124.9375W ~ 67.0625W)\n");
            exit (1);
        }

        counter++;
    }

    for (k = 0; k < counter; k++)
    {
        avg_pres = 0.0;
        pres_counter = 0;

        if (mode == PIHM)
        {
            printf ("Generate PIHM forcing ");
        }
        else if (mode == CYCLES)
        {
            printf ("Generate Cycles forcing ");
        }

        printf ("for %lf N, %lf W, ", lat[k], lon[k]);
        strftime (buffer, 11, "%m-%d-%Y", &timeinfo_start);
        printf ("from %s ", buffer);
        strftime (buffer, 11, "%m-%d-%Y", &timeinfo_end);
        printf ("to %s\n", buffer);

        /* Open output file */
        sprintf (filename, "met%.4lfNx%.4lfW.txt", lat[k], -lon[k]);
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
            fflush (output_file);
        }
        else if (mode == CYCLES)
        {
            temp_file = tmpfile ();
        }

        time_start = timegm (&timeinfo_start);
        time_end = timegm (&timeinfo_end);


        /* Find the nearest NLDAS grid */
        ind_i = rint ((lon[k] - LO1) / DI) + 1;
        ind_j = rint ((lat[k] - LA1) / DJ) + 1;

        ind = 1 + (ind_i - 1) + (ind_j - 1) * NI;

        /* Read NLDAS files */
        for (day_counter = time_start; day_counter <= time_end;
            day_counter += 24 * 60 * 60)
        {
            tx = -999.0;
            tn = 999.0;
            daily_solar = 0.0;
            daily_prcp = 0.0;
            rhx = -999.0;
            rhn = 999.0;
            daily_wind = 0.0;

            for (rawtime = day_counter; rawtime <= day_counter + 23 * 60 * 60;
                rawtime += 60 * 60)
            {
                timeinfo = gmtime (&rawtime);
                jday = timeinfo->tm_yday + 1;

                if (timeinfo->tm_hour == 0)
                {
                    printf ("%2.2d/%2.2d/%4.4d\n",
                        timeinfo->tm_mon + 1, timeinfo->tm_mday,
                        timeinfo->tm_year + 1900);
                }

                /* Open input file */
                sprintf (filename,
                    "Data/%4.4d/%3.3d/NLDAS_FORA0125_H.A%4.4d%2.2d%2.2d.%2.2d"
                    "00.002.grb.dat", timeinfo->tm_year + 1900, jday,
                    timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
                    timeinfo->tm_mday, timeinfo->tm_hour);
                input_file = fopen (filename, "rb");
                if (input_file == NULL)
                {
                    printf ("Error when opening %s.\n", filename);
                    exit (1);
                }

                for (i = 0; i < NVRBL; i++)
                {
                    /* Skip to locate the nearest NLDAS grid */
                    fseek (input_file,
                        (long int)((i * NLDAS_SIZE + ind - 1) * 4), SEEK_SET);
                    /* Read in forcing */
                    fread (&temp, 4L, 1, input_file);

                    value[i] = (double)temp;

                }
                fclose (input_file);

                tmp = value[TMP];
                /* Convert from total precipitation to precipitation rate */
                prcp = value[APCP] / 3600.;
                wind = sqrt (value[UGRD] * value[UGRD] +
                    value[VGRD] * value[VGRD]);
                dlwrf = value[DLWRF];
                dswrf = value[DSWRF];
                pres = value[PRES];
                spfh = value[SPFH];

                avg_pres += pres;
                pres_counter++;

                /* Calculate relative humidity from specific humidity */
                e_s = 611.2 * exp (17.67 * (tmp - 273.15) /
                    (tmp - 273.15 + 243.5));
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
                    strftime (buffer, 17, "%Y-%m-%d %H:%M", timeinfo);
                    fprintf (output_file, "%s\t", buffer);
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
                fprintf (temp_file, "%-7.4d\t", timeinfo->tm_year + 1900);
                fprintf (temp_file, "%-7d\t", jday);
                fprintf (temp_file, "%-7.4lf\t", daily_prcp);
                fprintf (temp_file, "%-7.2lf\t", tx - 273.15);
                fprintf (temp_file, "%-7.2lf\t", tn - 273.15);
                fprintf (temp_file, "%-7.4lf\t", daily_solar);
                fprintf (temp_file, "%-7.2lf\t", rhx);
                fprintf (temp_file, "%-7.2lf\t", rhn);
                fprintf (temp_file, "%-7.3lf\n", daily_wind / 24.0);
                fflush (temp_file);
            }
        }

        if (mode == PIHM)
        {
            fclose (output_file);
        }
        else
        {
            avg_pres /= (double)pres_counter;
            printf ("%lf, %lf\n", avg_pres, -8200.0 * log(avg_pres / 101325.0));

            fseek (temp_file, 0, SEEK_SET);

            fprintf (output_file, "%-20s\t%-lf\n", "LATITUDE", lat[k]);
            fprintf (output_file, "%-20s\t%-lf\n", "ALTITUDE", -8200.0 * log (avg_pres / 101325.0));
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

            cmdstr[0] = '\0';

            NextLine (temp_file, cmdstr, &lno);

            while (strcasecmp (cmdstr, "EOF") != 0)
            {
                fprintf (output_file, "%s", cmdstr);
                fflush (output_file);
                NextLine (temp_file, cmdstr, &lno);
            }

            fclose (output_file);
            fclose (temp_file);
        }

    }

    return (EXIT_SUCCESS);
}

int Readable (char *cmdstr)
{
    int             readable;
    int             i;
    char            ch;

    for (i = 0; i < strlen (cmdstr); i++)
    {
        if (cmdstr[i] == 32 || cmdstr[i] == '\t' || cmdstr[i] == ' ')
        {
            continue;
        }
        else
        {
            break;
        }
    }

    if (i >= strlen (cmdstr))
    {
        readable = 0;
    }
    else
    {
        ch = cmdstr[i];

        if (ch != '#' && ch != '\n' && ch != '\0' && ch != '\r')
        {
            readable = 1;
        }
        else
        {
            readable = 0;
        }
    }

    return (readable);
}

void NextLine (FILE *fid, char *cmdstr, int *lno)
{
    /*
     * Read a non-blank line into cmdstr
     */
    strcpy (cmdstr, "\0");

    while (!Readable (cmdstr))
    {
        if (fgets (cmdstr, MAXSTRING, fid) == NULL)
        {
            strcpy (cmdstr, "EOF");
            break;
        }
        else
        {
            (*lno)++;
        }
    }
}

