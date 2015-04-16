SET bdtInstallDir=..\..\..\..\build\windows\install
py %bdtInstallDir%\bigKmeans ^
	--data D:\BDT\examples\bigKMeans\txtData\data\dnase_test.txt ^
	--nrow 92554 ^
	--ncol 45 ^
	--k 100 ^
	--out D:\BDT\examples\bigKMeans\txtData\windows\out ^
	--thread-num 2 ^
	--dist-type Euclidean ^
	--thread-num 2 ^
	--max-iter 100 ^
	--min-expchg 0.0001