#/usr/bin/env bash

clx_diag port reset $1 speed=400g medium-type=cr8 lane-cnt=8 admin=enable

clx_diag port set property $1 an-lt=lt
