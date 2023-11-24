#/usr/bin/env bash

npx_diag port reset $1 speed=$2 medium-type=$3 lane-cnt=$4 admin=enable

npx_diag port set property $1 an-lt=lt

