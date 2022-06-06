/*Group-09
Name:1)Maria Mehjabin Shenjuti      id:2018-1-60-244
     2)Sumaita Tanjim Hridy         id:2018-1-60-251

                     Burger buddies problem

*/


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>

#define COOK_COUNT 		2
#define CASHIER_COUNT		2
#define CUSTOMER_COUNT		5
#define RACK_HOLDER_SIZE	5
#define WAITING_TIME		5

typedef struct 
{
	int id;
	sem_t *order;
	sem_t *burger;
} cashier_t;
typedef struct 
{
	 int id;
	sem_t *init_done;
} simple_arg_t;
bool interrupt = false;
void *cook_run();
void *cashier_run();
void *customer_run();
void assure_state();
sem_t rack;
sem_t cook;
sem_t cashier;
sem_t cashier_awake;
sem_t customer;
sem_t customer_mutex;
cashier_t cashier_exchange;

int burger_count = 0;

int main(int argc, char **argv) {
	srand(time(NULL));
	sem_init(&rack, 0, 1);
	sem_init(&cashier, 0, 1);
	sem_init(&cashier_awake, 0, 0);
	sem_init(&cook, 0, RACK_HOLDER_SIZE);
	sem_init(&customer, 0, 0);
	sem_init(&customer_mutex, 0, 1);
	simple_arg_t args;
	sem_t init_done;
	sem_init(&init_done, 0, 0);
	args.init_done = &init_done;
	pthread_t cooks[COOK_COUNT];
	for(int i=0; i<COOK_COUNT; i++) {
		args.id = i;
		if(pthread_create(cooks+i, NULL, cook_run, (void*) &args)) {
			printf("[MAIN]\t\t ERROR: Unable to create cook thread.\n");
			exit(1);
		}
                sem_wait(&init_done);
	}


	pthread_t cashiers[CASHIER_COUNT];
	for(int i=0; i<CASHIER_COUNT; i++) {
		args.id = i;
		if(pthread_create(cashiers+i, NULL, cashier_run, (void*) &args)) {
			printf("[MAIN]\t\t ERROR: Unable to create cashier thread.\n");
			exit(2);
		}
		sem_wait(&init_done);
	}


	pthread_t customers[CUSTOMER_COUNT];
	for(int i=0; i<CUSTOMER_COUNT; i++) {
		args.id = i;
		if(pthread_create(customers+i, NULL, customer_run, (void*) &args)) {
			printf("[MAIN]\t\t ERROR: Unable to create customer thread.\n");
			exit(3);
		}
		sem_wait(&init_done);
	}
	sem_destroy(&init_done);
	for(int i=0; i<CUSTOMER_COUNT; i++) {
		if(pthread_join(customers[i], NULL)) {
			printf("[MAIN]\t\t ERROR: Unable to join cutomers[%d]\n", i);
			exit(4);
		}
	}

	printf("[MAIN]\t\t  All customers terminated\n");
	interrupt = true;
	for(int i=0; i<COOK_COUNT; i++) {
		sem_post(&cook);
	}
	for(int i=0; i<CASHIER_COUNT; i++) {
		sem_post(&customer);
	}
	for(int i=0; i<COOK_COUNT; i++) {
		if(pthread_join(cooks[i], NULL)) {
			printf("[MAIN]\t\t ERROR: Unable to join cooks[%d]\n", i);
			exit(5);
		}
	}
	for(int i=0; i<CASHIER_COUNT; i++) {
		if(pthread_join(cashiers[i], NULL)) {
			printf("[MAIN]\t\t ERROR: Unable to join cashiers[%d]\n", i);
			exit(6);
		}
	}
	assure_state();
	printf("[MAIN]\t\t  All threads terminated.\n");
}

void *cook_run(void *args) 
{
	simple_arg_t *args_ptr = (simple_arg_t*) args;
	int cook_id = args_ptr->id;
	printf("[COOK %d]\t CREATED.\n", cook_id);
	sem_post(args_ptr->init_done);
	while(1) {
		sem_wait(&cook);
		if(interrupt) {
			break;
		}
		sleep(rand() % WAITING_TIME);
		sem_wait(&rack);
		assure_state();
		burger_count++;
		assure_state();
		sem_post(&rack);

		printf("[COOK %d]\t Placed new burger in rack.\n", cook_id);
		sem_post(&cashier);
	}

	printf("[COOK %d]\t complete.\n", cook_id);
	return NULL;
}

void *cashier_run(void *args)
 {
	simple_arg_t *args_ptr = (simple_arg_t*) args;
	int cashier_id = args_ptr->id;
	sem_t order;
	sem_t burger;
	sem_init(&order, 0, 0);
	sem_init(&burger, 0, 0);
	printf("[CASHIER %d]\t CREATED.\n", cashier_id);
	sem_post(args_ptr->init_done);
	while(1) {
		sem_wait(&customer);
		if(interrupt) {
			break;
		}
		printf("[CASHIER %d]\t Serving customer.\n", cashier_id);
		cashier_exchange.order = &order;
		cashier_exchange.burger = &burger;
		cashier_exchange.id = cashier_id;
		sem_post(&cashier_awake);
		sem_wait(&order);
		printf("[CASHIER %d]\t Got order from customer.\n", cashier_id);
		printf("[CASHIER %d]\t Going to rack to get burger.\n", cashier_id);
		sleep(rand() % WAITING_TIME);
		sem_wait(&cashier);
		sem_wait(&rack);
		assure_state();
		burger_count--;
		assure_state();
		sem_post(&rack);
		sem_post(&cook);
		printf("[CASHIER %d]\t Got burger from rack.\n", cashier_id);
		sleep(rand() % WAITING_TIME);
		sem_post(&burger);
		printf("[CASHIER %d]\t Gave burger to customer.\n", cashier_id);
	}
	sem_destroy(&order);
	sem_destroy(&burger);
	printf("[CASHIER %d]\t complete.\n", cashier_id);

	return NULL;
}

void *customer_run(void *args) 
{
	simple_arg_t *args_ptr = (simple_arg_t*) args;
	int customer_id = args_ptr->id;
	printf("[CUSTOMER %d]\t CREATED.\n", customer_id);
	sem_post(args_ptr->init_done);
	sleep(rand() % WAITING_TIME + 1);
	sem_wait(&customer_mutex);
	sem_post(&customer);
	sem_wait(&cashier_awake);
	sem_t *order = cashier_exchange.order;
	sem_t *burger = cashier_exchange.burger;
	int cashier_id = cashier_exchange.id;
	sem_post(&customer_mutex);
	printf("[CUSTOMER %d]\t Approached cashier no. %d.\n",
		customer_id, cashier_id);
	printf("[CUSTOMER %d]\t Placing order to cashier no. %d.\n",
		customer_id, cashier_id);
	sleep(rand() % WAITING_TIME);
	sem_post(order);
	sem_wait(burger);
	printf("[CUSTOMER %d]\t Got burger from cashier no. %d\n",
		customer_id, cashier_id);
	printf("[CUSTOMER %d]\t complete\n", customer_id);

	return NULL;

}

void assure_state()
 {
	if(burger_count < 0) {
		printf("[ASSURE_STATE]\t ERROR: No burger in the rack!\n");
		exit(40);
	}

	if(burger_count > RACK_HOLDER_SIZE) {
		printf("[ASSURE_STATE]\t ERROR: Rack overfull!\n");
		exit(41);
	}

}
