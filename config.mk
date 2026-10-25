#----------------------------- User configuration -----------------------------

# Common settings
#------------------------

# How you're loading your module. Used to determine how to find the target module. (AsRtld/Module/Kip)
LOAD_KIND := Module

# ExeFS slot of the module. subsdk7 leaves subsdk8 / subsdk9 free for other exlaunch mods, so they can be installed together.
SUBSDK := subsdk7

# Program you're targetting. Used to determine where to deploy your files.
PROGRAM_ID := 0100CF5010FEC000

# Optional path to copy the final ELF to, for convenience.
ELF_EXTRACT :=

# Python command to use. Must be Python 3.4+.
PYTHON := python3

# JSON to use to make .npdm
NPDM_JSON := application.json

# Additional C/C++ flags to use. LOD options (source/program/main.cpp) go here, e.g. -DLOD_TARGET=2 for the test build.
C_FLAGS :=
CXX_FLAGS :=

# AsRtld settings
#------------------------

# Path to the SD card. Used to mount and deploy files on SD, likely with hekate UMS.
MOUNT_PATH :=

# Module settings
#------------------------

# Settings for deploying over FTP. Used by the deploy-ftp.py script.
FTP_IP :=
FTP_PORT := 5000
FTP_USERNAME := anonymous
FTP_PASSWORD :=

# Settings for deploying to Ryu. Used by the deploy-ryu.sh script.
RYU_PATH :=
