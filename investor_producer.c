#include <types.h>
#include <lib.h>
#include <synch.h>
#include <test.h>
#include <thread.h>

#include "investor_producer.h"
#include "invest_assignment.h"


/*
 * **********************************************************************
 * YOU ARE FREE TO CHANGE THIS FILE BELOW THIS POINT AS YOU SEE FIT
 *
 */

extern struct semaphore *print_sem;
extern struct item *req_serv_item;
extern struct bankdata bank_account[NBANK];
extern long int customer_spending_amount[NCUSTOMER];
extern long int producer_income[NPRODUCER];


long int customer_order_count[NCUSTOMER];
long int total_order_amount = NCUSTOMER*N_ITEM_TYPE * 10;
static struct semaphore *sem_item, *sem_order_ready[NCUSTOMER], *sem_cust_ord_calc[NCUSTOMER], *sem_bank[NBANK], *sem_non_empty_order;


/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY CUSTOMER THREADS
 * **********************************************************************
 */


/*
 * order_item()
 *
 * Takes one argument specifying the item produces. The function
 * makes the item order available to producer threads and then blocks until the producers
 * have produced the item with that appropriate for the customers.
 *
 * The item itself contains the number of ordered items.
 */ 
 
void insert_head(struct item *ptr){
	req_serv_item = ptr;
}
void insert(struct item *temp, struct item *ptr){
	temp->next = ptr;
}

void order_item(void *itm){
	P(sem_item);
	P(print_sem);
	//kprintf("order item start");
	V(print_sem);
	struct item *temp = req_serv_item;
	struct item *order_itm_arr_ptr = itm;
	
	if(req_serv_item == NULL || noOrderLeft() == 1){
		V(sem_non_empty_order);
	}
	
	
	
	for(long int i=0; i<N_ITEM_TYPE; i++, order_itm_arr_ptr++){
		if(temp == NULL){
			insert_head(order_itm_arr_ptr);
			temp = req_serv_item;
		}
		else{
			insert(temp, order_itm_arr_ptr);
			temp = temp->next;
		}
		// now temp is pointing to the newly added item
		temp->i_price = temp->item_quantity*ITEM_PRICE;
		temp->order_type = REQUEST;
		//total_order_amount--;	// this instruction will be executed in take order function
		customer_order_count[temp->requestedBy]++;
			
	}
	/*
	if(temp!= NULL){	// iterate at the end of the list
		while(temp->next != NULL)
			temp = temp->next;
	}
	
	else{
		req_serv_item = order_itm_arr_ptr;
		req_serv_item->i_price = order_itm_arr_ptr->item_quantity * ITEM_PRICE;	// the price of orde is set here
		order_itm_arr_ptr->order_type = REQUEST;	// order type is set here
		i++;					// number of remaining orders to be added to the service queue is now one less.
		total_order_amount--;	// total remaining order will decrease
		temp = req_serv_item;	// temp points to the end of service queue, in this case to the head
		order_itm_arr_ptr++;	// now it points to the next order in the array
		customer_order_count[req_serv_item->requestedBy]++;	// does not need semaphore, only one customer access at a time
	}
	for(; i<N_ITEM_TYPE; i++){
		temp->next = order_itm_arr_ptr;	// one order added to the service queue
		order_itm_arr_ptr->i_price = order_itm_arr_ptr->item_quantity * ITEM_PRICE;	// the price of orde is set here
		order_itm_arr_ptr->order_type = REQUEST;	// order type is set here
		total_order_amount--;
		temp = temp->next;
		order_itm_arr_ptr++;	// now it points to the next order in the array
		customer_order_count[temp->next->requestedBy]++;	// does not need semaphore, only one customer access at a time
	}
	*/
	
	
	temp->next = NULL;
	//printQueue();	// for debug reasons
	
	P(print_sem);
	//kprintf("order item finish");
	V(print_sem);
	V(sem_item);
}

/*
 * struct item{ 			// customer order/producer when serve/produce
    unsigned int item_quantity; // quantity intended to purchase by a customer; 1 to MAX_ITEM_BUY or amount of item produced
    unsigned int  i_price; 	// item-unit price; given as ITEM_PRICE
    unsigned long int servBy;	// producer id
    long int requestedBy; 	// customer id
    unsigned int order_type;	// REQUEST or SERVICE in the order queue
    struct item *next;		// Link to next order	
};*/

// print the whole queue for debug reason
void printQueue(){
	P(print_sem);
	struct item *ptr = req_serv_item;
	kprintf("Request serv queue details");
	long int i=1;
	while(ptr != NULL){
		kprintf("Item %ld: quantity-%d i_price-%d requseted by-%ld\n", i, ptr->item_quantity, ptr->i_price, ptr->requestedBy);
		i++;
		ptr = ptr->next;
	}
	V(print_sem);
}

