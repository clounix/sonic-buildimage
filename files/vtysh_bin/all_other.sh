clx_diag vlan del member vid=1 portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 untag-portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76


clx_diag vlan del member vid=1000 portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 untag-portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76

clx_diag port set intf-property portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 mtu=9100



clx_diag port set intf-property portlist=0 pvid=100
sleep 1
clx_diag port set intf-property portlist=1 pvid=101
sleep 1
clx_diag port set intf-property portlist=2 pvid=102
sleep 1
clx_diag port set intf-property portlist=3 pvid=103
sleep 1
clx_diag port set intf-property portlist=4 pvid=104
clx_diag port set intf-property portlist=5 pvid=105
clx_diag port set intf-property portlist=6 pvid=106
clx_diag port set intf-property portlist=7 pvid=107
clx_diag port set intf-property portlist=8 pvid=108
clx_diag port set intf-property portlist=9 pvid=109
clx_diag port set intf-property portlist=10 pvid=110
clx_diag port set intf-property portlist=11 pvid=111
clx_diag port set intf-property portlist=12 pvid=112
clx_diag port set intf-property portlist=13 pvid=113
clx_diag port set intf-property portlist=14 pvid=114
clx_diag port set intf-property portlist=15 pvid=115
clx_diag port set intf-property portlist=16 pvid=116
clx_diag port set intf-property portlist=17 pvid=117
clx_diag port set intf-property portlist=18 pvid=118
clx_diag port set intf-property portlist=19 pvid=119
clx_diag port set intf-property portlist=20 pvid=120
clx_diag port set intf-property portlist=21 pvid=121
clx_diag port set intf-property portlist=22 pvid=122
clx_diag port set intf-property portlist=23 pvid=123
clx_diag port set intf-property portlist=24 pvid=124
clx_diag port set intf-property portlist=25 pvid=125
clx_diag port set intf-property portlist=26 pvid=126
clx_diag port set intf-property portlist=27 pvid=127
clx_diag port set intf-property portlist=28 pvid=128
clx_diag port set intf-property portlist=29 pvid=129
clx_diag port set intf-property portlist=30 pvid=130
clx_diag port set intf-property portlist=31 pvid=131
clx_diag port set intf-property portlist=32 pvid=132
clx_diag port set intf-property portlist=33 pvid=133
clx_diag port set intf-property portlist=34 pvid=134
clx_diag port set intf-property portlist=35 pvid=135
clx_diag port set intf-property portlist=36 pvid=136
clx_diag port set intf-property portlist=37 pvid=137
clx_diag port set intf-property portlist=38 pvid=138
clx_diag port set intf-property portlist=39 pvid=139
clx_diag port set intf-property portlist=40 pvid=140
clx_diag port set intf-property portlist=41 pvid=141
clx_diag port set intf-property portlist=42 pvid=142
clx_diag port set intf-property portlist=43 pvid=143
clx_diag port set intf-property portlist=44 pvid=144
clx_diag port set intf-property portlist=45 pvid=145
clx_diag port set intf-property portlist=46 pvid=146
clx_diag port set intf-property portlist=47 pvid=147

clx_diag port set intf-property portlist=48 pvid=148
clx_diag port set intf-property portlist=52 pvid=152
clx_diag port set intf-property portlist=56 pvid=156
clx_diag port set intf-property portlist=60 pvid=160
clx_diag port set intf-property portlist=64 pvid=164
clx_diag port set intf-property portlist=68 pvid=168
clx_diag port set intf-property portlist=72 pvid=172
clx_diag port set intf-property portlist=76 pvid=176




