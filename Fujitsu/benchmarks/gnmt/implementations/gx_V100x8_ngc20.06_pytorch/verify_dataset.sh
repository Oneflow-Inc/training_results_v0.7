#!/bin/bash

DATADIR=${DATADIR:-"/data/wmt16"}

set -e

ACTUAL_SRC_TRAIN=`cat $DATADIR/train.tok.clean.bpe.32000.en |md5sum`
EXPECTED_SRC_TRAIN='b7482095b787264a310d4933d197a134  -'
if [[ $ACTUAL_SRC_TRAIN = $EXPECTED_SRC_TRAIN ]]; then
  echo "OK: correct $DATADIR/train.tok.clean.bpe.32000.en"
else
  echo "ERROR: incorrect $DATADIR/train.tok.clean.bpe.32000.en"
  echo "ERROR: expected $EXPECTED_SRC_TRAIN"
  echo "ERROR: found $ACTUAL_SRC_TRAIN"
fi

ACTUAL_TGT_TRAIN=`cat $DATADIR/train.tok.clean.bpe.32000.de |md5sum`
EXPECTED_TGT_TRAIN='409064aedaef5b7c458ff19a7beda462  -'
if [[ $ACTUAL_TGT_TRAIN = $EXPECTED_TGT_TRAIN ]]; then
  echo "OK: correct $DATADIR/train.tok.clean.bpe.32000.de"
else
  echo "ERROR: incorrect $DATADIR/train.tok.clean.bpe.32000.de"
  echo "ERROR: expected $EXPECTED_TGT_TRAIN"
  echo "ERROR: found $ACTUAL_TGT_TRAIN"
fi

ACTUAL_SRC_VAL=`cat $DATADIR/newstest_dev.tok.clean.bpe.32000.en |md5sum`
EXPECTED_SRC_VAL='704c4ba8c8b63df1f6678d32b91438b5  -'
if [[ $ACTUAL_SRC_VAL = $EXPECTED_SRC_VAL ]]; then
  echo "OK: correct $DATADIR/newstest_dev.tok.clean.bpe.32000.en"
else
  echo "ERROR: incorrect $DATADIR/newstest_dev.tok.clean.bpe.32000.en"
  echo "ERROR: expected $EXPECTED_SRC_VAL"
  echo "ERROR: found $ACTUAL_SRC_VAL"
fi

ACTUAL_TGT_VAL=`cat $DATADIR/newstest_dev.tok.clean.bpe.32000.de |md5sum`
EXPECTED_TGT_VAL='d27f5a64c839e20c5caa8b9d60075dde  -'
if [[ $ACTUAL_TGT_VAL = $EXPECTED_TGT_VAL ]]; then
  echo "OK: correct $DATADIR/newstest_dev.tok.clean.bpe.32000.de"
else
  echo "ERROR: incorrect $DATADIR/newstest_dev.tok.clean.bpe.32000.de"
  echo "ERROR: expected $EXPECTED_TGT_VAL"
  echo "ERROR: found $ACTUAL_TGT_VAL"
fi

ACTUAL_SRC_TEST=`cat $DATADIR/newstest2014.tok.bpe.32000.en |md5sum`
EXPECTED_SRC_TEST='cb014e2509f86cd81d5a87c240c07464  -'
if [[ $ACTUAL_SRC_TEST = $EXPECTED_SRC_TEST ]]; then
  echo "OK: correct $DATADIR/newstest2014.tok.bpe.32000.en"
else
  echo "ERROR: incorrect $DATADIR/newstest2014.tok.bpe.32000.en"
  echo "ERROR: expected $EXPECTED_SRC_TEST"
  echo "ERROR: found $ACTUAL_SRC_TEST"
fi

ACTUAL_TGT_TEST=`cat $DATADIR/newstest2014.tok.bpe.32000.de |md5sum`
EXPECTED_TGT_TEST='d616740f6026dc493e66efdf9ac1cb04  -'
if [[ $ACTUAL_TGT_TEST = $EXPECTED_TGT_TEST ]]; then
  echo "OK: correct $DATADIR/newstest2014.tok.bpe.32000.de"
else
  echo "ERROR: incorrect $DATADIR/newstest2014.tok.bpe.32000.de"
  echo "ERROR: expected $EXPECTED_TGT_TEST"
  echo "ERROR: found $ACTUAL_TGT_TEST"
fi

ACTUAL_TGT_TEST_TARGET=`cat $DATADIR/newstest2014.de |md5sum`
EXPECTED_TGT_TEST_TARGET='f6c3818b477e4a25cad68b61cc883c17  -'
if [[ $ACTUAL_TGT_TEST_TARGET = $EXPECTED_TGT_TEST_TARGET ]]; then
  echo "OK: correct $DATADIR/newstest2014.de"
else
  echo "ERROR: incorrect $DATADIR/newstest2014.de"
  echo "ERROR: expected $EXPECTED_TGT_TEST_TARGET"
  echo "ERROR: found $ACTUAL_TGT_TEST_TARGET"
fi
