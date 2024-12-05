#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

// 定义常量
#define CHAIRS 3          // 等待椅子的数量
#define CUSTOMERS 30      // 顾客总数
#define BARBER_CUT_TIME_AVG 5 // 理发师平均理发时间（秒）

// 信号量
sem_t waitingCustomers;    // 等待的顾客信号量
sem_t waitingBarber;         // 理发师准备好的信号量
// sem_t waitingCustomersId    //等待获取当前顾客id

// 互斥量
pthread_mutex_t outputMutex;  // 确保输出有序
pthread_mutex_t mutex;     // 用于保护临界区的互斥量

//全局变量
int freeChairs = CHAIRS;   // 当前空闲的椅子数量
int totalCustomers = 0;    // 已到达的顾客数量

// 队列相关
int waitingQueue[CHAIRS];
int front = 0;
int rear = 0;
int count = 0;

// 新增的互斥量用于保护队列操作
pthread_mutex_t queueMutex;

// 入队函数
void enqueue(int id) {
    pthread_mutex_lock(&queueMutex);
    if (count < CHAIRS) {
        waitingQueue[rear] = id;
        rear = (rear + 1) % CHAIRS;
        count++;
    }
    pthread_mutex_unlock(&queueMutex);
}

// 出队函数
int dequeueCustomer() {
    pthread_mutex_lock(&queueMutex);
    int id = -1;
    if (count > 0) {
        id = waitingQueue[front];
        front = (front + 1) % CHAIRS;
        count--;
    }
    pthread_mutex_unlock(&queueMutex);
    return id;
}

// 理发师线程函数
void *barber(void *arg) {
    while (totalCustomers < CUSTOMERS || freeChairs != CHAIRS) {
        // 等待顾客到来
        sem_wait(&waitingCustomers);

        printf("Barber Ready to cut hair for Customer %d. Free chairs: %d\n", totalCustomers, freeChairs);
        // 发出信号通知顾客理发师已准备好
        sem_post(&waitingBarber);

        // 顾客起身
        pthread_mutex_lock(&mutex);
        printf("barber enter critical region");
        freeChairs++;
        pthread_mutex_unlock(&mutex);
        printf("barber leave critical region");

        // 理发中（随机时间）
        // printf("Barber Cutting hair for Customer using %d seconds...\n", cutTime);
        // sleep(cutTime);
        // printf("Barber Finished cutting hair for Customer.\n");

        // 从队列中取出顾客ID
        int customerId = dequeueCustomer();
        int cutTime = rand() % 3 + BARBER_CUT_TIME_AVG; // 理发时间为5秒左右，随机上下波动
        printf("Barber Cutting hair for Customer %d using %d seconds...\n", customerId, cutTime);
        sleep(cutTime);
        printf("Barber Finished cutting hair for Customer%d.\n", customerId);
    }
    return NULL;
}

// 顾客线程函数
void *customer(void *arg) {
    int id = *(int *)arg;

    // 顾客到达前的等待时间随机
    int arrivalTime = rand() % 3;
    sleep(arrivalTime);
    printf("Customer %d: Arrived after waiting for %d seconds.\n", id, arrivalTime);

    if (freeChairs > 0) {
        // 有空闲的椅子，顾客可以等待

        //顾客先坐
        printf("customer %d sit down")
        pthread_mutex_lock(&mutex);
        printf("customer %d enter critical region",id);
        freeChairs--;
        pthread_mutex_unlock(&mutex);
        printf("customer %d leave critical region",id);

        enqueue(id); // 将顾客ID加入队列

        printf("Customer %d: Waiting for haircut. Free chairs: %d\n", id, freeChairs);
        // 通知理发师有顾客到来
        sem_post(&waitingCustomers);

        // 等待理发师准备好
        sem_wait(&waitingBarber);

        // 理发中
        printf("Customer %d: Getting a haircut.\n", id);
    } else {
        // 没有空闲的椅子，顾客离开
        printf("Customer %d: Leaving as no chair is available.\n", id);
    }

    pthread_mutex_lock(&mutex);
    printf("customer %d enter critical region",id);
    totalCustomers++;
    pthread_mutex_unlock(&mutex);
    printf("customer %d leave critical region",id);

    return NULL;
}

int main() {
    pthread_t barberThread;
    pthread_t customerThreads[CUSTOMERS];
    int customerIds[CUSTOMERS];

    // 初始化随机种子
    srand(time(NULL));

    // 初始化信号量和互斥量
    sem_init(&waitingCustomers, 0, 0);//0:thread share 1:course share
    sem_init(&waitingBarber, 0, 0);//0:no customer or barer
    pthread_mutex_init(&mutex, NULL);//NULL:默认
    pthread_mutex_init(&outputMutex, NULL);
    pthread_mutex_init(&queueMutex, NULL);

    // 创建理发师线程
    pthread_create(&barberThread, NULL, barber, NULL);//默认属性线程

    // 创建顾客线程
    for (int i = 0; i < CUSTOMERS; i++) {
        customerIds[i] = i + 1;
        pthread_create(&customerThreads[i], NULL, customer, &customerIds[i]);
        printf("create customer %d \n",customerIds[i]);
    }

    printf("Customers create finished\n!");

    // 等待所有顾客线程结束
    for (int i = 0; i < CUSTOMERS; i++) {
        pthread_join(customerThreads[i], NULL);
    }

    // 等待理发师线程结束
    pthread_join(barberThread, NULL);

    printf("all Customer finished!\n");

    // 销毁信号量和互斥量
    sem_destroy(&waitingCustomers);
    sem_destroy(&waitingBarber);
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&outputMutex);

    return 0;
}
