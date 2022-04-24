#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
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
  char item = input_buffer[input_head++];
  input_head %= input_size;
  return item;
}

char input_peek(char *input_buffer) {
  return input_buffer[input_head];
}

void output_put(char *output_buffer, char item) {
  output_buffer[output_tail++]=item;
  output_tail %= output_size;
}

char output_get(char *output_buffer) {
  char item = output_buffer[output_head++];
  output_head %= output_size;
  return item;
}

char output_peek(char *output_buffer) {
  return output_buffer[output_head];
}

// circular buffer end

void reset_requested() {
  log_counts();
}

void reset_finished() {
}

void* reader(char *input_buffer) {
  printf("reader\n");
  sem_wait(&sem_input_empty);
  sem_wait(&sem_input_mutex);
  char c = read_input();
  input_put(input_buffer, c);
  sem_post(&sem_input_mutex);
  sem_post(&sem_input_full);
}

void* input_counter(char *input_buffer) {
  printf("input_counter\n");
  sem_wait(&sem_input_mutex);
  count_input(input_peek(input_buffer));
  sem_post(&sem_input_mutex);
}

void* encryption(char *output_buffer, char *input_buffer) {
  printf("encrypt\n");
  sem_wait(&sem_input_full);
  sem_wait(&sem_output_empty);
  sem_wait(&sem_input_mutex);
  sem_wait(&sem_output_mutex);
  char c = encrypt(input_get(input_buffer));
  output_put(output_buffer, c); 
  sem_post(&sem_input_mutex);
  sem_post(&sem_output_mutex);
  sem_post(&sem_input_empty);
  sem_post(&sem_output_full);
}

void* output_counter(char *output_buffer) {
  printf("output_counter\n");
  sem_wait(&sem_output_mutex);
  count_output(output_peek(output_buffer)); 
  sem_post(&sem_output_mutex);
}

void* writer(char *output_buffer) {
  printf("write\n");
  sem_wait(&sem_output_full);
  sem_wait(&sem_output_mutex);
  char c = output_get(output_buffer);
  write_output(c);
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

  sem_t sem_space_input_encrypt;
  sem_t sem_space_input_count;
  sem_t sem_space_output_count;
  sem_t sem_space_output_writer;
  sem_t sem_work_input_count;
  sem_t sem_work_encrypt;
  sem_t sem_work_output_count;
  sem_t sem_work_write;
  sem_t sem_input_mutex;
  sem_t sem_output_mutex;

  // initialize space as maximum size
  sem_init(&sem_space_input_encrypt, 0, input_size);
  sem_init(&sem_space_input_count, 0, input_size);
  sem_init(&sem_space_output_count, 0, output_size);
  sem_init(&sem_space_output_writer, 0, output_size);

  // initialize work as 0 size
  sem_init(&sem_work_input_count, 0, 0);
  sem_init(&sem_work_encrypt, 0, 0);
  sem_init(&sem_work_output_count, 0, 0);
  sem_init(&sem_work_write, 0, 0);

  // initialize buffer mutexes
  sem_init(&sem_input_mutex, 0, 1);
  sem_init(&sem_output_mutex, 0, 1);

  // initalize 5 threads
  pthread_t reader_t;
  pthread_t count_in_t;
  pthread_t encrypter_t;
  pthread_t count_out_t;
  pthread_t writer_t;
  pthread_create(&reader_t,NULL,reader(input_buffer),NULL);
  pthread_create(&count_in_t,NULL,input_counter(input_buffer),NULL);
  pthread_create(&encrypter_t,NULL,encryption(output_buffer, input_buffer),NULL);
  pthread_create(&count_out_t,NULL,output_counter(output_buffer),NULL);
  pthread_create(&writer_t,NULL,writer(output_buffer),NULL);

  printf("End of file reached.\n"); 
  log_counts();
}

