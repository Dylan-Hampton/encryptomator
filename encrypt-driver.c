#include <stdio.h>
#include <semaphore.h>
#include "encrypt-module.h"

int input_head = 0;
int input_tail = 0;
int input_size = -1;

int output_head = 0;
int output_tail = 0;
int output_size = -1;

// buffer control semaphores
sem_t sem_input_empty;
sem_t sem_input_full;
sem_t sem_output_empty;
sem_t sem_output_full;
// producer consumer mutexs
sem_t sem_input_mutex;
sem_t sem_output_mutex;

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

void reader(char *input_buffer) {
  sem_wait(sem_input_mutex);
  sem_wait(sem_input_empty);
  input_put(input_buffer, read_input());
  sem_wait(sem_input_full);
  sem_post(sem_input_mutex);
}

void input_counter() {
  //count_input(c);
}

void encryption(char *output_buffer, char *input_buffer) {
  sem_wait(sem_input_full);
  sem_wait(sem_output_empty);
  sem_wait(sem_input_mutex);
  sem_wait(sem_output_mutex);
  output_put(output_buffer, encrypt(input_get(input_buffer))); 
  sem_post(sem_input_full);
  sem_post(sem_output_empty);
  sem_post(sem_input_mutex);
  sem_post(sem_output_mutex);
}

void output_counter() {
  //count_output(c); 
}

void writer(char *output_buffer) {
  write_output(output_get(output_buffer)); 
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
  char c;
  sem_init(&sem_input_empty, 0, input_size);
  sem_init(&sem_input_full, 0, input_size);
  sem_init(&sem_input_mutex, 0, 1);
  sem_init(&sem_output_empty, 0, output_size);
  sem_init(&sem_output_full, 0, output_size);
  sem_init(&sem_output_mutex, 0, 1);
  while ((c = read_input()) != EOF) { 
    reader(input_buffer);
    input_counter();
    encryption(output_buffer, input_buffer);
    output_counter();
    writer(output_buffer);
  } 
  printf("End of file reached.\n"); 
  log_counts();
}

