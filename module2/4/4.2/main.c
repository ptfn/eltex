#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PRIORITY 255
#define EMPTY -1

typedef struct Node {
    int data;
    int priority;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
} PriorityQueue;

void pq_init(PriorityQueue *pq) {
    pq->head = NULL;
}

int pq_is_empty(PriorityQueue *pq) {
    return pq->head == NULL;
}

/* Add element to the end of the queue */
void pq_add(PriorityQueue *pq, int data, int priority) {
    Node *node = malloc(sizeof(Node));
    node->data = data;
    node->priority = priority;
    node->next = NULL;

    if (pq->head == NULL) {
        pq->head = node;
    } else {
        Node *cur = pq->head;
        while (cur->next != NULL)
            cur = cur->next;
        cur->next = node;
    }
}

/* Remove node after prev (or head if prev is NULL) */
static int pq_pop_after(PriorityQueue *pq, Node *prev) {
    Node *node;
    if (prev == NULL) {
        node = pq->head;
        pq->head = node->next;
    } else {
        node = prev->next;
        prev->next = node->next;
    }
    int data = node->data;
    free(node);
    return data;
}

/* Extract first element (FIFO order) */
int pq_extract_first(PriorityQueue *pq) {
    if (pq->head == NULL)
        return EMPTY;
    return pq_pop_after(pq, NULL);
}

/* Extract first element with exact priority */
int pq_extract_with_prio(PriorityQueue *pq, int priority) {
    Node *prev = NULL;
    for (Node *cur = pq->head; cur != NULL; cur = cur->next) {
        if (cur->priority == priority)
            return pq_pop_after(pq, prev);
        prev = cur;
    }
    return EMPTY;
}

/* Extract first element with priority >= min_prio */
int pq_extract_prio_not_less(PriorityQueue *pq, int min_prio) {
    Node *prev = NULL;
    for (Node *cur = pq->head; cur != NULL; cur = cur->next) {
        if (cur->priority >= min_prio)
            return pq_pop_after(pq, prev);
        prev = cur;
    }
    return EMPTY;
}

void pq_destroy(PriorityQueue *pq) {
    while (!pq_is_empty(pq))
        pq_extract_first(pq);
}

void print_queue(PriorityQueue *pq, const char *label) {
    printf("%s", label);
    if (pq_is_empty(pq)) {
        printf(" (empty)\n");
        return;
    }
    printf("\n");
    for (Node *cur = pq->head; cur != NULL; cur = cur->next)
        printf("  [data=%2d, prio=%3d]\n", cur->data, cur->priority);
}

int main() {
    PriorityQueue pq;
    pq_init(&pq);
    srand(time(NULL));

    const char *names[] = {"LOW", "NORMAL", "HIGH", "URGENT"};
    int levels[] = {50, 100, 180, 250};
    int n = 4;

    /* Generate messages */
    printf("=== Generating messages ===\n");
    for (int i = 1; i <= 12; i++) {
        int lvl = rand() % n;
        pq_add(&pq, i, levels[lvl]);
        printf("Message %2d: data=%2d, prio=%3d (%s)\n", i, i, levels[lvl], names[lvl]);
    }

    print_queue(&pq, "\nQueue state:");

    /* Test 1: Extract first */
    printf("\n=== Extract first ===\n");
    int val = pq_extract_first(&pq);
    printf("Result: data=%d\n", val);
    print_queue(&pq, "Queue after:");

    /* Test 2: Extract with exact priority */
    printf("\n=== Extract with priority 100 (NORMAL) ===\n");
    val = pq_extract_with_prio(&pq, 100);
    printf("Result: data=%d\n", val);
    print_queue(&pq, "Queue after:");

    /* Test 3: Extract with priority >= 180 */
    printf("\n=== Extract with priority >= 180 (HIGH+) ===\n");
    val = pq_extract_prio_not_less(&pq, 180);
    printf("Result: data=%d\n", val);
    print_queue(&pq, "Queue after:");

    /* Test 4: Extract first again */
    printf("\n=== Extract first again ===\n");
    val = pq_extract_first(&pq);
    printf("Result: data=%d\n", val);
    print_queue(&pq, "Queue after:");

    /* Drain */
    printf("\n=== Draining remaining ===\n");
    while (!pq_is_empty(&pq)) {
        val = pq_extract_first(&pq);
        printf("Extracted: data=%d\n", val);
    }

    pq_destroy(&pq);
    return 0;
}
