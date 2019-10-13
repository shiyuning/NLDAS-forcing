#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <time.h>
#include <string.h>
#include <math.h>

/* NLDAS-2 grid information */
#define NLDAS_SIZE  103936
#define LA1         25.0625
#define LO1         -124.9375
#define LA2         52.9375
#define LO2         -67.0625
#define DI          0.125
#define DJ          0.125
#define NI          464
#define NJ          224

#define MAXLOC      1024
#define MAXSTRING   1024
#define NVRBL       8

#define PIHM        0
#define CYCLES      1

enum            vrbl {TMP, SPFH, PRES, UGRD, VGRD, DLWRF, APCP, DSWRF};

void            ParseCmdLineOpt(int, char *[], time_t *, time_t *, int *);
void            ReadLoc(int [], int [], char [MAXLOC][MAXSTRING], int *);
int             Readable(const char *);
void            NextLine(FILE *, char *, int *);

int main(int argc, char *argv[])
{
    time_t          time_start, time_end;
    int             nloc = 0;
    char            siten[MAXLOC][MAXSTRING];
    int             model;
    int             ind_i[MAXLOC], ind_j[MAXLOC];
    int             ind[MAXLOC];
    double          avg_pres[MAXLOC];
    int             pres_counter[MAXLOC];
    double          tx[MAXLOC], tn[MAXLOC];
    double          daily_solar[MAXLOC];
    double          daily_prcp[MAXLOC];
    double          rhx[MAXLOC], rhn[MAXLOC];
    double          daily_wind[MAXLOC];
    FILE           *input_file;
    FILE           *output_file[MAXLOC];
    FILE           *temp_file[MAXLOC];
    float           buffer[NLDAS_SIZE * NVRBL];
    int             day_counter;
    int             kloc;

    /*
     * Get command line options
     */
    ParseCmdLineOpt(argc, argv, &time_start, &time_end, &model);

    /*
     * Read location.txt
     */
    ReadLoc(ind_i, ind_j, siten, &nloc);

    /* Initialize Files and Grid Locations */
#if defined(_OPENMP)
# pragma omp parallel for
#endif
    for (kloc = 0; kloc < nloc; kloc++)
    {
        char            output_fn[MAXSTRING];

        /* Find the nearest NLDAS grid */
        ind[kloc] = 1 + (ind_i[kloc] - 1) + (ind_j[kloc] - 1) * NI;

        /* Open output file */
        sprintf(output_fn, (PIHM == model) ? "%s.meteo" : "%s.weather",
            siten[kloc]);
        output_file[kloc] = fopen(output_fn, "w");

        /* Print NLDAS-2 grid */
        fprintf(output_file[kloc], "# NLDAS-2 grid: %.4lfNx%.4lfW\n",
            LA1 + (ind_j[kloc] - 1) * DJ, -(LO1 + (ind_i[kloc] - 1) * DI));

        if (PIHM == model)
        {
            /* Print header */
            fprintf(output_file[kloc], "%-16s\t", "TIME");
            fprintf(output_file[kloc], "%-11s\t", "PRCP");
            fprintf(output_file[kloc], "%-6s\t", "SFCTMP");
            fprintf(output_file[kloc], "%-5s\t", "RH");
            fprintf(output_file[kloc], "%-6s\t", "SFCSPD");
            fprintf(output_file[kloc], "%-6s\t", "SOLAR");
            fprintf(output_file[kloc], "%-6s\t", "LONGWV");
            fprintf(output_file[kloc], "%-9s\n", "PRES");
            fprintf(output_file[kloc], "%-16s\t", "TS");
            fprintf(output_file[kloc], "%-11s\t", "kg/m2/s");
            fprintf(output_file[kloc], "%-6s\t", "K");
            fprintf(output_file[kloc], "%-5s\t", "%");
            fprintf(output_file[kloc], "%-6s\t", "m/s");
            fprintf(output_file[kloc], "%-6s\t", "W/m2");
            fprintf(output_file[kloc], "%-6s\t", "W/m2");
            fprintf(output_file[kloc], "%-9s\n", "Pa");
            fflush(output_file[kloc]);
        }
        else if (CYCLES == model)
        {
            temp_file[kloc] = tmpfile();
            avg_pres[kloc] = 0.0;
            pres_counter[kloc] = 0;
        }
    }

    for (day_counter = time_start; day_counter <= time_end;
        day_counter += 24 * 60 * 60)
    {
        time_t          rawtime;
        struct tm      *timeinfo;
        int             jday;

        for (rawtime = day_counter; rawtime <= day_counter + 23 * 60 * 60;
            rawtime += 60 * 60)
        {
            char            nldas_fn[MAXSTRING];

            timeinfo = gmtime(&rawtime);
            jday = timeinfo->tm_yday + 1;

            if (0 == timeinfo->tm_hour)
            {
                printf("%2.2d/%2.2d/%4.4d\n",
                    timeinfo->tm_mon + 1, timeinfo->tm_mday,
                    timeinfo->tm_year + 1900);
            }

            /* Open input file */
            sprintf(nldas_fn,
                "Data/%4.4d/%3.3d/NLDAS_FORA0125_H.A%4.4d%2.2d%2.2d.%2.2d"
                "00.002.grb.dat", timeinfo->tm_year + 1900, jday,
                timeinfo->tm_year + 1900, timeinfo->tm_mon + 1,
                timeinfo->tm_mday, timeinfo->tm_hour);
            input_file = fopen(nldas_fn, "rb");
            if (NULL == input_file)
            {
                printf("Error when opening %s.\n", nldas_fn);
                break;
            }

            fread(buffer, 4L, NLDAS_SIZE * NVRBL, input_file);
            fclose(input_file);

#if defined(_OPENMP)
# pragma omp parallel for
#endif
            for (kloc = 0; kloc < nloc; kloc++)
            {
                double          prcp;
                double          tmp;
                double          spfh;
                double          wind;
                double          dlwrf;
                double          dswrf;
                double          pres;
                double          rh;
                double          e_s, w_s, w;
                float           temp;
                int             kvar;
                double          value[NVRBL];

                /* Initialize daily variables */
                if (0 == timeinfo->tm_hour)
                {
                    tx[kloc] = -999.0;
                    tn[kloc] = 999.0;
                    daily_solar[kloc] = 0.0;
                    daily_prcp[kloc] = 0.0;
                    rhx[kloc] = -999.0;
                    rhn[kloc] = 999.0;
                    daily_wind[kloc] = 0.0;
                }

                for (kvar = 0; kvar < NVRBL; kvar++)
                {
                    value[kvar] =
                        (double)buffer[kvar * NLDAS_SIZE + ind[kloc] - 1];
                }

                tmp = value[TMP];
                /* Convert from total precipitation to precipitation rate */
                prcp = value[APCP] / 3600.0;
                wind =
                    sqrt(value[UGRD] * value[UGRD] + value[VGRD] * value[VGRD]);
                dlwrf = value[DLWRF];
                dswrf = value[DSWRF];
                pres = value[PRES];
                spfh = value[SPFH];

                avg_pres[kloc] += pres;
                pres_counter[kloc]++;

                /* Calculate relative humidity from specific humidity */
                e_s = 611.2 * exp(17.67 * (tmp - 273.15) /
                    (tmp - 273.15 + 243.5));
                w_s = 0.622 * e_s / (pres - e_s);
                w = spfh / (1.0 - spfh);
                rh = w / w_s * 100.0;
                rh = (rh > 100.0) ? 100.0 : rh;

                daily_prcp[kloc] += prcp * 3600.0;

                tx[kloc] = (tmp > tx[kloc]) ? tmp : tx[kloc];
                tn[kloc] = (tmp < tn[kloc]) ? tmp : tn[kloc];
                daily_solar[kloc] += dswrf * 3600.0 / 1.0e6;
                rhx[kloc] = (rh > rhx[kloc]) ? rh : rhx[kloc];
                rhn[kloc] = (rh < rhn[kloc]) ? rh : rhn[kloc];
                daily_wind[kloc] += wind;

                if (PIHM == model)
                {
                    char            buffer[MAXSTRING];

                    strftime(buffer, 17, "%Y-%m-%d %H:%M", timeinfo);
                    fprintf(output_file[kloc], "%s\t", buffer);
                    fprintf(output_file[kloc], "%-11.8lf\t", prcp);
                    fprintf(output_file[kloc], "%-6.2lf\t", tmp);
                    fprintf(output_file[kloc], "%-5.2lf\t", w / w_s * 100.0);
                    fprintf(output_file[kloc], "%-5.2lf\t", wind);
                    fprintf(output_file[kloc], "%-6.2lf\t", dswrf);
                    fprintf(output_file[kloc], "%-6.2lf\t", dlwrf);
                    fprintf(output_file[kloc], "%-9.2lf\n", pres);
                    fflush(output_file[kloc]);
                }
            }
        }

        if (CYCLES == model)
        {
#if defined(_OPENMP)
# pragma omp parallel for
#endif
            for (kloc = 0; kloc < nloc; kloc++)
            {
                fprintf(temp_file[kloc], "%-7.4d\t", timeinfo->tm_year + 1900);
                fprintf(temp_file[kloc], "%-7d\t", jday);
                fprintf(temp_file[kloc], "%-7.4lf\t", daily_prcp[kloc]);
                fprintf(temp_file[kloc], "%-7.2lf\t", tx[kloc] - 273.15);
                fprintf(temp_file[kloc], "%-7.2lf\t", tn[kloc] - 273.15);
                fprintf(temp_file[kloc], "%-7.4lf\t", daily_solar[kloc]);
                fprintf(temp_file[kloc], "%-7.2lf\t", rhx[kloc]);
                fprintf(temp_file[kloc], "%-7.2lf\t", rhn[kloc]);
                fprintf(temp_file[kloc], "%-7.3lf\n", daily_wind[kloc] / 24.0);
                fflush(temp_file[kloc]);
            }
        }
    }

#if defined(_OPENMP)
# pragma omp parallel for
#endif
    for (kloc = 0; kloc < nloc; kloc++)
    {
        int             lno = 0;
        char            cmdstr[MAXSTRING];

        if (CYCLES == model)
        {
            avg_pres[kloc] /= (double)pres_counter[kloc];

            fprintf(output_file[kloc], "%-20s\t%-lf\n", "LATITUDE",
                LA1 + (ind_j[kloc] - 1) * DJ);
            fprintf(output_file[kloc], "%-20s\t%-.2lf\n", "ALTITUDE",
                -8200.0 * log(avg_pres[kloc] / 101325.0));
            fprintf(output_file[kloc], "%-20s\t10.0\n", "SCREENING_HEIGHT");
            fprintf(output_file[kloc], "%-7s\t", "YEAR");
            fprintf(output_file[kloc], "%-7s\t", "DOY");
            fprintf(output_file[kloc], "%-7s\t", "PP");
            fprintf(output_file[kloc], "%-7s\t", "TX");
            fprintf(output_file[kloc], "%-7s\t", "TN");
            fprintf(output_file[kloc], "%-7s\t", "SOLAR");
            fprintf(output_file[kloc], "%-7s\t", "RHX");
            fprintf(output_file[kloc], "%-7s\t", "RHN");
            fprintf(output_file[kloc], "%-7s\n", "WIND");

            fseek(temp_file[kloc], 0, SEEK_SET);
            cmdstr[0] = '\0';
            NextLine(temp_file[kloc], cmdstr, &lno);

            while (strcasecmp(cmdstr, "EOF") != 0)
            {
                fprintf(output_file[kloc], "%s", cmdstr);
                fflush(output_file[kloc]);
                NextLine(temp_file[kloc], cmdstr, &lno);
            }

            fclose(temp_file[kloc]);
        }

        fclose(output_file[kloc]);
    }

    return EXIT_SUCCESS;
}

