#!/opt/homebrew/bin/bash

source $GARFIELD_HOME/install/share/Garfield/setupGarfield.sh
echo $GARFIELD_IONDATA
export DCSimNtrack=1
export DCSimtrackang=0.0

export DCSimtrackx=7.62
export DCSimOutFile=DCsimX1p0A0p0.root
./smalljet_drifttime
