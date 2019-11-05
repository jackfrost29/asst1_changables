/*
 * **********************************************************************
 * You are free to add or modify anything you think you require to this file
 */

#ifndef _INVESTOR_PRODUCER_H_
#define _INVESTOR_PRODUCER_H_

/**
 * FUNCTIONs Prototype declares here
 * Add or modify more funtion if you need more
 **/

struct order{
	struct item *ptr;
	struct order *next, *prev;
};

void order_item(void *); 			// customer orders one or more items
void consume_item(long); 			// consume items and update spending
void end_shoping(void); 			//finish shoping and go home
long int noOrderLeft(void);				// to check if any order is left to be taken or processable
void insert_head(struct item *);
void insert(struct item *, struct item *);
void printQueue(void);


/* Producer functions */
void *take_order(void); 			//take order from the customers
void produce_item(void *); 			// producer produce an item or product
long int calculate_loan_amount(void*); 		//calculate loan amount
void loan_request(void *,unsigned long); 			// producer request loan to a bank
void serve_order(void *,unsigned long); 			// producer serve the request of a customer
void loan_reimburse(void *,unsigned long); 			// return loan money with service charge
void insert_order(struct order *temp, struct item *ptr);
struct order *insert_order_head(struct item *ptr);


/* Investor-Producer process opening and closing functions */

void initialize(void);
void finish(void);

#endif
// finally updated
