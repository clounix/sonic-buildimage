clx_diag vlan del member vid=1 portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 untag-portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248


clx_diag vlan del member vid=1000 portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 untag-portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248

clx_diag port set intf-property portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 mtu=9100



clx_diag port set intf-property portlist=0 pvid=100
sleep 1
clx_diag port set intf-property portlist=8 pvid=101
sleep 1
clx_diag port set intf-property portlist=16 pvid=102
sleep 1
clx_diag port set intf-property portlist=24 pvid=103
sleep 1
clx_diag port set intf-property portlist=32 pvid=104
clx_diag port set intf-property portlist=40 pvid=105
clx_diag port set intf-property portlist=48 pvid=106
clx_diag port set intf-property portlist=56 pvid=107
clx_diag port set intf-property portlist=64 pvid=108
clx_diag port set intf-property portlist=72 pvid=109
clx_diag port set intf-property portlist=80 pvid=110
clx_diag port set intf-property portlist=88 pvid=111
clx_diag port set intf-property portlist=96 pvid=112
clx_diag port set intf-property portlist=104 pvid=113
clx_diag port set intf-property portlist=112 pvid=114
clx_diag port set intf-property portlist=120 pvid=115
clx_diag port set intf-property portlist=128 pvid=116
clx_diag port set intf-property portlist=136 pvid=117
clx_diag port set intf-property portlist=144 pvid=118
clx_diag port set intf-property portlist=152 pvid=119
clx_diag port set intf-property portlist=160 pvid=120
clx_diag port set intf-property portlist=168 pvid=121
clx_diag port set intf-property portlist=176 pvid=122
clx_diag port set intf-property portlist=184 pvid=123
clx_diag port set intf-property portlist=192 pvid=124
clx_diag port set intf-property portlist=200 pvid=125
clx_diag port set intf-property portlist=208 pvid=126
clx_diag port set intf-property portlist=216 pvid=127
clx_diag port set intf-property portlist=224 pvid=128
clx_diag port set intf-property portlist=232 pvid=129
clx_diag port set intf-property portlist=240 pvid=130
clx_diag port set intf-property portlist=248 pvid=131







clx_diag vlan add member vid=100  portlist=0,8     untag-portlist=0,8  
clx_diag vlan add member vid=101  portlist=8,16      untag-portlist=8,16    
clx_diag vlan add member vid=102  portlist=16,24      untag-portlist=16,24   
clx_diag vlan add member vid=103  portlist=24,32     untag-portlist=24,32  
clx_diag vlan add member vid=104  portlist=32,40    untag-portlist=32,40 
clx_diag vlan add member vid=105  portlist=40,48   untag-portlist=40,48
clx_diag vlan add member vid=106  portlist=48,56   untag-portlist=48,56  
clx_diag vlan add member vid=107  portlist=56,64    untag-portlist=56,64  
clx_diag vlan add member vid=108  portlist=64,72    untag-portlist=64,72 
clx_diag vlan add member vid=109  portlist=72,80    untag-portlist=72,80 
clx_diag vlan add member vid=110  portlist=80,88    untag-portlist=80,88 
clx_diag vlan add member vid=111  portlist=88,96    untag-portlist=88,96 
clx_diag vlan add member vid=112  portlist=96,104    untag-portlist=96,104 
clx_diag vlan add member vid=113  portlist=104,112    untag-portlist=104,112 
clx_diag vlan add member vid=114  portlist=112,120    untag-portlist=112,120 
clx_diag vlan add member vid=115  portlist=120,128    untag-portlist=120,128 
clx_diag vlan add member vid=116  portlist=128,136    untag-portlist=128,136 
clx_diag vlan add member vid=117  portlist=136,144    untag-portlist=136,144 
clx_diag vlan add member vid=118  portlist=144,152   untag-portlist=144,152 
clx_diag vlan add member vid=119  portlist=152,160    untag-portlist=152,160
clx_diag vlan add member vid=120  portlist=160,168   untag-portlist=160,168 
clx_diag vlan add member vid=121  portlist=168,176    untag-portlist=168,176
clx_diag vlan add member vid=122  portlist=176,184    untag-portlist=176,184  
clx_diag vlan add member vid=123  portlist=184,192    untag-portlist=184,192
clx_diag vlan add member vid=124  portlist=192,200    untag-portlist=192,200 
clx_diag vlan add member vid=125  portlist=200,208    untag-portlist=200,208 
clx_diag vlan add member vid=126  portlist=208,216   untag-portlist=208,216
clx_diag vlan add member vid=127 portlist=216,224    untag-portlist=216,224 
clx_diag vlan add member vid=128 portlist=224,232    untag-portlist=224,232
clx_diag vlan add member vid=129 portlist=232,240    untag-portlist=232,240
clx_diag vlan add member vid=130 portlist=240,248    untag-portlist=240,248
clx_diag vlan add member vid=131 portlist=248,0    untag-portlist=248,0 




clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=100  port=8
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=101  port=16
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=102  port=24
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=103  port=32
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=104  port=40
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=105  port=48
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=106  port=56
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=107  port=64
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=108  port=72
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=109  port=80
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=110  port=88
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=111  port=96
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=112  port=104
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=113  port=112
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=114  port=120
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=115  port=128
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=116  port=136
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=117  port=144
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=118  port=152
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=119  port=160
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=120  port=168
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=121  port=176
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=122  port=184
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=123  port=192
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=124  port=200
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=125 port=208
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=126 port=216
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=127 port=224
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=128 port=232
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=129 port=240
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=130 port=248
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=131 port=0






clx_diag pkt set sequence clear
clx_diag pkt set monitor sequence
clx_diag pkt clear monitor

clx_diag pkt send tx portlist=0   len=1028 num=2000 dmac=00:00:00:00:00:01 smac=00:00:00:01:00:01 payload=0xffff sequence





clx_diag tm set shaper  portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 max-bw=1000000 max-burst=64000000
clx_diag tm set shaper  portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 queue=0 max-bw=1000000 max-burst=64000000



sleep 5



clx_diag netif destroy profile profile-id=0



clx_diag pkt set monitor disable

clx_diag tm set shaper  portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 max-bw=0 max-burst=64000000
clx_diag tm set shaper  portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 queue=0 max-bw=0 max-burst=64000000

clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=100 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=101 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=102
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=103 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=104
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=105
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=106
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=107
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=108 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=109 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=110 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=111
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=112
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=113 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=114 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=115 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=116 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=117 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=118 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=119 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=120 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=121 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=122
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=123 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=124
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=125 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=126
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=127
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=128
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=129
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=130
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=131




