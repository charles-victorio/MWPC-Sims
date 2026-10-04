#!/bin/bash
#$ -N gasgen
#$ -cwd
#$ -t 1-40                      # 40 slices = 1 E-point each (= ne), max granularity
#$ -l h_rt=02:00:00,h_data=8G   # generous per-task; one point is minutes, not hours
#$ -j y
#$ -o garfield_code/logs/
#$ -tc 20                      # uncomment to cap concurrent tasks if your cluster requires
#$ -M charlesvictorio@ucla.edu
#$ -m ea

NTASKS=40

# load the job environment:
source /u/local/Modules/default/init/modules.sh
module load apptainer

apptainer exec ~/myimage.sif bash -c "
    cd /opt/root
    source bin/thisroot.sh
    source /opt/garfieldpp/share/Garfield/setupGarfield.sh
    cd ~/MWPC-Simulation/garfield_code/build
    ./gasgen $SGE_TASK_ID $NTASKS
"