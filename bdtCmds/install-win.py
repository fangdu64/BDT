import sys, traceback
import getopt
import os
import shutil
import time
from datetime import datetime, date

#import iBSConfig

def install_copyDir(srcDir, destDir, ignore=None):
    if os.path.exists(destDir):
        shutil.rmtree(destDir)
    shutil.copytree(srcDir, destDir, False, ignore)

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

def install_python_cmdline(srcDir, srcFileName, destDir):
    if not os.path.exists(destDir):
        os.makedirs(destDir)
    infile = open("{0}/{1}".format(srcDir, srcFileName))
    outFN = "{0}/{1}".format(destDir, srcFileName.replace(".py",""))
    outfile = open(outFN, "w")
    replacements = {"Platform = None": 'Platform = "Windows"',
                    "__PYTHON_BIN_PATH__": "python3.3"}

    for line in infile:
        for src, target in replacements.items():
            line = line.replace(src, target)
        outfile.write(line)
    infile.close()
    outfile.close()

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
    install_copyDir(icePySrcDir, icePyDestDir)

    # install bdt bin
    lapackLibDir = os.path.abspath(bdtCmdsDir+"\\..\\build\\windows\\dependency\\Armadillo\\armadillo-4.650.4\\examples\\lib_win64")
    iceBinSrcDir = os.path.abspath(bdtCmdsDir+"\\..\\build\\windows\\dependency\\Ice\\Ice-3.5.1\\bin\\vc120\\x64")
    bdtBinSrcDir =  os.path.abspath(bdtCmdsDir+"\\..\\build\\windows\\bin\\x64\\Dlls\\Release")
    bdtBinDestDir =  bdtInstallDir+"\\bdt\\bin"
    install_bdtBin(iceBinSrcDir, lapackLibDir, bdtBinSrcDir, bdtBinDestDir)

    # install slice
    bdtSliceSrcDir = os.path.abspath(bdtCmdsDir+"\\..\\common\\slice")
    bdtSliceDestDir = bdtInstallDir+"\\bdt\\slice"
    install_copyDir(bdtSliceSrcDir, bdtSliceDestDir)

    # install config
    bdtConfigSrcDir = os.path.abspath(bdtCmdsDir+"\\..\\common\\config")
    bdtConfigDestDir = bdtInstallDir+"\\bdt\\config"
    install_copyDir(bdtConfigSrcDir, bdtConfigDestDir)

    # install bdtR
    bdtRSrcDir = os.path.abspath(bdtCmdsDir+"\\..\\bdtR\\outputTemplates")
    bdtRDestDir = bdtInstallDir+"\\bdt\\bdtR\\outputTemplates"
    install_copyDir(bdtRSrcDir, bdtRDestDir)

    bdtRSrcDir = os.path.abspath(bdtCmdsDir+"\\..\\bdtR\\Bioconductor")
    bdtRDestDir = bdtInstallDir+"\\bdt\\bdtR\\Bioconductor"
    install_copyDir(bdtRSrcDir, bdtRDestDir,shutil.ignore_patterns('.*'))


    # install bdtPy
    bdtPySrcDir = os.path.abspath(bdtCmdsDir+"\\..\\bdtPy")
    bdtPyDestDir = bdtInstallDir+"\\bdt\\bdtPy"
    install_copyDir(bdtPySrcDir, bdtPyDestDir)

    # install bdtCmds
    cmdFiles = next(os.walk(bdtCmdsDir))[2]
    shortCutCmds = [
        'bigKmeans.py',
        'bdvd.py',
        'bdvdExport.py',
        'bigMat.py',
        'bigKmeansC.py',
        'bigMatExport.py',
        'bdvdRowSelection.py']
    for cmdFile in cmdFiles:
        if cmdFile[-3:] != '.py':
            continue
        if cmdFile in ['install-linux.py', 'install-win.py']:
            continue
        destDir = bdtInstallDir+"\\bdt\\bdtCmds"
        if cmdFile in shortCutCmds:
            destDir = bdtInstallDir
        install_python_cmdline(bdtCmdsDir, cmdFile, destDir)

if __name__ == "__main__":
	sys.exit(main())