clx_diag vlan del member vid=100  portlist=0,8     untag-portlist=0,8  
clx_diag vlan del member vid=101  portlist=8,16      untag-portlist=8,16    
clx_diag vlan del member vid=102  portlist=16,24      untag-portlist=16,24   
clx_diag vlan del member vid=103  portlist=24,32     untag-portlist=24,32  
clx_diag vlan del member vid=104  portlist=32,40    untag-portlist=32,40 
clx_diag vlan del member vid=105  portlist=40,48   untag-portlist=40,48
clx_diag vlan del member vid=106  portlist=48,56   untag-portlist=48,56  
clx_diag vlan del member vid=107  portlist=56,64    untag-portlist=56,64  
clx_diag vlan del member vid=108  portlist=64,72    untag-portlist=64,72 
clx_diag vlan del member vid=109  portlist=72,80    untag-portlist=72,80 
clx_diag vlan del member vid=110  portlist=80,88    untag-portlist=80,88 
clx_diag vlan del member vid=111  portlist=88,96    untag-portlist=88,96 
clx_diag vlan del member vid=112  portlist=96,104    untag-portlist=96,104 
clx_diag vlan del member vid=113  portlist=104,112    untag-portlist=104,112 
clx_diag vlan del member vid=114  portlist=112,120    untag-portlist=112,120 
clx_diag vlan del member vid=115  portlist=120,128    untag-portlist=120,128 
clx_diag vlan del member vid=116  portlist=128,136    untag-portlist=128,136 
clx_diag vlan del member vid=117  portlist=136,144    untag-portlist=136,144 
clx_diag vlan del member vid=118  portlist=144,152   untag-portlist=144,152 
clx_diag vlan del member vid=119  portlist=152,160    untag-portlist=152,160
clx_diag vlan del member vid=120  portlist=160,168   untag-portlist=160,168 
clx_diag vlan del member vid=121  portlist=168,176    untag-portlist=168,176
clx_diag vlan del member vid=122  portlist=176,184    untag-portlist=176,184  
clx_diag vlan del member vid=123  portlist=184,192    untag-portlist=184,192
clx_diag vlan del member vid=124  portlist=192,200    untag-portlist=192,200 
clx_diag vlan del member vid=125  portlist=200,208    untag-portlist=200,208 
clx_diag vlan del member vid=126  portlist=208,216   untag-portlist=208,216
clx_diag vlan del member vid=127 portlist=216,224    untag-portlist=216,224 
clx_diag vlan del member vid=128 portlist=224,232    untag-portlist=224,232
clx_diag vlan del member vid=129 portlist=232,240    untag-portlist=232,240
clx_diag vlan del member vid=130 portlist=240,248    untag-portlist=240,248
clx_diag vlan del member vid=131 portlist=248,0    untag-portlist=248,0 





clx_diag port set intf-property portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248  mtu=1536

clx_diag port set intf-property portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248  pvid=1

clx_diag vlan add member vid=1 portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248  untag-portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 

clx_diag vlan add member vid=1000 portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248  untag-portlist=0,8,16,24,32,40,48,56,64,72,80,88,96,104,112,120,128,136,144,152,160,168,176,184,192,200,208,216,224,232,240,248 

clx_diag stat show unit=0 portlist=0 > 0.txt 
clx_diag stat show unit=0 portlist=8 > 8.txt 
clx_diag stat show unit=0 portlist=16 > 16.txt 
clx_diag stat show unit=0 portlist=24 > 24.txt 
clx_diag stat show unit=0 portlist=32 > 32.txt 
clx_diag stat show unit=0 portlist=40 > 40.txt 
clx_diag stat show unit=0 portlist=48 > 48.txt 
clx_diag stat show unit=0 portlist=56 > 56.txt 
clx_diag stat show unit=0 portlist=64 > 64.txt 
clx_diag stat show unit=0 portlist=72 > 72.txt 
clx_diag stat show unit=0 portlist=80 > 80.txt 
clx_diag stat show unit=0 portlist=88 > 88.txt 
clx_diag stat show unit=0 portlist=96 > 96.txt 
clx_diag stat show unit=0 portlist=104 > 104.txt 
clx_diag stat show unit=0 portlist=112 > 112.txt 
clx_diag stat show unit=0 portlist=120 > 120.txt 
clx_diag stat show unit=0 portlist=128 > 128.txt 
clx_diag stat show unit=0 portlist=136 > 136.txt 
clx_diag stat show unit=0 portlist=144 > 144.txt 
clx_diag stat show unit=0 portlist=152 > 152.txt 
clx_diag stat show unit=0 portlist=160 > 160.txt 
clx_diag stat show unit=0 portlist=168 > 168.txt 
clx_diag stat show unit=0 portlist=176 > 176.txt 
clx_diag stat show unit=0 portlist=184 > 184.txt 
clx_diag stat show unit=0 portlist=192 > 192.txt 
clx_diag stat show unit=0 portlist=200 > 200.txt 
clx_diag stat show unit=0 portlist=208 > 208.txt 
clx_diag stat show unit=0 portlist=216 > 216.txt 
clx_diag stat show unit=0 portlist=224 > 224.txt 
clx_diag stat show unit=0 portlist=232 > 232.txt 
clx_diag stat show unit=0 portlist=240 > 240.txt 
clx_diag stat show unit=0 portlist=248 > 248.txt 

