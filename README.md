# Introduction
Big Data Tools (BDT) is a computational framework for analyzing very large scale genomic data. Currently, it offers several unique tools.

## BDVD：Big Data Variance Decomposition for High-throughput Genomic Data
Variance decomposition (e.g., ANOVA, PCA) is a fundamental tool in statistics to
understand data structure. High-throughput genomic data have heterogeneous sources of variation. Some are of biological interest, and others are unwanted (e.g., lab and batch effects). Knowing the relative contribution of each source to the total data variance is crucial for making data-driven discoveries. However, when one has massive amounts of high-dimensional data with heterogeneous origins, analyzing variances is non-trivial. The dimension, size and heterogeneity of the data all pose significant challenges. Big Data Variance Decomposition (BDVD) is a new tool developed to solve this problem. Built upon the recently developed RUV approach, BDVD decomposes data into biological signals, unwanted systematic variation, and independent random noise. The biological signals can then be further decomposed to study variations among genomic loci or sample types, or correlation between different data types. The algorithm is implemented by incorporating techniques to handle big data and offers several unique features:
- Implemented with efficient C++ language
- Fully exploits multi-core/multi-cpu computation power
- Ability to handle very large scale data  (e.g., a 30,000,000 × 500 data matrix)
- Ability to directly take a large number of BAM files as input with multi-core parallel processing
- Provides command line tools
- Provides R package to run BDVD and for seamless integration
- Transparency/open-source code
- Easy installation - one liner command, no root user required

In addition, BDVD naturally outputs normalized biological variations for downstream statistical inferences such as clustering large scale genomic loci with BigClust that is also provided in BDT.

## BigClust: Big Data Clustering Methods
Cluster analysis is the task of assigning a set of elements into groups (clusters) on the basis of their similarity. BigClust offers several tools to quickly perform clustering for very large scale dataset. 

### BigKmeans
BigKmeans enhences the widely used K-means with important improvments making it very suitable for big data.
- Improved the seeding (choosing initial centroids) with kmeans++
- Ability to evaluate optimal K with no extra computational cost
- Implemented with efficient C++ language
- Fully exploits multi-core/multi-cpu computation power
- Ability to handle very large scale data  (e.g., a 30,000,000 × 500 data matrix)
- Built-in ability to exploit multi-machine resources with distriubted computing for super large dataset
- Provides command line tools
- Provides R package to run BigKmeans and for seamless integration
- Transparency/open-source code
- Easy installation - one liner command, no root user required

# Installation
## Platforms
BDT runs on the following platforms:
- Linux
- Mac OS X
- Windows

## Installation on Linux
1. Download the latest source code:  [v0.1.2.tar.gz](https://github.com/fangdu64/BDT/archive/v0.1.2.tar.gz)
2. Extract and go to the extracted directory:

        $ tar xfz v0.1.2.tar.gz
        $ cd BDT-v0.1.2
3. Build and install BDT:

        $ make bdt_home={install_path}
where {install_path} is an installation directory (has to be an absolute path). The directory will be created if it does not exist.

## Installation on Mac OS X
1. Ensure that the Xcode Command Line Tools is installed. Otherwise open the Terminal and type:

        $ xcode-select --install
A pop-up windows will appears asking you about install tools.
2. Download the latest source code:  [v0.1.2.tar.gz](https://github.com/fangdu64/BDT/archive/v0.1.2.tar.gz)
3. Extract and go to the extracted directory:

        $ tar xfz v0.1.2.tar.gz
        $ cd BDT-v0.1.2
4. Build and install BDT:

        $ make bdt_home={install_path}
where {install_path} is an installation directory (has to be an absolute path). The directory will be created if it does not exist.

## Installation on Windows
1. Ensure that the Python3.3.3 (64-bit) is installed. Otherwise download [Windows X86-64 MSI Installer (3.3.3)](https://www.python.org/ftp/python/3.3.3/python-3.3.3.amd64.msi) and install it.
2. Ensure that the [Visual C++ Redistributable Packages for Visual Studio 2013](https://www.microsoft.com/en-us/download/details.aspx?id=40784) is installed. Otherwise download [vcredist_x64.exe](https://www.microsoft.com/en-us/download/details.aspx?id=40784) and install it.
3. Download BDT executable zip [BDT-v0.1.2-win64.zip](https://github.com/fangdu64/BDT/releases/download/v0.1.2/BDT-v0.1.2-Win64.zip).
4. Extract it and all the required executables/scripts will be in the extracted directory.

## R package
The bdt R package is to run BDT within R for seamless integration. Under the hood, it simply calls BDT commond line tools and provides a convinient way to retrieve the output results from BDT into R for follow-up analysis.

1. Have BDT installed (see above sections)
2. Have R installed
2. Ensure that the devtools package is installed. Otherwise,

        install.packages('devtools')
3. Install bdt library:

        library(devtools)
        install_git('https://github.com/fangdu64/rpackages', subdir = 'bdt')
4. Use bdt:

        library(bdt)
Example usages can be found in [R examples](https://github.com/fangdu64/BDT/tree/master/examples/R) and [ analysis](https://github.com/fangdu64/BDT/tree/master/examples/analysis)

# Usage
- [BDVD Manual](https://github.com/fangdu64/BDT/blob/master/doc/bdvd.md)
- [BigKmeans Manual](https://github.com/fangdu64/BDT/blob/master/doc/bigKmeans.md)

# Authors
BDVD: [Fang Du](https://www.linkedin.com/pub/fang-du/73/424/786), [Ben Sherwood](http://www.biostat.jhsph.edu/~hji/index_files/people.htm), [Bing He](http://www.biostat.jhsph.edu/~hji/index_files/people.htm), [Hongkai Ji](http://www.biostat.jhsph.edu/~hji/)

BigClust: [Fang Du](https://www.linkedin.com/pub/fang-du/73/424/786), [Ben Sherwood](http://www.biostat.jhsph.edu/~hji/index_files/people.htm), [Hongkai Ji](http://www.biostat.jhsph.edu/~hji/)