clx_diag vlan add member vid=100  portlist=0,1     untag-portlist=0,1  
clx_diag vlan add member vid=101  portlist=1,2       untag-portlist=1,2    
clx_diag vlan add member vid=102  portlist=2,3      untag-portlist=2,3   
clx_diag vlan add member vid=103  portlist=3,4     untag-portlist=3,4  
clx_diag vlan add member vid=104  portlist=4,5    untag-portlist=4,5 
clx_diag vlan add member vid=105  portlist=5,6   untag-portlist=5,6
clx_diag vlan add member vid=106  portlist=6,7   untag-portlist=6,7  
clx_diag vlan add member vid=107  portlist=7,8    untag-portlist=7,8  
clx_diag vlan add member vid=108  portlist=8,9    untag-portlist=8,9 
clx_diag vlan add member vid=109  portlist=9,10    untag-portlist=9,10 
clx_diag vlan add member vid=110  portlist=10,11    untag-portlist=10,11 
clx_diag vlan add member vid=111  portlist=11,12    untag-portlist=11,12 
clx_diag vlan add member vid=112  portlist=12,13    untag-portlist=12,13 
clx_diag vlan add member vid=113  portlist=13,14    untag-portlist=13,14 
clx_diag vlan add member vid=114  portlist=14,15    untag-portlist=14,15 
clx_diag vlan add member vid=115  portlist=15,16    untag-portlist=15,16 
clx_diag vlan add member vid=116  portlist=16,17    untag-portlist=16,17 
clx_diag vlan add member vid=117  portlist=17,18    untag-portlist=17,18 
clx_diag vlan add member vid=118  portlist=18,19   untag-portlist=18,19 
clx_diag vlan add member vid=119  portlist=19,20    untag-portlist=19,20
clx_diag vlan add member vid=120  portlist=20,21   untag-portlist=20,21 
clx_diag vlan add member vid=121  portlist=21,22    untag-portlist=21,22
clx_diag vlan add member vid=122  portlist=22,23    untag-portlist=22,23  
clx_diag vlan add member vid=123  portlist=23,24    untag-portlist=23,24
clx_diag vlan add member vid=124  portlist=24,25    untag-portlist=24,25 
clx_diag vlan add member vid=125  portlist=25,26    untag-portlist=25,26 
clx_diag vlan add member vid=126  portlist=26,27   untag-portlist=26,27
clx_diag vlan add member vid=127 portlist=27,28    untag-portlist=27,28 
clx_diag vlan add member vid=128 portlist=28,29    untag-portlist=28,29
clx_diag vlan add member vid=129 portlist=29,30    untag-portlist=29,30
clx_diag vlan add member vid=130 portlist=30,31    untag-portlist=30,31
clx_diag vlan add member vid=131 portlist=31,32    untag-portlist=31,32
clx_diag vlan add member vid=132 portlist=32,33    untag-portlist=32,33
clx_diag vlan add member vid=133 portlist=33,34    untag-portlist=33,34
clx_diag vlan add member vid=134 portlist=34,35    untag-portlist=34,35
clx_diag vlan add member vid=135 portlist=35,36    untag-portlist=35,36
clx_diag vlan add member vid=136 portlist=36,37    untag-portlist=36,37
clx_diag vlan add member vid=137 portlist=37,38    untag-portlist=37,38
clx_diag vlan add member vid=138 portlist=38,39    untag-portlist=38,39
clx_diag vlan add member vid=139 portlist=39,40    untag-portlist=39,40
clx_diag vlan add member vid=140 portlist=40,41    untag-portlist=40,41
clx_diag vlan add member vid=141 portlist=41,42    untag-portlist=41,42
clx_diag vlan add member vid=142 portlist=42,43    untag-portlist=42,43
clx_diag vlan add member vid=143 portlist=43,44    untag-portlist=43,44
clx_diag vlan add member vid=144 portlist=44,45    untag-portlist=44,45
clx_diag vlan add member vid=145 portlist=45,46    untag-portlist=45,46
clx_diag vlan add member vid=146 portlist=46,47    untag-portlist=46,47
clx_diag vlan add member vid=147 portlist=47,0    untag-portlist=47,0 



