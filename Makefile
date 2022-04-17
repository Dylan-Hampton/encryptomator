default:
	gcc encrypt-driver.c encrypt-module.c -lpthread -o encrypt
clean:
	rm -rf encrypt

