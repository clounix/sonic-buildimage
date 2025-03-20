clx_diag vlan del member vid=1 portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 untag-portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152


clx_diag vlan del member vid=1000 portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 untag-portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152

clx_diag port set intf-property portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 mtu=9100


clx_diag port set intf-property portlist=0 pvid=100
sleep 1
clx_diag port set intf-property portlist=2 pvid=102
sleep 1
clx_diag port set intf-property portlist=4 pvid=104
sleep 1
clx_diag port set intf-property portlist=6 pvid=106
sleep 1
clx_diag port set intf-property portlist=8 pvid=108
clx_diag port set intf-property portlist=10 pvid=110
clx_diag port set intf-property portlist=12 pvid=112
clx_diag port set intf-property portlist=14 pvid=114
clx_diag port set intf-property portlist=16 pvid=116
clx_diag port set intf-property portlist=18 pvid=118
clx_diag port set intf-property portlist=20 pvid=120
clx_diag port set intf-property portlist=22 pvid=122
clx_diag port set intf-property portlist=24 pvid=124
clx_diag port set intf-property portlist=26 pvid=126
clx_diag port set intf-property portlist=28 pvid=128
clx_diag port set intf-property portlist=30 pvid=130
clx_diag port set intf-property portlist=32 pvid=132
clx_diag port set intf-property portlist=34 pvid=134
clx_diag port set intf-property portlist=36 pvid=136
clx_diag port set intf-property portlist=38 pvid=138
clx_diag port set intf-property portlist=40 pvid=140
clx_diag port set intf-property portlist=42 pvid=142
clx_diag port set intf-property portlist=44 pvid=144
clx_diag port set intf-property portlist=46 pvid=146
clx_diag port set intf-property portlist=48 pvid=148
clx_diag port set intf-property portlist=50 pvid=150
clx_diag port set intf-property portlist=52 pvid=152
clx_diag port set intf-property portlist=54 pvid=154
clx_diag port set intf-property portlist=56 pvid=156
clx_diag port set intf-property portlist=58 pvid=158
clx_diag port set intf-property portlist=60 pvid=160
clx_diag port set intf-property portlist=62 pvid=162
clx_diag port set intf-property portlist=64 pvid=164
clx_diag port set intf-property portlist=66 pvid=166
clx_diag port set intf-property portlist=68 pvid=168
clx_diag port set intf-property portlist=70 pvid=170
clx_diag port set intf-property portlist=72 pvid=172
clx_diag port set intf-property portlist=74 pvid=174
clx_diag port set intf-property portlist=76 pvid=176
clx_diag port set intf-property portlist=78 pvid=178
clx_diag port set intf-property portlist=80 pvid=180
clx_diag port set intf-property portlist=82 pvid=182
clx_diag port set intf-property portlist=84 pvid=184
clx_diag port set intf-property portlist=86 pvid=186
clx_diag port set intf-property portlist=88 pvid=188
clx_diag port set intf-property portlist=90 pvid=190
clx_diag port set intf-property portlist=92 pvid=192
clx_diag port set intf-property portlist=94 pvid=194

clx_diag port set intf-property portlist=96 pvid=196
clx_diag port set intf-property portlist=104 pvid=1104
clx_diag port set intf-property portlist=112 pvid=1112
clx_diag port set intf-property portlist=120 pvid=1120
clx_diag port set intf-property portlist=128 pvid=1128
clx_diag port set intf-property portlist=136 pvid=1136
clx_diag port set intf-property portlist=144 pvid=1144
clx_diag port set intf-property portlist=152 pvid=1152