clx_diag vlan add member vid=148 portlist=48,52 untag-portlist=48,52
clx_diag vlan add member vid=152 portlist=52,56 untag-portlist=52,56
clx_diag vlan add member vid=156 portlist=56,60 untag-portlist=56,60
clx_diag vlan add member vid=160 portlist=60,64 untag-portlist=60,64
clx_diag vlan add member vid=164 portlist=64,68 untag-portlist=64,68
clx_diag vlan add member vid=168 portlist=68,72 untag-portlist=68,72
clx_diag vlan add member vid=172 portlist=72,76 untag-portlist=72,76
clx_diag vlan add member vid=176 portlist=76,48 untag-portlist=76,48


clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=100  port=1
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=101  port=2
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=102  port=3
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=103  port=4
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=104  port=5
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=105  port=6
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=106  port=7
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=107  port=8
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=108  port=9
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=109  port=10
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=110  port=11
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=111  port=12
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=112  port=13
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=113  port=14
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=114  port=15
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=115  port=16
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=116  port=17
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=117  port=18
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=118  port=19
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=119  port=20
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=120  port=21
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=121  port=22
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=122  port=23
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=123  port=24
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=124  port=25
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=125 port=26
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=126 port=27
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=127 port=28
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=128 port=29
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=129 port=30
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=130 port=31
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=131 port=32
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=132 port=33
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=133 port=34
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=134 port=35
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=135 port=36
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=136 port=37
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=137 port=38
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=138 port=39
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=139 port=40
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=140 port=41
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=141 port=42
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=142 port=43
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=143 port=44
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=144 port=45
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=145 port=46
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=146 port=47
clx_diag l2 add mac-table static mac=00:00:00:00:00:01 vid=147 port=0

clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=148 port=52
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=152 port=56
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=156 port=60
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=160 port=64
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=164 port=68
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=168 port=72
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=172 port=76
clx_diag l2 add mac-table static mac=00:00:00:00:00:02 vid=176 port=48



clx_diag pkt set sequence clear
clx_diag pkt set monitor sequence
clx_diag pkt clear monitor

clx_diag pkt send tx portlist=0   len=1028 num=2000 dmac=00:00:00:00:00:01 smac=00:00:00:01:00:01 payload=0xffff sequence
clx_diag pkt send tx portlist=48  len=1028 num=2000 dmac=00:00:00:00:00:02 smac=00:00:00:01:00:01 payload=0xffff sequence




clx_diag tm set shaper  portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 max-bw=1000000 max-burst=64000000
clx_diag tm set shaper  portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 queue=0 max-bw=1000000 max-burst=64000000



sleep 5










clx_diag netif destroy profile profile-id=0
clx_diag netif destroy profile profile-id=1


clx_diag pkt set monitor disable

clx_diag tm set shaper  portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 max-bw=0 max-burst=64000000
clx_diag tm set shaper  portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 queue=0 max-bw=0 max-burst=64000000

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
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=132
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=133
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=134
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=135
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=136
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=137
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=138
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=139
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=140
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=141
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=142
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=143
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=144
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=145
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=146
clx_diag l2 del mac-table mac=00:00:00:00:00:01 vid=147

clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=148 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=152 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=156 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=160 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=164 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=168 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=172 
clx_diag l2 del mac-table mac=00:00:00:00:00:02 vid=176 


 

