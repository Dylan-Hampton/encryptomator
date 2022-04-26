#include <stdio.h>
#include <stdlib.h>
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

char* input_buffer;
char* output_buffer;

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
sem_t sem_reader_mutex;

pthread_t reader_t;
pthread_t count_in_t;
pthread_t encrypter_t;
pthread_t count_out_t;
pthread_t writer_t;

// circular array code borrowed from https://en.wikipedia.org/wiki/Circular_buffer

void input_put(char item) {
  input_buffer[input_tail++]=item;
  input_tail %= input_size;
}

char input_get_count() {
  char item = input_buffer[input_head_count++];
  input_head_count %= input_size;
  return item;
}

char input_get_encrypt() {
  char item = input_buffer[input_head_encrypt++];
  input_head_encrypt %= input_size;
  return item;
}

void output_put(char item) {
  output_buffer[output_tail++]=item;
  output_tail %= output_size;
}

char output_get_count() {
  char item = output_buffer[output_head_count++];
  output_head_count %= output_size;
  return item;
}

char output_get_write() {
  char item = output_buffer[output_head_write++];
  output_head_write %= output_size;
  return item;
}

void print_buffer_input() {
  printf("ibuffer: \n");
  printf("isize: %d\n", input_size);
  for (int i = 0; i < input_size; i++) {
    printf("%d: %c \n", i, input_buffer[i]);
  }
  printf("\n");
}

void print_buffer_output() {
  printf("obuffer: \n");
  printf("osize: %d\n", output_size);
  for (int i = 0; i < output_size; i++) {
    printf("%d: %c \n", i, output_buffer[i]);
  }
  printf("\n");
}

// circular buffer end

int can_reset() {
  int val1, val2, val3, val4;
  int ret = 0;
  sem_getvalue(&sem_work_input_count, &val1);
  sem_getvalue(&sem_work_encrypt, &val2);
  sem_getvalue(&sem_work_output_count, &val3);
  sem_getvalue(&sem_work_write, &val4);
  sem_wait(&sem_input_mutex);
  sem_wait(&sem_output_mutex);

  if (   val1 == 0
      && val2 == 0
      && val3 == 0
      && val4 == 0 
      && get_input_total_count()
      == get_output_total_count() 
     ) {
    ret = 1;
  }
  sem_post(&sem_input_mutex);
  sem_post(&sem_output_mutex);
  return ret;
}

int has_ended() {
  int val1, val2, val3, val4, val5, val6;
  sem_getvalue(&sem_work_input_count, &val1);
  sem_getvalue(&sem_work_encrypt, &val2);
  sem_getvalue(&sem_work_output_count, &val3);
  sem_getvalue(&sem_work_write, &val4); 
  sem_getvalue(&sem_input_mutex, &val5);
  sem_getvalue(&sem_output_mutex, &val6);

  if (has_found_eof
      && val1 == 0
      && val2 == 0
      && val3 == 0
      && val4 == 0 
      && val5 == 1
      && val6 == 1
     ) {
    return 1;
  }
  return 0;
}

void reset_requested() {
  sem_wait(&sem_reader_mutex);
  while(!can_reset()){ }  
  log_counts(); 
}

void reset_finished() {
  sem_post(&sem_reader_mutex);
}

void* reader() {
  char c;
  while (!has_ended()) {
    while ((c = read_input()) != EOF) {
      sem_wait(&sem_space_input_count);
      sem_wait(&sem_space_input_encrypt);
      sem_wait(&sem_reader_mutex);
      sem_wait(&sem_input_mutex);
      input_put(c);
      sem_post(&sem_reader_mutex);
      sem_post(&sem_input_mutex);
      sem_post(&sem_work_encrypt);
      sem_post(&sem_work_input_count);
    }
    has_found_eof = 1;
  }
}

void* input_counter() {
  while (1) {
    sem_wait(&sem_work_input_count);
    sem_wait(&sem_input_mutex);
    char c = input_get_count();
    sem_post(&sem_input_mutex);
    count_input(c);
    sem_post(&sem_space_input_count);
  }
}

void* encryption() {
  while (1) {
    sem_wait(&sem_work_encrypt);
    sem_wait(&sem_input_mutex);
    char c = input_get_encrypt();
    sem_post(&sem_input_mutex);
    sem_post(&sem_space_input_encrypt);
    c = encrypt(c);
    sem_wait(&sem_space_output_count);
    sem_wait(&sem_space_output_writer);
    sem_wait(&sem_output_mutex);
    output_put(c);
    sem_post(&sem_output_mutex);
    sem_post(&sem_work_output_count);
    sem_post(&sem_work_write);
  }
}

void* output_counter() {
  while (1) {
    sem_wait(&sem_work_output_count);
    sem_wait(&sem_output_mutex);
    char c = output_get_count();
    sem_post(&sem_output_mutex);
    count_output(c);
    sem_post(&sem_space_output_count);
  }
}

void* writer() {
  while (1) {
    sem_wait(&sem_work_write);
    sem_wait(&sem_output_mutex);
    char c = output_get_write();
    sem_post(&sem_space_output_writer);
    sem_post(&sem_output_mutex);
    write_output(c);
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

  input_buffer = (char*) malloc(sizeof(char) * (input_size + 1));
  output_buffer = (char*) malloc(sizeof(char) * (output_size + 1));

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
  sem_init(&sem_reader_mutex, 0, 1);

  // initalize 5 threads
  pthread_create(&reader_t, NULL, reader, NULL);
  pthread_create(&count_in_t, NULL, input_counter, NULL);
  pthread_create(&encrypter_t, NULL, encryption, NULL);
  pthread_create(&count_out_t, NULL, output_counter, NULL);
  pthread_create(&writer_t, NULL, writer, NULL);

  // join threads at end
  pthread_join(reader_t, NULL);

  printf("End of file reached.\n");
  log_counts();
}

