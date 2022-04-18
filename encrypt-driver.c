#include <stdio.h>
#include "encrypt-module.h"

void reset_requested() {
	log_counts();
}

void reset_finished() {
}

int main(int argc, char *argv[]) {
  if(argc < 3)
  {
    printf("Please include input, output, and log filenames as arguments\n");
    return 0;
  }
  //printf("%s\n%s\n%s\n", argv[1], argv[2], argv[3]);
	init(argv[1], argv[2], argv[3]); 
    printf("Size of input buffer N:\n");
    int n = -1;
    int m = -1;
    scanf("%d", &n);
    printf("Size of input buffer M:\n");
    scanf("%d", &m);
    printf("N:%d\nM:%d\n", n, m);
	char c;
	while ((c = read_input()) != EOF) { 
		count_input(c); 
		c = encrypt(c); 
		count_output(c); 
		write_output(c); 
	} 
	printf("End of file reached.\n"); 
	log_counts();
}