clx_diag vlan add member vid=100  portlist=0,2     untag-portlist=0,2  
clx_diag vlan add member vid=102  portlist=2,4       untag-portlist=2,4    
clx_diag vlan add member vid=104  portlist=4,6      untag-portlist=4,6   
clx_diag vlan add member vid=106  portlist=6,8     untag-portlist=6,8  
clx_diag vlan add member vid=108  portlist=8,10    untag-portlist=8,10 
clx_diag vlan add member vid=110  portlist=10,12    untag-portlist=10,12
clx_diag vlan add member vid=112  portlist=12,14   untag-portlist=12,14  
clx_diag vlan add member vid=114  portlist=14,16    untag-portlist=14,16 
clx_diag vlan add member vid=116  portlist=16,18    untag-portlist=16,18 
clx_diag vlan add member vid=118  portlist=18,20    untag-portlist=18,20 
clx_diag vlan add member vid=120  portlist=20,22    untag-portlist=20,22 
clx_diag vlan add member vid=122  portlist=22,24    untag-portlist=22,24 
clx_diag vlan add member vid=124  portlist=24,26    untag-portlist=24,26 
clx_diag vlan add member vid=126  portlist=26,28    untag-portlist=26,28 
clx_diag vlan add member vid=128  portlist=28,30    untag-portlist=28,30 
clx_diag vlan add member vid=130  portlist=30,32    untag-portlist=30,32 
clx_diag vlan add member vid=132  portlist=32,34    untag-portlist=32,34 
clx_diag vlan add member vid=134  portlist=34,36    untag-portlist=34,36 
clx_diag vlan add member vid=136  portlist=36,38    untag-portlist=36,38 
clx_diag vlan add member vid=138  portlist=38,40    untag-portlist=38,40
clx_diag vlan add member vid=140  portlist=40,42    untag-portlist=40,42 
clx_diag vlan add member vid=142  portlist=42,44    untag-portlist=42,44 
clx_diag vlan add member vid=144  portlist=44,46    untag-portlist=44,46 
clx_diag vlan add member vid=146  portlist=46,48    untag-portlist=46,48 
clx_diag vlan add member vid=148  portlist=48,50    untag-portlist=48,50 
clx_diag vlan add member vid=150  portlist=50,52    untag-portlist=50,52 
clx_diag vlan add member vid=152  portlist=52,54    untag-portlist=52,54
clx_diag vlan add member vid=154 portlist=54,56    untag-portlist=54,56 
clx_diag vlan add member vid=156 portlist=56,58    untag-portlist=56,58
clx_diag vlan add member vid=158 portlist=58,60    untag-portlist=58,60
clx_diag vlan add member vid=160 portlist=60,62    untag-portlist=60,62
clx_diag vlan add member vid=162 portlist=62,64    untag-portlist=62,64
clx_diag vlan add member vid=164 portlist=64,66    untag-portlist=64,66
clx_diag vlan add member vid=166 portlist=66,68    untag-portlist=66,68
clx_diag vlan add member vid=168 portlist=68,70    untag-portlist=68,70
clx_diag vlan add member vid=170 portlist=70,72    untag-portlist=70,72
clx_diag vlan add member vid=172 portlist=72,74    untag-portlist=72,74
clx_diag vlan add member vid=174 portlist=74,76    untag-portlist=74,76
clx_diag vlan add member vid=176 portlist=76,78    untag-portlist=76,78
clx_diag vlan add member vid=178 portlist=78,80    untag-portlist=78,80
clx_diag vlan add member vid=180 portlist=80,82    untag-portlist=80,82
clx_diag vlan add member vid=182 portlist=82,84    untag-portlist=82,84
clx_diag vlan add member vid=184 portlist=84,86    untag-portlist=84,86
clx_diag vlan add member vid=186 portlist=86,88    untag-portlist=86,88
clx_diag vlan add member vid=188 portlist=88,90    untag-portlist=88,90
clx_diag vlan add member vid=190 portlist=90,92    untag-portlist=90,92
clx_diag vlan add member vid=192 portlist=92,94    untag-portlist=92,94
clx_diag vlan add member vid=194 portlist=94,0     untag-portlist=94,0 


