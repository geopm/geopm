#!/bin/bash
#  Copyright (c) 2015, 2016, 2017, 2018, 2019, 2020, Intel Corporation
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#      * Redistributions of source code must retain the above copyright
#        notice, this list of conditions and the following disclaimer.
#
#      * Redistributions in binary form must reproduce the above copyright
#        notice, this list of conditions and the following disclaimer in
#        the documentation and/or other materials provided with the
#        distribution.
#
#      * Neither the name of Intel Corporation nor the names of its
#        contributors may be used to endorse or promote products derived
#        from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY LOG OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

set -x
set -e

# Get helper functions
source ../build_func.sh

# Full tutorial here:
# https://github.com/IntelAI/models/blob/master/docs/image_recognition/tensorflow/Tutorial.md

#gather dataset
cd IntelAIModel/datasets/coco/

get_archive val2017.zip http://images.cocodataset.org/zips
unzip val2017.zip

get_archive train2017.zip http://images.cocodataset.org/zips
unzip train2017.zip

get_archive test2017.zip http://images.cocodataset.org/zips
unzip test2017.zip

get_archive annotations_trainval2017.zip http://images.cocodataset.org/annotations
unzip annotations_trainval2017.zip

#get_archive stuff_annotations_trainval2017.zip http://images.cocodataset.org/annotations
#unzip stuff_annotations_trainval2017.zip
#
#get_archive panoptic_annotations_trainval2017.zip http://images.cocodataset.org/annotations
#unzip panoptic_annotations_trainval2017.zip
#
#get_archive image_info_test2017.zip http://images.cocodataset.org/annotations
#unzip image_info_test2017.zip
#
#get_archive image_info_unlabeled2017.zip http://images.cocodataset.org/annotations
#unzip image_info_unlabeled2017.zip

mkdir tf_records
export DATASET_DIR=$PWD/
export OUTPUT_DIR=$PWD/tf_records

#Get tensorflow
#git clone https://github.com/tensorflow/models.git tensorflow-models
#cd tensorflow-models
export TF_MODELS_BRANCH=1efe98bb8e8d98bbffc703a90d88df15fc2ce906
#export TF_MODELS_DIR=$(pwd)
#cd ..

#TODO: try podman first

docker run \
   --env VAL_IMAGE_DIR=${DATASET_DIR}/val2017 \
   --env ANNOTATIONS_DIR=${DATASET_DIR}/annotations \
   --env TF_MODELS_BRANCH=${TF_MODELS_BRANCH} \
   --env OUTPUT_DIR=${OUTPUT_DIR} \
   -v ${DATASET_DIR}:${DATASET_DIR} \
   -v ${OUTPUT_DIR}:${OUTPUT_DIR} \
   -t intel/object-detection:tf-1.15.2-imz-2.2.0-preprocess-coco-val

mv ${OUTPUT_DIR}/coco_val.record ${OUTPUT_DIR}/validation-00000-of-00001
cp -r ${DATASET_DIR}/annotations ${OUTPUT_DIR}
