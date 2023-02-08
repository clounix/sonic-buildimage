cc=gcc
#target=main
target=cpld_update_app
obj= main.o aje2vec.o vec.o ajeutil.o decodel.o gpio.o lzw_lib.o gpio_come.o

$(target):$(obj)
	$(cc) $(obj) -O1 -Wall -o $(target) 
main.o:main.c
	$(cc) -O1 -c main.c
aje2vec.o:aje2vec.c
	$(cc) -O1 -c aje2vec.c
vec.o:vec.c
	$(cc) -O1 -c vec.c	
ajeutil.o:ajeutil.c
	$(cc) -O1 -c ajeutil.c
decodel.o:decodel.c
	$(cc) -O1 -c decodel.c
gpio.o:gpio.c
	$(cc) -O1 -c gpio.c
lzw_lib.o:lzw_lib.c
	$(cc) -O1 -c lzw_lib.c
gpio_come.o:gpio_come.c
	$(cc) -O1 -c gpio_come.c		
.PHONY:clean
clean:
	$(RM) *.o $(target)
#	rm *.o