long int noOrderLeft(){
	if(req_serv_item == NULL)
		return 1;	// no order left
	struct item *temp = req_serv_item;
	while(temp != NULL){
		if(temp->order_type == REQUEST)
			return 0;	// still order left
		temp = temp->next;
	}
	return 1;	// truly no processable order left
}

/**
 * consume_item() 
 * Customer consume items which were served by the producers.
 * affected variables in the order queue, on item quantity, order type, requested by, served by
 * customer should keep records for his/her spending in shopping
 * and update spending account
 **/
void consume_item(long customernum){
	P(sem_order_ready[customernum]);
	kprintf("prepared order will now be consumed\n");
	P(sem_item);
	//printQueue();	// this line is for debugging purpose
	struct item *prev = NULL, *del = req_serv_item;
	while(del != NULL){
		if(del->requestedBy == customernum){
			/*
			 * this item was req by customernum and it surely is produced, 
			 * otherwise the consumer thread could't enter in this region
			 */
			if(prev == NULL){	//it's the head of service queue that needs to be deleted.
				req_serv_item = del->next;
				//kfree(del);
				del = req_serv_item;
			}
			else{
				prev->next = del->next;
				//kfree(del);
				del = prev->next;
			}
		}
		else{
			prev = del;
			del = del->next;
		}
	}
	V(sem_item);
}

/*
 * end_shoping()
 *
 * This function is called by customers when they go home. It could be
 * used to keep track of the number of remaining customers to allow
 * producer threads to exit when no customers remain.
 */

void end_shoping(){
}


/*
 * **********************************************************************
 * FUNCTIONS EXECUTED BY ITEM PRODUCER THREADS
 * **********************************************************************
 */

/*
 * take_order()
 *
 * This function waits for a new order to be submitted by
 * customers. When submitted, it records the details and returns a
 * pointer to something representing the order.
 *
 * The return pointer type is void * to allow freedom of representation
 * of orders.
 *
 * The function can return NULL to signal the producer thread it can now
 * exit as there are no customers nor orders left.
 */

struct order *insert_order_head(struct item *ptr){
	struct order *temp = (void *)kmalloc(sizeof(struct order));
	temp->prev = NULL;
	temp->next = NULL;
	temp->ptr = ptr;
	return temp;
}

void insert_order(struct order *temp, struct item *ptr){
	temp->next = (void *)kmalloc(sizeof(struct order));
	temp->next->prev = temp;
	temp->next->ptr = ptr;
	temp->next->next = NULL;
}


void *take_order(){
	/*
	if(req_serv_item == NULL && total_order_amount <= 0){
		return NULL;
	}
	*/
	if(total_order_amount <= 0)
		return NULL;

	P(sem_non_empty_order);
	P(sem_item);
	struct item *temp = req_serv_item;
	struct order *order_list = NULL, *temp_order_list = NULL;
	long int count = 0;
	while(temp != NULL){
		if(temp->order_type == REQUEST){
			count ++;
		}
		temp = temp->next;
	}
	if(count > 15)
		count = count / 3;
	temp = req_serv_item;
	
	for(long int i=0; i<count; i++, temp = temp->next){
		if(temp->order_type == REQUEST){
			total_order_amount--;	// this instruciton was previously executed inside order_item function 
			temp->order_type = SERVICED;
			if(order_list == NULL){
				order_list = insert_order_head(temp);
				temp_order_list = order_list;
			}
			else{
				insert_order(temp_order_list, temp);
				temp_order_list = temp_order_list->next;
			}
		}
		else
			i--;
		
		
	}
	/*
	while(count --){
		if(temp->order_type == REQUEST){
			if(temp_order_list == NULL){	// no order adder to the order list as of yet
				order_list = (void *)kmalloc(sizeof(struct order));
				order_list->prev = NULL;
				order_list->next = NULL;
				order_list->ptr = temp;
				temp_order_list = order_list;
			}
			else{
				temp_order_list->next = (void*)kmalloc(sizeof(struct order));
				temp_order_list->next->ptr = temp;
				temp_order_list->next->prev = temp_order_list;
				temp_order_list = temp_order_list->next;
			}
			
			temp->order_type = SERVICED;	// so that no other producer can take the order
			temp = temp->next;
		}
		else
			count++;
	}
	temp_order_list->next = NULL;
	*/
	long int flag = 1;
	temp = req_serv_item;
	while(temp->next != NULL){
		if(temp->order_type == REQUEST)
			flag = 0;
		temp = temp->next;
	}
	V(sem_item);
	if(flag == 0)	// request service queue is not empty, sem lock can be opened
		V(sem_non_empty_order);

	return order_list;
}


