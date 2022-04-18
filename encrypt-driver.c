#include <stdio.h>
#include "encrypt-module.h"

int input_head;
int input_tail;
int input_size;

int output_head;
int output_tail;
int output_size;

// circular array code borrowed from https://en.wikipedia.org/wiki/Circular_buffer

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
  int n = -1;
  int m = -1;
  while (m < 1 && n < 1) {
    printf("Size of input buffer N:\n");
    scanf("%d", &n);
    printf("Size of output buffer M:\n");
    scanf("%d", &m);
    if (m < 1 || n < 1) {
      printf("Buffer sizes must be > 1\n");
    }
  }
  int input_buffer[n];
  int output_buffer[m];
  input_head = 0;
  input_tail = 0;
  input_size = n;
  output_head = 0;
  output_tail = 0;
  output_size = m;
  //printf("N:%d\nM:%d\n", n, m);
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

void input_put(int *input_buffer, int item) {
  input_buffer[input_tail++]=item;
  input_tail %= input_size;
}

int input_get(int *input_buffer) {
  int item = input_buffer[input_head++];
  input_head %= input_size;
  return item;
}

void output_put(int *output_buffer, int item) {
  output_buffer[output_tail++]=item;
  output_tail %= output_size;
}

int output_get(int *output_buffer) {
  int item = output_buffer[output_head++];
  output_head %= output_size;
  return item;
}

