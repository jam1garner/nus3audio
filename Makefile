all: nus3audio.exe

debug:
	gcc -g -o nus3audio source/main.c source/nus3audio.c	

clean:
	rm -f nus3audio nus3audio.exe

nus3audio.exe:
	gcc -o nus3audio source/main.c source/nus3audio.c