clx_diag vlan add member vid=196 portlist=96,104 untag-portlist=96,104
clx_diag vlan add member vid=1104 portlist=104,112 untag-portlist=104,112
clx_diag vlan add member vid=1112 portlist=112,120 untag-portlist=112,120
clx_diag vlan add member vid=1120 portlist=120,128 untag-portlist=120,128
clx_diag vlan add member vid=1128 portlist=128,136 untag-portlist=128,136
clx_diag vlan add member vid=1136 portlist=136,144 untag-portlist=136,144
clx_diag vlan add member vid=1144 portlist=144,152 untag-portlist=144,152
clx_diag vlan add member vid=1152 portlist=152,96 untag-portlist=152,96


clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=100  port=2
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=102  port=4
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=104  port=6
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=106  port=8
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=108  port=10
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=110  port=12
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=112  port=14
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=114  port=16
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=116  port=18
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=118  port=20
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=120  port=22
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=122  port=24
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=124  port=26
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=126  port=28
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=128  port=30
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=130  port=32
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=132  port=34
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=134  port=36
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=136  port=38
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=138  port=40
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=140  port=42
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=142  port=44
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=144  port=46
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=146  port=48
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=148  port=50
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=150 port=52
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=152 port=54
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=154 port=56
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=156 port=58
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=158 port=60
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=160 port=62
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=162 port=64
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=164 port=66
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=166 port=68
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=168 port=70
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=170 port=72
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=172 port=74
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=174 port=76
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=176 port=78
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=178 port=80
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=180 port=82
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=182 port=84
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=184 port=86
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=186 port=88
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=188 port=90
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=190 port=92
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=192 port=94
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=194 port=0

clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=196 port=104
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=1104 port=112
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=1112 port=120
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=1120 port=128
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=1128 port=136
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=1136 port=144
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=1144 port=152
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=1152 port=96






clx_diag pkt set sequence clear
clx_diag pkt set monitor sequence
clx_diag pkt clear monitor

clx_diag pkt send tx portlist=0   len=1028 num=2000 dmac=00:00:00:00:00:01 smac=00:00:00:01:00:01 payload=0xffff sequence
clx_diag pkt send tx portlist=96  len=1028 num=2000 dmac=00:00:00:00:00:02 smac=00:00:00:01:00:01 payload=0xffff sequence



clx_diag tm set shaper  portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 max-bw=1000000 max-burst=64000000
clx_diag tm set shaper  portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 queue=0 max-bw=1000000 max-burst=64000000



sleep 5



clx_diag netif destroy profile profile-id=0
clx_diag netif destroy profile profile-id=1


clx_diag pkt set monitor disable

clx_diag tm set shaper  portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 max-bw=0 max-burst=64000000
clx_diag tm set shaper  portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 queue=0 max-bw=0 max-burst=64000000

clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=100 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=102 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=104
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=106 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=108
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=110
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=112
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=114 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=116 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=118 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=120 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=122 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=124 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=126 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=128 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=130 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=132 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=134 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=136 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=138 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=140 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=142 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=144 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=146 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=148 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=150 
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=152
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=154
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=156
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=158
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=160
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=162
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=164
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=166
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=168
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=170
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=172
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=174
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=176
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=178
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=180
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=182
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=184
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=186
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=188
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=190
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=192
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=194

clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=196 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=1104 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=1112 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=1120 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=1128 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=1136 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=1144 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=1152 





