#!/bin/sh

GIT_HASH=$(git rev-parse --short HEAD)

sed -e "s/@GIT_HASH@/${GIT_HASH}/g" \
    template.h.in > inc/clx_dev_ver_released.h
