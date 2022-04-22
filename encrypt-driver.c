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
  printf("input tail: %d\n", input_tail);
  input_buffer[input_tail++]=item;
  input_tail %= input_size;
}

char input_get(char *input_buffer) {
  printf("input head: %d\n", input_head);
  char item = input_buffer[input_head++];
  input_head %= input_size;
  return item;
}

char input_peek(char *input_buffer) {
  return input_buffer[input_head];
}

void output_put(char *output_buffer, char item) {
  printf("output tail: %d\n", output_tail);
  output_buffer[output_tail++]=item;
  output_tail %= output_size;
}

char output_get(char *output_buffer) {
  printf("output head: %d\n", output_head);
  char item = output_buffer[output_head++];
  output_head %= output_size;
  return item;
}

char output_peek(char *output_buffer) {
  return output_buffer[output_head];
}


void reset_requested() {
  log_counts();
}

void reset_finished() {
}

int reader(char *input_buffer) {
  sem_wait(&sem_input_empty);
  sem_wait(&sem_input_mutex);
  char c = read_input();
  input_put(input_buffer, c);
  printf("reader: %c\n", input_peek(input_buffer));
  sem_post(&sem_input_mutex);
  sem_post(&sem_input_full);
  return c == EOF ? 1 : 0;
}

int input_counter(char *input_buffer, int *i) {
  sem_wait(&sem_input_mutex);
  printf("input++\n");
  *i++;
  count_input(input_peek(input_buffer));
  sem_post(&sem_input_mutex);
}

void encryption(char *output_buffer, char *input_buffer) {
  sem_wait(&sem_input_full);
  printf("input_full\n");
  sem_wait(&sem_output_empty);
  printf("output_empty\n");
  sem_wait(&sem_input_mutex);
  printf("input_mutex\n");
  sem_wait(&sem_output_mutex);
  printf("output_mutex\n");
  char c = encrypt(input_get(input_buffer));
  printf("encryptor: %c\n", c);
  output_put(output_buffer, c); 
  sem_post(&sem_input_mutex);
  sem_post(&sem_output_mutex);
  sem_post(&sem_input_empty);
  sem_post(&sem_output_full);
}

int output_counter(char *output_buffer, int *i) {
  sem_wait(&sem_output_mutex);
  printf("output++\n");
  *i--;
  count_output(output_peek(output_buffer)); 
  sem_post(&sem_output_mutex);
}

void writer(char *output_buffer) {
  sem_wait(&sem_output_full);
  sem_wait(&sem_output_mutex);
  char c = output_get(output_buffer);
  write_output(c);
  printf("writer: %c\n", output_peek(output_buffer));
  sem_post(&sem_output_mutex);
  sem_post(&sem_output_empty); 
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
  sem_init(&sem_input_empty, 0, input_size);
  sem_init(&sem_input_full, 0, input_size);
  sem_init(&sem_input_mutex, 0, 1);
  sem_init(&sem_output_empty, 0, output_size);
  sem_init(&sem_output_full, 0, output_size);
  sem_init(&sem_output_mutex, 0, 1);
  int isEOF = 0; 
  int i = 0;
  while (isEOF != 1 || i != 0) { 
    isEOF = reader(input_buffer);
    input_counter(input_buffer, &i);
    encryption(output_buffer, input_buffer);
    output_counter(output_buffer, &i);
    writer(output_buffer);
  } 
  printf("End of file reached.\n"); 
  log_counts();
}

