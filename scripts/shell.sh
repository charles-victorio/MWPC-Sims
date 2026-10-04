#!/bin/bash

source /u/local/Modules/default/init/modules.sh
module load apptainer

apptainer shell ~/myimage.sif

# source /opt/root/bin/thisroot.sh
# source /opt/geant4/bin/geant4.sh
# source /opt/garfieldpp/share/Garfield/setupGarfield.sh