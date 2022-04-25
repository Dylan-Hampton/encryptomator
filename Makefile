default:
	gcc encrypt-driver.c encrypt-module.c -lpthread -o encrypt
proj:	
	clear;gcc encrypt-driver.c encrypt-module.c -lpthread -o encrypt;./encrypt in.txt out.txt log.txt
clean:
	rm -rf encrypt

