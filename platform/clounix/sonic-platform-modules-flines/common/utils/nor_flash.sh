#!/bin/bash

i2c_channel=0x1
cpld_addr=0x7f
cpld_reg=0x8
flash_change_bit=0
mask=$(( 1 << $flash_change_bit ))
MOD_NAME="nor_flash"

dot_loop() {
    while true; do echo -n "."; sleep 0.1; done
}

if [ -z "$1" ]; then
	echo "文件路径不能为空"
	exit 1
else
	if [ ! -e "$1" ]; then
		echo "$1 路径不存在"
		exit 1
	fi
fi

if [ -z "$2" ]; then
	echo "要写入flash的地址不能为空"
	exit 1
fi


if lsmod | grep -q "^${MOD_NAME} "; then
	echo "模块 $MOD_NAME 已加载"
else
	echo "模块 $MOD_NAME 未加载，需要加载模块"
	MOD_FILE=$(modinfo "$MOD_NAME.ko" 2>/dev/null | awk '/^filename:/ {print $2}')
	if [ -n "${MOD_FILE}" ] && [ -f "${MOD_FILE}" ]; then
		echo "模块文件存在：${MOD_FILE}，开始加载"
		sh -c "insmod $MOD_FILE"
	else
		echo "模块不存在或获取路径失败"
		exit 1
	fi
fi


reg_val=$(sudo i2cget -y -a $i2c_channel $cpld_addr $cpld_reg 2>&1)
if [[ "$reg_val" == Error* ]]; then
	echo "读取寄存器失败：$reg_val"
	exit 1
fi

echo "原始读取值: $reg_val"

if [ $(( $((reg_val)) & mask )) -eq 1 ]; then
	echo "当前flash为备flash，需要切换到主flash"

	new=$(( $((reg_val)) & ~mask ))
	sudo i2cset -y -a $i2c_channel $cpld_addr $cpld_reg $new
	dec_val=$(sudo i2cget -y -a $i2c_channel $cpld_addr $cpld_reg 2>&1)
	if [[ "$dec_val" == Error* ]]; then
		echo "写入后读取寄存器失败：$dec_val"
		exit 1
	fi
	if [ $(( $((dec_val)) & mask )) -eq 0 ]; then
		echo "当前flash为主flash，切换flash成功"
	else
		echo "当前flash为备flash，切换失败"
		exit 1
	fi
	echo "切换后的值: $dec_val"
else
	echo "当前flash为主flash，可以烧录"	
fi


echo "开始烧写$1 到 flash 地址 $2"
dot_loop &
DOT_PID=$!

sh -c "echo 'buf-write $1 $2'> /proc/nor_flash_mmio"

sleep 1
kill $DOT_PID
wait $DOT_PID 2>/dev/null
echo -e "\n操作完成！"
