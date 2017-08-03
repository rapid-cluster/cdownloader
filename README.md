# cdownloader
 Downloads, unpacks, and reads data from the Cluster CSA arhive

## Purpose ##
The program provides a way to download data from the CLUSTER CSA and convert them into a set of ASCII
or binary tables, applying optional filtering and averaging. The data can be fetched from CSA web-
servers or can be taken from a local cache (created by the program itself on-the-fly or accumulated
using manual downloads).

## Configuration ##
To download from CSA, one needs an account there. The account is supplied to the CSA web-server via 
an assigned cookie. For cdownloader this cookie has to be placed inside of `~/.csacookie` file.

## Synopsis ##

For a given time range, the program gets CSA data, optionally filters them, optionally averages over
a given time interval and outputs specified products into one or more ASCII or binary tables. Each
output file is described by its format (ASCII or binary) and a list of products. These definitions
are give to the program in the output definition files. Here are a couple of examples:
```
name=c4-coordinates
format=ASCII
products=sc_pos_xyz_gse__C4_CP_FGM_SPIN
```
```
name=plasma-sheet-coords
format=ASCII
products=cis_mode__C4_CP_CIS_MODES,density__C4_CP_CIS-CODIF_HS_H1_MOMENTS,T__C4_CP_CIS-CODIF_HS_H1_MOMENTS,density__C4_CP_CIS-CODIF_HS_O1_MOMENTS,T__C4_CP_CIS-CODIF_HS_O1_MOMENTS,B_mag__C4_CP_FGM_SPIN,sc_pos_xyz_gse__C4_CP_FGM_SPIN
```

Notable program options:

  `-v [ --verbosity-level ] arg (=info)`  Verbosity level: controls minimal severity of messages that
appear in the program log. Allowed values: trace, debug, info, warning, error, fatal. Default value
is `info`.

  `--log-file arg` Log file location.
  
  `--continue [=arg(=1)] (=0)` Continue downloading existing output files. When this option is give (either in form 
  `--continue=true` or `--continue`) output files will not be truncated but instead new data will be appended to them.
  
  `--cache-dir arg` Sets directory with pre-downloaded CDF files. File names should follow scheme
  `<DATASET_NAME>__<BEGIN_DATE>_<BEGIN_TIME>_<END_DATE>_<END_TIME>.cdf.` This is the same format as in archives, downloaded
  from the CSA via offline method. Example: `C4_CP_FGM_SPIN__20010107_001002_20170502_034413_V170704.cdf`
  
  `--download-missing [=arg(=1)] (=1)` Specifies whether missing data will be downloaded from CSA automatically.

### Time options ###

  `--start arg (=2000-08-10T00:00:00.000Z)` Defines beginning of the output time range.
                                  
  `--end arg (=2017-08-01T12:56:57.000Z)` Defines end of the time range. Default value is current UTC time.
  
  `--valid-time-ranges arg` Allows to process only specified data ranges, by providing a list of them via named file.
  
  `--cell-size arg` Enables data averaging over the given time interval.
  
  `--no-averaging [=arg(=1)] (=0)` Disable data averaging. One of the options (`--cell-size` or `--no-averaging`) is required.

### Options to control output files ###
  `--output-dir arg (="/tmp")` Specifies directory for output files. Should exist.
  
  `--work-dir arg (="/tmp")`  Specifies working directory, where downloaded archives will be unpacked, for example. Should exist.
  
  `--output arg`  Specifies named file with output definition (see above). May be used several times.


For a full list of options, launch program with `--help` parameter.


Additional tool `csa-async-donwload-url`, can be usefull to fill data cache. For a given data set name, it outputs
a link for initiating offline downloading for that data set. Example:
```bash
$ csa-async-donwload-url C1_CP_CIS-HIA_ONBOARD_MOMENTS
https://csa.esac.esa.int/csa/aio/async-product-action?DATASET_ID=C1_CP_CIS-HIA_ONBOARD_MOMENTS&START_DATE=2001-01-01T00:00:00.000Z&END_DATE=2015-12-31T23:59:59.000Z&DELIVERY_FORMAT=CDF&DELIVERY_INTERVAL=All
```
The tool downloads metadata for the given datasets, detect maximal available time range and composes download URL.
