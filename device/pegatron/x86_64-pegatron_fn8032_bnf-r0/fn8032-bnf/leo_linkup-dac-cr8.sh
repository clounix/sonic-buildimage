#/usr/bin/env bash

npx_diag port reset $1 speed=400g medium-type=cr8 lane-cnt=8 admin=enable

npx_diag port set property $1 an-lt=lt
