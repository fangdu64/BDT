
# ----------------------------------------------------------------------
# Sample Information
# ----------------------------------------------------------------------
sample_table_help_message = '''
comma separated fields
Format:
    sample_name, bam_dir, bam_file, biological_group, tags

Columns:
    sample_name         <string>    [ if left empty, set the same as bam_file ]

    bam_dir             <string>    [ absolute directory containing the bam_file (e.g., /home/user/) 
                                      if left empty, set the same as in the previous row            ]

    bam_file            <string>    [ bam file basename, i.e., stripped out dir (e.g., abc.bam)     ]
    biological_group    <string>    [                                         ]
    tags                <string>    [optional                                 ]
'''

#
# Replace sample rows between [rows begin] and [rows end] wiht actual contents
#
sample_table_content = '''
sample_name,bam_dir,bam_file,biological_group,tags
[rows begin]
cellARep1,/dir1/,cellARep1.bam,cellA
cellARep2,,cellARep2.bam,cellA
cellBRep1,/dir2/,cellARep1.bam,cellB
cellBRep2,,cellBRep2.bam,cellB
[rows end]
'''

sample_table_content = '''
sample_name,bam_dir,bam_file,biological_group,tags
[rows begin]
8988tAlnRep1,/dcs01/gcode/fdu1/iBS/Data/ENCODE/DNAse/DukeHG19/,wgEncodeOpenChromDnase8988tAlnRep1.bam,wgEncodeEH001103
8988tAlnRep2,,wgEncodeOpenChromDnase8988tAlnRep2.bam,wgEncodeEH001103
A549AlnRep1,,wgEncodeOpenChromDnaseA549AlnRep1.bam,wgEncodeEH001095
A549AlnRep2,,wgEncodeOpenChromDnaseA549AlnRep2.bam,wgEncodeEH001095

#Adultcd4th0AlnRep1,,wgEncodeOpenChromDnaseAdultcd4th0AlnRep1.bam,wgEncodeEH002562
#Adultcd4th0AlnRep2,,wgEncodeOpenChromDnaseAdultcd4th0AlnRep2.bam,wgEncodeEH002562
#Adultcd4th1AlnRep1,,wgEncodeOpenChromDnaseAdultcd4th1AlnRep1.bam,wgEncodeEH002563
#Adultcd4th1AlnRep2,,wgEncodeOpenChromDnaseAdultcd4th1AlnRep2.bam,wgEncodeEH002563
#AosmcSerumfreeAlnRep1,,wgEncodeOpenChromDnaseAosmcSerumfreeAlnRep1.bam,wgEncodeEH000601
#AosmcSerumfreeAlnRep2,,wgEncodeOpenChromDnaseAosmcSerumfreeAlnRep2.bam,wgEncodeEH000601

A549AlnRep1,/dcs01/gcode/fdu1/iBS/Data/ENCODE/DNAse/UWHG19/,wgEncodeUwDnaseA549AlnRep1.bam,wgEncodeEH001095,UW
A549AlnRep2,,wgEncodeUwDnaseA549AlnRep1.bam,wgEncodeEH001095,UW
[rows end]
'''



# ----------------------------------------------------------------------
# Parameters
# ----------------------------------------------------------------------
#chr1,chr2,chr3,chr4,chr5,chr6,chr7,chr8,chr9,chr10,chr11,chr12,chr13,chr14,chr15,chr16,chr17,chr18,chr19,chr20,chr21,chr22,chrX
chromosomes = ['chr1', 'chr2', 'chr3', 'chr4', 'chr5', 'chr6', 'chr7', 'chr8', 'chr9', 
               'chr10', 'chr11', 'chr12', 'chr13', 'chr14', 'chr15', 'chr16', 'chr17', 
               'chr18', 'chr19', 'chr20', 'chr21', 'chr22', 'chrX']

bin_width = 100


# ----------------------------------------------------------------------
# Don't change anything below this line!
# ----------------------------------------------------------------------
import os
import iBSDefines
#
# Get sample information
#

def define_samples():
    sample_list=[]
    #parse sample_table
    lines = sample_table_content.split('\n')
    dir=""
    for line in lines:
        line=line.strip()
        if line in ['',
                    'sample_name,bam_dir,bam_file,biological_group,tags',
                    '[rows begin]',
                    '[rows end]']:
            continue
        if line[0]=="#":
            continue

        cols=line.split(',')
        if len(cols)<4:
            print("ERROR in:",line)
            print("incomplete information")
            return None
        for i in range(len(cols)):
            cols[i]=cols[i].strip()

        sample_name=cols[0]
        bam_dir=cols[1]
        bam_file=cols[2]
        biological_group=cols[3]
        tags=None
        if len(cols)>4:
            tags=cols[4:]

        if bam_dir:
            dir=bam_dir
        fullname=dir+bam_file
        if not os.path.exists(fullname):
            print("ERROR in:",line)
            print("BAM file not exist")
            return None

        if not biological_group:
            print("ERROR in:",line)
            print("biological group not specified")
            return None

        sample_list.append(iBSDefines.BamSampleInfo(sample_name,fullname,biological_group,tags))

    return sample_list

def define_genomicBins():
    binInfo = iBSDefines.GenomicBinInfo(chromosomes, bin_width)
    return binInfo