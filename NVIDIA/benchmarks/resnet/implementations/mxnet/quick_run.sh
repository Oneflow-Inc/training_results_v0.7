docker build --pull -t mlperf-nvidia:rn50 .
source config_DGX1.sh
#CONT=mlperf-nvidia:image_classification DATADIR=/data/imagenet/MXNet LOGDIR=log ./run_with_docker.sh
export BATCHSIZE="32"
export DGXNGPU=1
export CUDA_VISIBLE_DEVICES=3
CONT=mlperf-nvidia:rn50 DATADIR=/dataset/imagenet-mxnet LOGDIR=log ./run_with_docker.sh