void ParseCmdLineOpt(int argc, char *argv[], time_t *time_start,
    time_t *time_end, int *model)
{
    int             c;
    struct tm       timeinfo_start = { 0, 0, 0, 0, 0, 0 };
    struct tm       timeinfo_end = { 0, 0, 0, 0, 0, 0 };
    int             year, month, mday;

    while (1)
    {
        static struct option long_options[] = {
            {"start", required_argument, 0, 's'},
            {"end",   required_argument, 0, 'e'},
            {"model", required_argument, 0, 'm'},
            {0, 0, 0, 0}
        };
        /* getopt_long stores the option index here. */
        int             option_index = 0;

        c = getopt_long(argc, argv, "a:b:c:d:", long_options, &option_index);

        /* Detect the end of the options. */
        if (-1 == c)
        {
            break;
        }

        switch (c)
        {
            case 's':
                sscanf(optarg, "%d-%d-%d", &year, &month, &mday);
                timeinfo_start.tm_year = year - 1900;
                timeinfo_start.tm_mon = month - 1;
                timeinfo_start.tm_mday = mday;
                *time_start = timegm(&timeinfo_start);
                break;
            case 'e':
                sscanf(optarg, "%d-%d-%d", &year, &month, &mday);
                timeinfo_end.tm_year = year - 1900;
                timeinfo_end.tm_mon = month - 1;
                timeinfo_end.tm_mday = mday;
                *time_end = timegm(&timeinfo_end);
                break;
            case 'm':
                if (strcasecmp("PIHM", optarg) == 0)
                {
                    *model = PIHM;
                }
                else if (strcasecmp("CYCLES", optarg) == 0)
                {
                    *model = CYCLES;
                }
                else
                {
                    printf("Error: Model %s is not recognized.\n", optarg);
                    exit(EXIT_FAILURE);
                }
                break;
            case '?':
                /* getopt_long already printed an error message. */
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    /* Print any remaining command line arguments (not options). */
    if (optind < argc)
    {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
        {
            printf("%s ", argv[optind++]);
        }
        putchar('\n');
    }
}

void ReadLoc(int ind_i[], int ind_j[], char siten[MAXLOC][MAXSTRING],
    int *nloc)
{
    FILE           *loc_file;
    int             match;
    int             lno = 0;
    char            cmdstr[MAXSTRING];
    char            lon_char[MAXSTRING], lat_char[MAXSTRING];
    char            siten_char[MAXSTRING];
    int             kloc;
    int             exist;
    double          lat, lon;

    /* Open location file */
    loc_file = fopen("location.txt", "r");
    if (NULL == loc_file)
    {
        printf("ERROR: Cannot find the required location.txt file.\n");
        exit(EXIT_FAILURE);
    }

    /* Read header line */
    NextLine(loc_file, cmdstr, &lno);
    match = sscanf(cmdstr, "%s %s %s", lat_char, lon_char, siten_char);
    if (match != 3 || strcasecmp(lat_char, "LATITUDE") != 0 ||
        strcasecmp(lon_char, "LONGITUDE") != 0 ||
        strcasecmp(siten_char, "NAME") != 0)
    {
        printf( "Error reading the header line of location.txt at Line %d.\n",
            lno);
        exit(EXIT_FAILURE);
    }

    NextLine(loc_file, cmdstr, &lno);
    while (strcasecmp(cmdstr, "EOF") != 0)
    {
        /* Read one line of lat, lon, and site name */
        match = sscanf(cmdstr, "%lf %lf %s", &lat, &lon, siten_char);
        if (match != 3 && match != 2)
        {
            printf("Error reading location.txt at Line %d.\n", lno);
            exit(EXIT_FAILURE);
        }

        /* Adjust longitude */
        lon -= (lon > 180.0) ? 360.0 : 0.0;

        /* Create a name when site name is missing from input */
        if (match == 2)
        {
            sprintf(siten_char, "met%.4lfNx%.4lfW", lat, -lon);
        }

        /* Check if specified lat/lon are within NLDAS-2 domain */
        if (lat < LA1 || lat > LA2)
        {
            printf("Error: latitude out of range (25.0625N ~ 52.9375N)\n");
            exit(EXIT_FAILURE);
        }

        if (lon < LO1 || lon > LO2)
        {
            printf("Error: longitude out of range (124.9375W ~ 67.0625W)\n");
            exit(EXIT_FAILURE);
        }

        /* Check if site are in the same NLDAS grid as other sites */
        exist = 0;
        if (*nloc > 0)
        {
            for (kloc = 0; kloc < *nloc; kloc++)
            {
                if (rint((lon - LO1) / DI) + 1 == ind_i[kloc] &&
                    rint((lat - LA1) / DJ) + 1 == ind_j[kloc])
                {
                    printf("Site %s and site %s are in the same NLDAS grid.\n"
                        "Forcing for Site %s will not be generated.\n",
                        siten_char, siten[kloc], siten_char);
                    exist = 1;
                    break;
                }
            }
        }

        if (!exist)
        {
            ind_i[*nloc] = rint((lon - LO1) / DI) + 1;
            ind_j[*nloc] = rint((lat - LA1) / DJ) + 1;
            strcpy(siten[*nloc], siten_char);
            (*nloc)++;
        }

        NextLine(loc_file, cmdstr, &lno);
    }
}

int Readable(const char *cmdstr)
{
    int             readable;
    int             i;
    char            ch;

    for (i = 0; i < strlen(cmdstr); i++)
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

    if (i >= strlen(cmdstr))
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

    return readable;
}

void NextLine(FILE *fid, char *cmdstr, int *lno)
{
    /*
     * Read a non-blank line into cmdstr
     */
    strcpy(cmdstr, "\0");

    while (!Readable(cmdstr))
    {
        if (fgets(cmdstr, MAXSTRING, fid) == NULL)
        {
            strcpy(cmdstr, "EOF");
            break;
        }
        else
        {
            (*lno)++;
        }
    }
}
