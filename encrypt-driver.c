#include <stdio.h>
#include <semaphore.h>
#include <pthread.h>
#include "encrypt-module.h"

int has_found_eof = 0;

int input_head_count = 0;
int input_head_encrypt = 0;
int input_tail = 0;
int input_size = -1;

int output_head_count = 0;
int output_head_write = 0;
int output_tail = 0;
int output_size = -1;

sem_t sem_space_input_count;
sem_t sem_space_input_encrypt;
sem_t sem_space_output_count;
sem_t sem_space_output_writer;
sem_t sem_work_input_count;
sem_t sem_work_encrypt;
sem_t sem_work_output_count;
sem_t sem_work_write;
sem_t sem_input_mutex;
sem_t sem_output_mutex;

pthread_t reader_t;
pthread_t count_in_t;
pthread_t encrypter_t;
pthread_t count_out_t;
pthread_t writer_t;

// circular array code borrowed from https://en.wikipedia.org/wiki/Circular_buffer

void input_put(char *input_buffer, char item) {
  input_buffer[input_tail++]=item;
  input_tail %= input_size;
}

char input_get_count(char *input_buffer) {
  char item = input_buffer[input_head_count++];
  input_head_count %= input_size;
  return item;
}

char input_get_encrypt(char *input_buffer) {
  char item = input_buffer[input_head_encrypt++];
  input_head_encrypt %= input_size;
  return item;
}

void output_put(char *output_buffer, char item) {
  output_buffer[output_tail++]=item;
  output_tail %= output_size;
}

char output_get_count(char *output_buffer) {
  char item = output_buffer[output_head_count++];
  output_head_count %= output_size;
  return item;
}

char output_get_write(char *output_buffer) {
  char item = output_buffer[output_head_write++];
  output_head_write %= output_size;
  return item;
}

// circular buffer end

void reset_requested() {
  log_counts();
}

void reset_finished() {
}

int hasEnded() {
  printf("hello\n");
  int val1, val2, val3, val4;
  if (has_found_eof
      && sem_getvalue(&sem_work_input_count, &val1) == 0
      && sem_getvalue(&sem_work_encrypt, &val2) == 0
      && sem_getvalue(&sem_work_output_count, &val3) == 0
      && sem_getvalue(&sem_work_write, &val4) == 0
     ) {
    return 1;
  }
  return 0;
}

void* reader(char *input_buffer) {
  //input_put(input_buffer, c);
  char c;
  int i = 0;
  while (i < 10/*(c = read_input()) != EOF*/) {
    sem_wait(&sem_space_input_count);
    sem_wait(&sem_space_input_encrypt);
    sem_wait(&sem_input_mutex);
    printf("reader\n");
    i++;
    sem_post(&sem_input_mutex);
    sem_post(&sem_work_encrypt);
    sem_post(&sem_work_input_count);
  }
  has_found_eof = 1;
}

void* input_counter(char *input_buffer) {
  //count_input(c);
  while (!hasEnded()) {
    sem_wait(&sem_work_input_count);
    sem_wait(&sem_input_mutex);
    printf("input_counter\n");
    // read from input
    sem_post(&sem_input_mutex);
    // counts it
    sem_post(&sem_space_input_count);
  }
}

void* encryption(char *output_buffer, char *input_buffer) {
  // char c = encrypt(input_get(input_buffer));
  // output_put(output_buffer, c);
  while (!hasEnded()) {
    sem_wait(&sem_work_encrypt);
    sem_wait(&sem_input_mutex);
    printf("encrypt\n");
    // gets char from input
    sem_post(&sem_input_mutex);
    sem_post(&sem_space_input_encrypt);
    // encrypts char
    sem_wait(&sem_space_output_count);
    sem_wait(&sem_space_output_writer);
    sem_wait(&sem_output_mutex);
    // puts encrypted char into output buffer
    sem_post(&sem_output_mutex);
    sem_post(&sem_work_output_count);
    sem_post(&sem_work_write);
  }
}

void* output_counter(char *output_buffer) {
  // count_output(c);
  while (!hasEnded()) {
    sem_wait(&sem_work_output_count);
    sem_wait(&sem_output_mutex);
    printf("output_counter\n");
    // read from output
    sem_post(&sem_output_mutex);
    // counts it
    sem_post(&sem_space_output_count);
  }
}

void* writer(char *output_buffer) {
  // char c = output_get(output_buffer);
  // write_output(c);
  while (!hasEnded()) {
    sem_wait(&sem_work_write);
    sem_wait(&sem_output_mutex);
    printf("writer\n");
    // takes from output buffer and sends to page
    sem_post(&sem_output_mutex);
  }
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
  pthread_create(&reader_t, NULL, reader(input_buffer), NULL);
  pthread_create(&count_in_t, NULL, input_counter(input_buffer), NULL);
  pthread_create(&encrypter_t, NULL, encryption(output_buffer, input_buffer), NULL);
  pthread_create(&count_out_t, NULL, output_counter(output_buffer), NULL);
  pthread_create(&writer_t, NULL, writer(output_buffer), NULL);

  // join threads at end
  pthread_join(reader_t, NULL);
  pthread_join(count_in_t, NULL);
  pthread_join(encrypter_t, NULL);
  pthread_join(count_out_t, NULL);
  pthread_join(writer_t, NULL);

  printf("End of file reached.\n");
  log_counts();
}

