#include <stdio.h>
#include "encrypt-module.h"

int input_head = 0;
int input_tail = 0;
int input_size = -1;

int output_head = 0;
int output_tail = 0;
int output_size = -1;

// circular array code borrowed from https://en.wikipedia.org/wiki/Circular_buffer

void input_put(char *input_buffer, char item) {
  input_buffer[input_tail++]=item;
  input_tail %= input_size;
}

char input_get(char *input_buffer) {
  int item = input_buffer[input_head++];
  input_head %= input_size;
  return item;
}

void output_put(char *output_buffer, char item) {
  output_buffer[output_tail++]=item;
  output_tail %= output_size;
}

char output_get(char *output_buffer) {
  int item = output_buffer[output_head++];
  output_head %= output_size;
  return item;
}

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
  init(argv[1], argv[2], argv[3]); 
  while (input_size < 1 || output_size < 1) {
    printf("Size of input buffer N:\n");
    scanf("%d", &input_size);
    printf("Size of output buffer M:\n");
    scanf("%d", &output_size);
    if (input_size < 1 || output_size < 1) {
      printf("Buffer sizes must be > 1\n");
    }
  }
  char input_buffer[input_size];
  char output_buffer[output_size];
  //input_put(input_buffer, 5);
  //output_put(output_buffer, 4);
  //printf("%d\n", input_get(input_buffer));
  //printf("%d\n", output_get(output_buffer));
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