clx_diag vlan del member vid=100  portlist=0,2     untag-portlist=0,2  
clx_diag vlan del member vid=102  portlist=2,4       untag-portlist=2,4    
clx_diag vlan del member vid=104  portlist=4,6      untag-portlist=4,6   
clx_diag vlan del member vid=106  portlist=6,8     untag-portlist=6,8  
clx_diag vlan del member vid=108  portlist=8,10    untag-portlist=8,10 
clx_diag vlan del member vid=110  portlist=10,12    untag-portlist=10,12
clx_diag vlan del member vid=112  portlist=12,14   untag-portlist=12,14  
clx_diag vlan del member vid=114  portlist=14,16    untag-portlist=14,16 
clx_diag vlan del member vid=116  portlist=16,18    untag-portlist=16,18 
clx_diag vlan del member vid=118  portlist=18,20    untag-portlist=18,20 
clx_diag vlan del member vid=120  portlist=20,22    untag-portlist=20,22 
clx_diag vlan del member vid=122  portlist=22,24    untag-portlist=22,24 
clx_diag vlan del member vid=124  portlist=24,26    untag-portlist=24,26 
clx_diag vlan del member vid=126  portlist=26,28    untag-portlist=26,28 
clx_diag vlan del member vid=128  portlist=28,30    untag-portlist=28,30 
clx_diag vlan del member vid=130  portlist=30,32    untag-portlist=30,32 
clx_diag vlan del member vid=132  portlist=32,34    untag-portlist=32,34 
clx_diag vlan del member vid=134  portlist=34,36    untag-portlist=34,36 
clx_diag vlan del member vid=136  portlist=36,38    untag-portlist=36,38 
clx_diag vlan del member vid=138  portlist=38,40    untag-portlist=38,40
clx_diag vlan del member vid=140  portlist=40,42    untag-portlist=40,42 
clx_diag vlan del member vid=142  portlist=42,44    untag-portlist=42,44 
clx_diag vlan del member vid=144  portlist=44,46    untag-portlist=44,46 
clx_diag vlan del member vid=146  portlist=46,48    untag-portlist=46,48 
clx_diag vlan del member vid=148  portlist=48,50    untag-portlist=48,50 
clx_diag vlan del member vid=150  portlist=50,52    untag-portlist=50,52 
clx_diag vlan del member vid=152  portlist=52,54    untag-portlist=52,54
clx_diag vlan del member vid=154 portlist=54,56    untag-portlist=54,56 
clx_diag vlan del member vid=156 portlist=56,58    untag-portlist=56,58
clx_diag vlan del member vid=158 portlist=58,60    untag-portlist=58,60
clx_diag vlan del member vid=160 portlist=60,62    untag-portlist=60,62
clx_diag vlan del member vid=162 portlist=62,64    untag-portlist=62,64
clx_diag vlan del member vid=164 portlist=64,66    untag-portlist=64,66
clx_diag vlan del member vid=166 portlist=66,68    untag-portlist=66,68
clx_diag vlan del member vid=168 portlist=68,70    untag-portlist=68,70
clx_diag vlan del member vid=170 portlist=70,72    untag-portlist=70,72
clx_diag vlan del member vid=172 portlist=72,74    untag-portlist=72,74
clx_diag vlan del member vid=174 portlist=74,76    untag-portlist=74,76
clx_diag vlan del member vid=176 portlist=76,78    untag-portlist=76,78
clx_diag vlan del member vid=178 portlist=78,80    untag-portlist=78,80
clx_diag vlan del member vid=180 portlist=80,82    untag-portlist=80,82
clx_diag vlan del member vid=182 portlist=82,84    untag-portlist=82,84
clx_diag vlan del member vid=184 portlist=84,86    untag-portlist=84,86
clx_diag vlan del member vid=186 portlist=86,88    untag-portlist=86,88
clx_diag vlan del member vid=188 portlist=88,90    untag-portlist=88,90
clx_diag vlan del member vid=190 portlist=90,92    untag-portlist=90,92
clx_diag vlan del member vid=192 portlist=92,94    untag-portlist=92,94
clx_diag vlan del member vid=194 portlist=94,0     untag-portlist=94,0 


clx_diag vlan del member vid=196 portlist=96,104 untag-portlist=96,104
clx_diag vlan del member vid=1104 portlist=104,112 untag-portlist=104,112
clx_diag vlan del member vid=1112 portlist=112,120 untag-portlist=112,120
clx_diag vlan del member vid=1120 portlist=120,128 untag-portlist=120,128
clx_diag vlan del member vid=1128 portlist=128,136 untag-portlist=128,136
clx_diag vlan del member vid=1136 portlist=136,144 untag-portlist=136,144
clx_diag vlan del member vid=1144 portlist=144,152 untag-portlist=144,152
clx_diag vlan del member vid=1152 portlist=152,96 untag-portlist=152,96



