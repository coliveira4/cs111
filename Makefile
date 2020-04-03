default:
	gcc -Wall -Wextra lab3a.c -o lab3a
dist:	default
	tar -czf lab3a-704540685.tar.gz lab3a.c ext2_fs.h Makefile README
clean:
	rm -f lab3a *.o *.txt *.tar.gz