clx_diag vlan del member vid=100  portlist=0,1     untag-portlist=0,1  
clx_diag vlan del member vid=101  portlist=1,2       untag-portlist=1,2    
clx_diag vlan del member vid=102  portlist=2,3      untag-portlist=2,3   
clx_diag vlan del member vid=103  portlist=3,4     untag-portlist=3,4  
clx_diag vlan del member vid=104  portlist=4,5    untag-portlist=4,5 
clx_diag vlan del member vid=105  portlist=5,6   untag-portlist=5,6
clx_diag vlan del member vid=106  portlist=6,7   untag-portlist=6,7  
clx_diag vlan del member vid=107  portlist=7,8    untag-portlist=7,8  
clx_diag vlan del member vid=108  portlist=8,9    untag-portlist=8,9 
clx_diag vlan del member vid=109  portlist=9,10    untag-portlist=9,10 
clx_diag vlan del member vid=110  portlist=10,11    untag-portlist=10,11 
clx_diag vlan del member vid=111  portlist=11,12    untag-portlist=11,12 
clx_diag vlan del member vid=112  portlist=12,13    untag-portlist=12,13 
clx_diag vlan del member vid=113  portlist=13,14    untag-portlist=13,14 
clx_diag vlan del member vid=114  portlist=14,15    untag-portlist=14,15 
clx_diag vlan del member vid=115  portlist=15,16    untag-portlist=15,16 
clx_diag vlan del member vid=116  portlist=16,17    untag-portlist=16,17 
clx_diag vlan del member vid=117  portlist=17,18    untag-portlist=17,18 
clx_diag vlan del member vid=118  portlist=18,19   untag-portlist=18,19 
clx_diag vlan del member vid=119  portlist=19,20    untag-portlist=19,20
clx_diag vlan del member vid=120  portlist=20,21   untag-portlist=20,21 
clx_diag vlan del member vid=121  portlist=21,22    untag-portlist=21,22
clx_diag vlan del member vid=122  portlist=22,23    untag-portlist=22,23  
clx_diag vlan del member vid=123  portlist=23,24    untag-portlist=23,24
clx_diag vlan del member vid=124  portlist=24,25    untag-portlist=24,25 
clx_diag vlan del member vid=125  portlist=25,26    untag-portlist=25,26 
clx_diag vlan del member vid=126  portlist=26,27   untag-portlist=26,27
clx_diag vlan del member vid=127 portlist=27,28    untag-portlist=27,28 
clx_diag vlan del member vid=128 portlist=28,29    untag-portlist=28,29
clx_diag vlan del member vid=129 portlist=29,30    untag-portlist=29,30
clx_diag vlan del member vid=130 portlist=30,31    untag-portlist=30,31
clx_diag vlan del member vid=131 portlist=31,32    untag-portlist=31,32
clx_diag vlan del member vid=132 portlist=32,33    untag-portlist=32,33
clx_diag vlan del member vid=133 portlist=33,34    untag-portlist=33,34
clx_diag vlan del member vid=134 portlist=34,35    untag-portlist=34,35
clx_diag vlan del member vid=135 portlist=35,36    untag-portlist=35,36
clx_diag vlan del member vid=136 portlist=36,37    untag-portlist=36,37
clx_diag vlan del member vid=137 portlist=37,38    untag-portlist=37,38
clx_diag vlan del member vid=138 portlist=38,39    untag-portlist=38,39
clx_diag vlan del member vid=139 portlist=39,40    untag-portlist=39,40
clx_diag vlan del member vid=140 portlist=40,41    untag-portlist=40,41
clx_diag vlan del member vid=141 portlist=41,42    untag-portlist=41,42
clx_diag vlan del member vid=142 portlist=42,43    untag-portlist=42,43
clx_diag vlan del member vid=143 portlist=43,44    untag-portlist=43,44
clx_diag vlan del member vid=144 portlist=44,45    untag-portlist=44,45
clx_diag vlan del member vid=145 portlist=45,46    untag-portlist=45,46
clx_diag vlan del member vid=146 portlist=46,47    untag-portlist=46,47
clx_diag vlan del member vid=147 portlist=47,0    untag-portlist=47,0 



clx_diag vlan del member vid=148 portlist=48,52 untag-portlist=48,52
clx_diag vlan del member vid=152 portlist=52,56 untag-portlist=52,56
clx_diag vlan del member vid=156 portlist=56,60 untag-portlist=56,60
clx_diag vlan del member vid=160 portlist=60,64 untag-portlist=60,64
clx_diag vlan del member vid=164 portlist=64,68 untag-portlist=64,68
clx_diag vlan del member vid=168 portlist=68,72 untag-portlist=68,72
clx_diag vlan del member vid=172 portlist=72,76 untag-portlist=72,76
clx_diag vlan del member vid=176 portlist=76,48 untag-portlist=76,48


clx_diag port set intf-property portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 mtu=1536

