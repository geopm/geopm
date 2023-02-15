#  Copyright (c) 2015 - 2023, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

if [ -f ${HOME}/.geopmrc ]; then
    source ${HOME}/.geopmrc
fi

source $GEOPM_SOURCE/integration/config/build_env.sh

# Clean out old versions of application source and warn user
clean_source() {
    local DIRNAME=$1
    # Clear out old versions:
    if [ -d "${DIRNAME}" ]; then
        echo "WARNING: Previous source directory detected at ./${DIRNAME}"
        read -p "OK to delete and rebuild? (y/n) " -n 1 -r
        echo
        if [[ "${REPLY}" =~ ^[Yy]$ ]]; then
            rm -rf ${DIRNAME}
        else
            echo "Not OK.  Stopping." 1>&2
            exit 1
        fi
    fi
}

# Clone a git reposutory
clone_repo_git(){
    local REPOURL=$1
    local DIRNAME=$2
    local TOPHASH=$3
    local CLONEARGS=$4
    local ARCHIVE=${DIRNAME}_${TOPHASH}.tgz

    git clone ${CLONEARGS} ${REPOURL} ${DIRNAME}
    if [ ! -d ${DIRNAME} ]; then
        echo "Error: Unable to clone ${DIRNAME} source."
        return 1
    fi

    cd ${DIRNAME}
    if ! git reset --hard ${TOPHASH}; then
        echo "Error: Could not reset to specified git hash: ${TOPHASH}." 1>&2
        return 1
    fi
    cd -
    backup_archive ${ARCHIVE} ${DIRNAME}
}

backup_archive() {
    local ARCHIVE=$1
    local DIRNAME=$2 # Optional; Used when this function should create ARCHIVE

    # If the user set the cache dir, pack up the source for next time
    if [ -d ${GEOPM_APPS_SOURCES} ]; then
        if [ ! -f ${ARCHIVE} ]; then
            tar czvf ${ARCHIVE} ${DIRNAME}
        fi
        if [ ! -f ${GEOPM_APPS_SOURCE}/${ARCHIVE} ]; then
            cp ${ARCHIVE} ${GEOPM_APPS_SOURCES}
        fi
    else
        echo "Warning: Please set GEOPM_APPS_SOURCES in your environment to enable the local source code cache."
    fi
}

# Setup a git repository and apply patches
setup_source_git() {
    local BASEDIR=${PWD}
    local DIRNAME=$1
    if [ $# == 2 ]; then
        local PATCH_LIST=$2
    else
        local PATCH_LIST="$(ls ${BASEDIR}/*.patch 2> /dev/null || true)"
    fi
    cd ${DIRNAME}
    # Create a git repo for the app source
    if [ ! -d .git ]; then
        git init
        git checkout -b main
        git add -A
        git commit --no-edit -sm "Initial commit"
    fi

    if [ ! -z  "${PATCH_LIST}" ]; then
        git am ${PATCH_LIST}
    fi
    cd -
}

# Get the source archive from local cache or web page
get_archive() {
    local ARCHIVE=$1
    if [ ! -f ${ARCHIVE} ]; then
        if [ -f "${GEOPM_APPS_SOURCES}/${ARCHIVE}" ]; then
            ln -s "${GEOPM_APPS_SOURCES}/${ARCHIVE}"
        elif [ $# -eq 2 ]; then
            local URL=$2
            wget ${URL}/${ARCHIVE}
            backup_archive ${ARCHIVE}
        fi
    fi
}

# Unpack an archive with tar or unzip
unpack_archive() {
    local ARCHIVE=$1
    if [ "${ARCHIVE##*.}" == zip ]; then
        unzip ${ARCHIVE}
    else
        # Inspect the tarball to get the root DIR.  Assume that if the
        # root dir already exists in the PWD that this has already been extracted.
        local DIRNAME=$(tar -tf ${ARCHIVE} | head -1 | cut -f1 -d"/")
        if [ ! -d ${DIRNAME} ]; then
            tar xvf ${ARCHIVE}
        fi
    fi
}

# Clear out previously installed binaries
clean_geopm() {
    local BUILD_DIR=$1
    if [ -e "${BUILD_DIR}" ]; then
        echo "WARNING: Previous build of geopm or other data found at ${BUILD_DIR}"
        read -p "OK to delete all object files and recompile? (y/n) " -n 1 -r
        echo
        if [[ "${REPLY}" =~ ^[Yy]$ ]]; then
            rm -rf ${BUILD_DIR}
        elif [[ "${REPLY}" =~ ^[Nn]$ ]]; then
            echo "WARNING: Executing incremental build"
        else
            echo "Error: Did not understand reply: ${REPLY}" 1>&2
            return 1
        fi
    fi
    if [ -e "${GEOPM_INSTALL}" ]; then
        echo "WARNING: Previous install of geopm or other data found at ${GEOPM_INSTALL}"
        read -p "OK to delete and reinstall? (y/n) " -n 1 -r
        echo
        if [[ ${REPLY} =~ ^[Yy]$ ]]; then
            rm -rf ${GEOPM_INSTALL}
        else
            echo "Not OK.  Stopping." 1>&2
            return 1
        fi
    fi
}
