import sys, traceback
import getopt
import os
import shutil
import time
from datetime import datetime, date

#import iBSConfig

def install_icepy(icePySrcDir, icePyDestDir):
    if os.path.exists(icePyDestDir):
        shutil.rmtree(icePyDestDir)
    shutil.copytree(icePySrcDir, icePyDestDir)

def install_bdtBin(iceBinSrcDir, lapackLibDir, bdtBinSrcDir, bdtBinDestDir):
    if not os.path.exists(bdtBinDestDir):
        os.makedirs(bdtBinDestDir)
    files = ['bzip2.dll', 'freeze35.dll', 'ice35.dll', 'iceutil35.dll', 'libdb53.dll']
    for f in files:
        shutil.copy2("{0}\\{1}".format(iceBinSrcDir, f), "{0}\\{1}".format(bdtBinDestDir, f))
    
    files = ['blas_win64_MT.dll', 'lapack_win64_MT.dll']
    for f in files:
        shutil.copy2("{0}\\{1}".format(lapackLibDir, f), "{0}\\{1}".format(bdtBinDestDir, f))
    
    files = ['bigMat.exe', 'bigKmeansContractor.exe', 'bigKmeansServer.exe', 'bdvd.exe']
    for f in files:
        shutil.copy2("{0}\\{1}".format(bdtBinSrcDir, f), "{0}\\{1}".format(bdtBinDestDir, f))

def main(argv=None):
    bdtCmdsDir = os.path.dirname(os.path.abspath(__file__))
    print("bdtCmdsDir: {0}".format(bdtCmdsDir))
    bdtInstallDir = os.path.abspath(bdtCmdsDir+"\\..\\build\\windows\\install")
    print("bdtInstallDir: {0}".format(bdtInstallDir))

    if not os.path.exists(bdtInstallDir):
        os.mkdir(bdtInstallDir)

    # create dependency sub dir
    dependencyDir = bdtInstallDir+"\\dependency"
    if not os.path.exists(dependencyDir):
        os.mkdir(dependencyDir)
    
    # install IcePy
    icePySrcDir = os.path.abspath(bdtCmdsDir+"\\..\\build\\windows\\dependency\\Ice\\Ice-3.5.1\\python\\x64")
    icePyDestDir = dependencyDir+"\\IcePy"
    install_icepy(icePySrcDir, icePyDestDir)

    # install bdt bin
    lapackLibDir = os.path.abspath(bdtCmdsDir+"\\..\\build\\windows\\dependency\\Armadillo\\armadillo-4.650.4\\examples\\lib_win64")
    iceBinSrcDir = os.path.abspath(bdtCmdsDir+"\\..\\build\\windows\\dependency\\Ice\\Ice-3.5.1\\bin\\vc120\\x64")
    bdtBinSrcDir =  os.path.abspath(bdtCmdsDir+"\\..\\build\\windows\\bin\\x64\\Dlls\\Release")
    bdtBinDestDir =  bdtInstallDir+"\\bdt\\bin"
    install_bdtBin(iceBinSrcDir, lapackLibDir, bdtBinSrcDir, bdtBinDestDir)

if __name__ == "__main__":
	sys.exit(main())
