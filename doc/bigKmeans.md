# BigKmeans Manual

## Using BigKmeans
```shell
$ {bdt_home}/bigKmeans [options]
```
where ```{bdt_home}``` is BDT's installtion directory.

Options:
- ```--data-input```: the input data, see Data Input section for details
- ```--k```: the number of clusters
- ```--out```: the output directory. The directory will be created if it does not exist.
- ```--thread-num```: the number of threads to be used for running bigKmeans
- ```--dist-type```: the distance measure for calculation the distance between rows
- ```--max-iter```: the maximum number of iterations allowed. The default is 100.
- ```--min-expchg```: the minimum change of fraction of varaince explained by centroids in a 3 consecutive iteratino, once the change is smaller than the specified threshold, bigKmeans will terminate. The default is 0.0001.
- ```--start-from```: there can be 3 steps in running bigKmeans: ```1-data-mat```, ```2-seeds-mat``` (when --seed-input is used), ```3-run-kmeans```, by default the program will start from ```1-data-mat```, i.e., reading input data and representes it internally as a binary matrix. This option allows the bigKmeans to skip some of the steps. For example, when there is a need to run bigKmeans with different number of clusters (ks), the user can use ```--start-from 3-run-kmeans``` with a different ```--k```, so that skipping the process of loading the input data again.
- ```--seed-input```: the initial k centroids can be optionally specified with the same syntax as ```--data-input```. This can be used for example to reproduce the clustering with the initial seeds previously used.

## Examples
- [Linux/Mac OS X command line examples](https://github.com/fangdu64/BDT/tree/master/examples/linux/bigKMeans)
- [Windows command line examples](https://github.com/fangdu64/BDT/tree/master/examples/windows/bigKMeans)
- [R package examples](https://github.com/fangdu64/BDT/tree/master/examples/R/bigKMeans)

## Data Input
Internally, the input data for BigKmeans is represented as a big matrix in which rows are elements to be clustered, and columns are features (dimentions) that row distances are based on. We used the following syntax for users to specify the input data:
```shell
--data-input {type}@{location}
```
where ```{type}``` represent the data input type and can be one of the ```text-mat```, ```binary-mat```, ```bams```, and some output/internal matrices from upstream analysis preformed by other BDT tools such as BDVD. ```{location}``` specifies the physical location of the input source and can be a path of the input file, or the output directory of a previous analysis, depedinng on the specific ```{type}```. 

### text-mat
syntax:
```shell
--data-input text-mat@{location}
--data-nrow {nrow}
--data-ncol {ncol}
```
where ```{location}``` is the name of the data file (an absolute path) which the matrix data are to be read from. Each row of the matrix appears as one line of the file. The matrix size must be specified. The ```{nrow}``` specifies the number of rows to be read. The ```{ncol}``` specifies the number of columns.
By default, we assumes that values on each line of the file are separated by a white space, and there is no column header or row header in the file. The user can provide the following optional settings for customization.
```shell
--data-col-sep "{sep}"
--data-skip-cols {skip-cols}
--data-skip-rows {skip-rows}
--data-col-names {col-names}
```
where ```{sep}``` is the field separator character, ```{skip-rows}``` is the the number of lines of the data file to skip before beginning to read data,  ```{skip-cols}``` is the the number of columns of the data file to skip before beginning to read data. For example, to read a matrix stored in a CSV format that the first line is column headers and the first column is row headers, the settings would be:
```shell
--data-col-sep ","
--data-skip-cols 1
--data-skip-rows 1
```
Note that we need to put the separator in double quotes.

The user can also explicity provide the column names of the input matrix, so that the column names can be used in plotting.
```shell
--data-col-names {col-names}
```
where ```{col-names}``` is a comma separated string of column names. For example,
```shell
--data-col-names columnA,columnB,columnC
```
specifies that the column names are columnA, columnB, and columnC.

### binary-mat
syntax:
```shell
--data-input binary-mat@{location}
--data-nrow {nrow}
--data-ncol {ncol}
```
where ```{location}``` is the name of the data file (an absolute path) which the matrix data are to be read from. The matrix size is specified by ```{nrow}``` and ```{ncol}```.