clx_diag port set intf-property portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 pvid=1

clx_diag vlan add member vid=1 portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 untag-portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76

clx_diag vlan add member vid=1000 portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76 untag-portlist=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,52,56,60,64,68,72,76

clx_diag stat show unit=0 portlist=0 > 0.txt 
clx_diag stat show unit=0 portlist=1 > 1.txt 
clx_diag stat show unit=0 portlist=2 > 2.txt 
clx_diag stat show unit=0 portlist=3 > 3.txt 
clx_diag stat show unit=0 portlist=4 > 4.txt 
clx_diag stat show unit=0 portlist=5 > 5.txt 
clx_diag stat show unit=0 portlist=6 > 6.txt 
clx_diag stat show unit=0 portlist=7 > 7.txt 
clx_diag stat show unit=0 portlist=8 > 8.txt 
clx_diag stat show unit=0 portlist=9 > 9.txt 
clx_diag stat show unit=0 portlist=10 > 10.txt 
clx_diag stat show unit=0 portlist=11 > 11.txt 
clx_diag stat show unit=0 portlist=12 > 12.txt 
clx_diag stat show unit=0 portlist=13 > 13.txt 
clx_diag stat show unit=0 portlist=14 > 14.txt 
clx_diag stat show unit=0 portlist=15 > 15.txt 
clx_diag stat show unit=0 portlist=16 > 16.txt 
clx_diag stat show unit=0 portlist=17 > 17.txt 
clx_diag stat show unit=0 portlist=18 > 18.txt 
clx_diag stat show unit=0 portlist=19 > 19.txt 
clx_diag stat show unit=0 portlist=20 > 20.txt 
clx_diag stat show unit=0 portlist=21 > 21.txt 
clx_diag stat show unit=0 portlist=22 > 22.txt 
clx_diag stat show unit=0 portlist=23 > 23.txt 
clx_diag stat show unit=0 portlist=24 > 24.txt 
clx_diag stat show unit=0 portlist=25 > 25.txt 
clx_diag stat show unit=0 portlist=26 > 26.txt 
clx_diag stat show unit=0 portlist=27 > 27.txt 
clx_diag stat show unit=0 portlist=28 > 28.txt 
clx_diag stat show unit=0 portlist=29 > 29.txt 
clx_diag stat show unit=0 portlist=30 > 30.txt 
clx_diag stat show unit=0 portlist=31 > 31.txt 
clx_diag stat show unit=0 portlist=32 > 32.txt 
clx_diag stat show unit=0 portlist=33 > 33.txt 
clx_diag stat show unit=0 portlist=34 > 34.txt 
clx_diag stat show unit=0 portlist=35 > 35.txt 
clx_diag stat show unit=0 portlist=36 > 36.txt 
clx_diag stat show unit=0 portlist=37 > 37.txt 
clx_diag stat show unit=0 portlist=38 > 38.txt 
clx_diag stat show unit=0 portlist=39 > 39.txt 
clx_diag stat show unit=0 portlist=40 > 40.txt 
clx_diag stat show unit=0 portlist=41 > 41.txt 
clx_diag stat show unit=0 portlist=42 > 42.txt 
clx_diag stat show unit=0 portlist=43 > 43.txt 
clx_diag stat show unit=0 portlist=44 > 44.txt 
clx_diag stat show unit=0 portlist=45 > 45.txt 
clx_diag stat show unit=0 portlist=46 > 46.txt 
clx_diag stat show unit=0 portlist=47 > 47.txt 
clx_diag stat show unit=0 portlist=48 > 48.txt 
clx_diag stat show unit=0 portlist=52 > 52.txt 
clx_diag stat show unit=0 portlist=56 > 56.txt 
clx_diag stat show unit=0 portlist=60 > 60.txt 
clx_diag stat show unit=0 portlist=64 > 64.txt 
clx_diag stat show unit=0 portlist=68 > 68.txt 
clx_diag stat show unit=0 portlist=72 > 72.txt 
clx_diag stat show unit=0 portlist=76 > 76.txt 

