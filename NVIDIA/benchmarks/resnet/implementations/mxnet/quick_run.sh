docker build --pull -t mlperf-nvidia:image_classification .
source config_DGX1.sh
CONT=mlperf-nvidia:image_classification DATADIR=/dataset/imagenet-mxnet LOGDIR=log ./run_with_docker.sh
