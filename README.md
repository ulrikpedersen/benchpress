benchpress
==========

Compression test and benchmark tool

How to run
==========

This is a CLI application. The options and arguments are described here.

    benchpress [options] FILE DATASET
    
    Arguments:
      FILE:             The name of input HDF5 file.
      DATASET:          The name of the dataset inside FILE to use as input data. 
    
    Available options:
     -h [ --help ]                     Show online help
     -a [ --algorithm ] arg (=blosclz) Compression algorithm
     -t [ --threads ] arg (=1)         Number of threads to use
     -l [ --level ] arg (=0)           Compression level [0..9]
     --file arg                        Input file
     --dataset arg                     Input dataset
      