clx_diag port set intf-property portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 mtu=1536

clx_diag port set intf-property portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 pvid=1

clx_diag vlan add member vid=1 portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152 untag-portlist=0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,104,112,120,128,136,144,152





clx_diag stat show unit=0 portlist=0 > 0.txt 
clx_diag stat show unit=0 portlist=2 > 2.txt 
clx_diag stat show unit=0 portlist=4 > 4.txt 
clx_diag stat show unit=0 portlist=6 > 6.txt 
clx_diag stat show unit=0 portlist=8 > 8.txt 
clx_diag stat show unit=0 portlist=10 > 10.txt 
clx_diag stat show unit=0 portlist=12 > 12.txt 
clx_diag stat show unit=0 portlist=14 > 14.txt 
clx_diag stat show unit=0 portlist=16 > 16.txt 
clx_diag stat show unit=0 portlist=18 > 18.txt 
clx_diag stat show unit=0 portlist=20 > 20.txt 
clx_diag stat show unit=0 portlist=22 > 22.txt 
clx_diag stat show unit=0 portlist=24 > 24.txt 
clx_diag stat show unit=0 portlist=26 > 26.txt 
clx_diag stat show unit=0 portlist=28 > 28.txt 
clx_diag stat show unit=0 portlist=30 > 30.txt 
clx_diag stat show unit=0 portlist=32 > 32.txt 
clx_diag stat show unit=0 portlist=34 > 34.txt 
clx_diag stat show unit=0 portlist=36 > 36.txt 
clx_diag stat show unit=0 portlist=38 > 38.txt 
clx_diag stat show unit=0 portlist=40 > 40.txt 
clx_diag stat show unit=0 portlist=42 > 42.txt 
clx_diag stat show unit=0 portlist=44 > 44.txt 
clx_diag stat show unit=0 portlist=46 > 46.txt 
clx_diag stat show unit=0 portlist=48 > 48.txt 
clx_diag stat show unit=0 portlist=50 > 50.txt 
clx_diag stat show unit=0 portlist=52 > 52.txt 
clx_diag stat show unit=0 portlist=54 > 54.txt 
clx_diag stat show unit=0 portlist=56 > 56.txt 
clx_diag stat show unit=0 portlist=58 > 58.txt 
clx_diag stat show unit=0 portlist=60 > 60.txt 
clx_diag stat show unit=0 portlist=62 > 62.txt 
clx_diag stat show unit=0 portlist=64 > 64.txt 
clx_diag stat show unit=0 portlist=66 > 66.txt 
clx_diag stat show unit=0 portlist=68 > 68.txt 
clx_diag stat show unit=0 portlist=70 > 70.txt 
clx_diag stat show unit=0 portlist=72 > 72.txt 
clx_diag stat show unit=0 portlist=74 > 74.txt 
clx_diag stat show unit=0 portlist=76 > 76.txt 
clx_diag stat show unit=0 portlist=78 > 78.txt 
clx_diag stat show unit=0 portlist=80 > 80.txt 
clx_diag stat show unit=0 portlist=82 > 82.txt 
clx_diag stat show unit=0 portlist=84 > 84.txt 
clx_diag stat show unit=0 portlist=86 > 86.txt 
clx_diag stat show unit=0 portlist=88 > 88.txt 
clx_diag stat show unit=0 portlist=90 > 90.txt 
clx_diag stat show unit=0 portlist=92 > 92.txt 
clx_diag stat show unit=0 portlist=94 > 94.txt 
clx_diag stat show unit=0 portlist=96 > 96.txt 
clx_diag stat show unit=0 portlist=104 > 104.txt 
clx_diag stat show unit=0 portlist=112 > 112.txt 
clx_diag stat show unit=0 portlist=120 > 120.txt 
clx_diag stat show unit=0 portlist=128 > 128.txt 
clx_diag stat show unit=0 portlist=136 > 136.txt 
clx_diag stat show unit=0 portlist=144 > 144.txt 
clx_diag stat show unit=0 portlist=152 > 152.txt 