/*
 * produce_item()
 *
 * This function produce an item if the investment is available for the product
 *
 */

void produce_item(void *v){
	(void)v;
}


/*
 * serve_order()
 *
 * Takes a produced item and makes it available to the waiting customer.
 */

void serve_order(void *p,unsigned long producernumber){
	struct order *temp = p;
	while(temp != NULL){
		temp->ptr->servBy = producernumber;
		int price = temp->ptr->i_price + (PRODUCT_PROFIT * temp->ptr->i_price / 100);
		
		P(sem_cust_ord_calc[temp->ptr->requestedBy]);
		customer_order_count[temp->ptr->requestedBy]--;
		customer_spending_amount[temp->ptr->requestedBy] += price;	// needs semaphore, cause multiple producer can serve the same consumer simultaneously
		if(customer_order_count[temp->ptr->requestedBy] <= 0){
			kprintf("Customer order is prepared\n");
			V(sem_order_ready[temp->ptr->requestedBy]);
		}
		V(sem_cust_ord_calc[temp->ptr->requestedBy]);
		
		producer_income[producernumber] += price;
		
		temp = temp->next;

	}
}

/**
 * calculate_loan_amount()
 * Calculate loan amount
 */
long int calculate_loan_amount(void* itm){
	struct order *temp = itm;
	long int tot_price = 0;
	while(temp != NULL){
		tot_price += temp->ptr->i_price;
		temp = temp->next;
	}
    return tot_price;
}

/**
 * void loan_request()
 * Request for loan from bank
 */
void loan_request(void *amount,unsigned long producernumber){
	
	
	long int *amount_temp = (long int *) amount;
	int no = random()%NBANK;
	P(sem_bank[no]);
	bank_account[no].remaining_cash -= *(amount_temp);
	bank_account[no].prod_loan[producernumber] += *(amount_temp);
	V(sem_bank[no]);
}

/**
 * loan_reimburse()
 * Return loan amount and service charge
 */
void loan_reimburse(void * loan,unsigned long producernumber){
	/*
	 * the producer surely will be able to return back the loan from the bank
	 * because be fore calling this method the producer has called serve order on which all the orders
	 * taken by the producers based on which the loan was taken were acknowledged by
	 * the customer's payment with profit.
	 */
	 
	(void)loan;
	for(int i=0; i<NBANK; i++){
		P(sem_bank[i]);
		
		if(bank_account[i].prod_loan[producernumber] > 0){
			int ret = bank_account[i].prod_loan[producernumber] + (bank_account[i].prod_loan[producernumber] * BANK_INTEREST / 100);
			int interest = ret - bank_account[i].prod_loan[producernumber];
			bank_account[i].remaining_cash += ret;
			bank_account[i].interest_amount += interest;
			producer_income[producernumber] -= bank_account[i].prod_loan[producernumber];	// no semaphore, only one at a time
			bank_account[i].prod_loan[producernumber] = 0;
		}
		
		V(sem_bank[i]);
	}
}

/*
 * **********************************************************************
 * INITIALISATION AND CLEANUP FUNCTIONS
 * **********************************************************************
 */


/*
 * initialize()
 *
 * Perform any initialization you need before opening the investor-producer process to
 * producers and customers
 */

void initialize(){
	//kfree(NULL);
	print_sem = sem_create("semaphore for printing", 1);	// debug
	req_serv_item = NULL;	// initially the service queue is empty, it's not rocket science
	sem_item = sem_create("request service item sem", 1);
	for(int i=0; i<NBANK; i++){
		sem_bank[i] = sem_create("individual bank semaphore", 1);

	}
	for(int i=0; i<NCUSTOMER; i++){
		sem_cust_ord_calc[i] = sem_create("semaphore for access to individual customer record", 1);
		sem_order_ready[i] = sem_create("order of individual customer", 0);
		customer_spending_amount[i] = 0;
	}
	for(int i=0; i<NPRODUCER; i++)
		producer_income[i] = 0;
	sem_non_empty_order = sem_create("semaphore to check if service queue is not empty", 0);	// initially service queue is empty
	
}

/*
 * finish()
 *
 * Perform any cleanup investor-producer process after the finish everything and everybody
 * has gone home.
 */

void finish(){
	sem_destroy(print_sem);
	sem_destroy(sem_item);
	for(int i=0; i<NBANK; i++){
		sem_destroy(sem_bank[i]);

	}
	for(int i=0; i<NCUSTOMER; i++){
		sem_destroy(sem_order_ready[i]);
		sem_destroy(sem_cust_ord_calc[i]);
	}
}
