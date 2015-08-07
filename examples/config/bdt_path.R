bdtDatasetsDir = '/dcs01/gcode/fdu1/data/bdtDatasets'
bdtHome = '/dcs01/gcode/fdu1/install/gcc-4.4.7/bdt'

if (.Platform$OS.type == "windows") {
    bdtDatasetsDir = 'C:/work/bdtDatasets'
    bdtHome = 'C:/work/BDT/build/windows/install'
}