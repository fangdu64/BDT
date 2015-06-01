# BDVD Usage

## Data Input
Internally, the input data for BDVD is represented as a big matrix in which rows are elements to be clustered, and columns are features (dimentions) that row distances are based on. We used the following syntax for users to specify the input data:
```
--data-input {type}@{location}
```
where ```{type}``` represent the data input type and can be one of the ```text-mat```, ```binary-mat```, ```bams```, and some output/internal matrices from upstream analysis preformed by other BDT tools. ```{location}``` specifies the physical location of the input source and can be a path of the input file, or the output directory of a previous analysis, depedinng on the specific ```{type}```. 



