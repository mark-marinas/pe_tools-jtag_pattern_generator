#include <stdlib.h>
#include "linklist.h"
#include <stdio.h>

static void cleanup_linklistE(linklistE_t *e) {
	if ( e->next ) {
		cleanup_linklistE (e->next);
		free(e->data);
		free(e);
	} else {
		free(e->data);
		free(e);
	}
}

static void cleanup_linklist(linklist_t *l) {
	linklistE_t *e = l->head ;

	//Free up all the elements recursively.
	cleanup_linklistE(e);		

	//Finally, point the head and tail to null
	l->head = l->tail = 0;
}


void init_linklist(linklist_t *l) {
	l->head = l->tail = l->currE =  0;
}

void enqueue_linklist(linklist_t *l, void *d) {
	//Empty pointer to linklist or data.
	if (d == 0 || l == 0) {
		return ;	
	}	

	linklistE_t *e = malloc(sizeof(linklistE_t)) ; 
	e->data = d;
	e->next = 0;

	//Empty linklist
	if (l->head == 0) {
		l->head =  e;
		l->tail =  e;
		l->currE = 0;
	} else {
		(l->tail)->next = e;
		l->tail	= e;
	}
}

void *dequeue_linklist(linklist_t *l) {
	//The current linklist element starts by pointing to nowhere.
	//static linklistE_t *currlinklistE = 0;
	void *ret;

	//Empty linklist
	if (l->head == 0) {
		return (void *) 0;
	}

	//First time to get data from this linklist.
	if (l->currE == 0) {
		l->currE = l->head;
	} else { 
		l->currE = (l->currE)->next ;	
	} 		


	//No more to element to return.	
	if ( l->currE == 0) {
		ret = (void *) 0;
		cleanup_linklist(l);
	} else {
		ret = (l->currE)->data;
	}

	return ret;
}

inline void *gettail_linklist(linklist_t *l) {
        return (l->tail != 0) ? (l->tail)->data:0;
}


#ifdef _TEST_LINKLIST_

#include <string.h>
#include <stdio.h>

char *lleements[4];

int main() {
	linklist myll;
	init_linklist(&myll);

	int i;
	for (i=0; i<5; i++) {
		lleements[i]=malloc(strlen("elem1") + 1);
		sprintf (lleements[i],"elem%d", i);
		enqueue_linklist(&myll, lleements[i]); 	
	}

	char *e = (char *) dequeue_linklist(&myll);
	while (e) {
		printf ("%s\n", e);
		e = (char *) dequeue_linklist(&myll);
	}

	for (i=5; i<10; i++) {
		lleements[i]=malloc(strlen("elem1") + 1);
		sprintf (lleements[i],"elem%d", i);
		enqueue_linklist(&myll, lleements[i]); 	
	}
	
	e = (char *) dequeue_linklist(&myll);
	while (e) {
		printf ("%s\n", e);
		e = (char *) dequeue_linklist(&myll);
	}

	e = (char *) dequeue_linklist(&myll);
	while (e) {
		printf ("%s\n", e);
		e = (char *) dequeue_linklist(&myll);
	}

	return 0;	
	
}

#endif
