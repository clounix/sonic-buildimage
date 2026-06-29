#!/bin/bash

i2c_channel=0x1
cpld_addr=0x7f
cpld_reg=0x8
flash_change_bit=0
mask=$(( 1 << $flash_change_bit ))
MOD_NAME="nor_flash"
write_file=$1
write_address=$(($2))
flash_read_file="/home/admin/flash_read.bin"
size_byte=$(stat -c "%s" "${write_file}")
size_byte_dec=$((size_byte))

dot_loop() {
    while true; do echo -n "."; sleep 0.5; done
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

size_byte_hex=$(printf "0x%x" $size_byte_dec)
write_address_hex=$(printf "0x%x" $write_address)

echo "开始烧写$write_file到 flash 地址 $write_address_hex, 文件大小是$size_byte_hex 字节"
dot_loop &
DOT_PID=$!

sh -c "echo 'buf-write $write_file $write_address_hex'> /proc/nor_flash_mmio"

sleep 1
kill $DOT_PID
wait $DOT_PID 2>/dev/null
echo -e "\n烧录完成,开始对比flash内容和烧录文件是否一致..."

rm -f $flash_read_file

dot_loop &
DOT_PID=$!

sh -c "echo 'buf-read $flash_read_file $write_address_hex $size_byte_hex' > /proc/nor_flash_mmio"

sleep 1
kill $DOT_PID
wait $DOT_PID 2>/dev/null

if cmp -s "${write_file}" "${flash_read_file}"; then
	echo "\n校验通过：烧录文件与Flash读出内容完全一致"
else
	echo "\n校验失败：Flash内容与原始文件存在差异"
fi

