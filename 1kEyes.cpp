//I'm still struggling with getting the threads to continue running
//in-progress -> cancelling tasks && task2
//Written by Alexandra Dyla
//MAC OS Sierra
#include <sys/time.h>
#include <pthread.h>
#include <cstdlib>
#include <unistd.h>
#include <mutex>          // std::mutex
#include <iostream>       //std::cout
#include <queue>          //std::priority_queue
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sqlite3.h>      //sqlite

typedef void (*TaskToRun)();
std::mutex mtx;
std::mutex mtx2;

#define MAX_SIZE 500

struct addrinfo hints, *result, *rp;   //structs for getaddrinfo
//seperate struct for scheduling the next time to run it
//interval and time to run that specific instance of the task are different
struct TaskDef{
  //function to be run , function pointer
  TaskToRun t;    //function pointer
  int interval;   //interval to wait to run ex: 30 s
};

struct Task{
    //needs a reference to the TaskDef
    //need to save time to run
    TaskDef d;       //reference to the task definition
    int run_time;     //actual time to run the task ex: 3:01pm
};

//callback for sqlite, taken from https://www.tutorialspoint.com/sqlite/sqlite_c_cpp.htm
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
   return 0;
}

//overload the < operator
bool operator< (Task const& lhs, Task const& rhs){
   return lhs.run_time > rhs.run_time;
}
//priority queue holds "pending" tasks
typedef std::priority_queue<Task> TaskQueue;
TaskQueue tq;

//appends runtime to all items currently in queue
void scheduleTask(Task &task, TaskQueue &taskQueue){
    //std::cout<<"schedule" <<std::endl;
    time_t timenow;
    int i;
    timenow = time(NULL);  //current time
    task.run_time = task.d.interval + timenow;
    mtx.lock();
    taskQueue.push(task);
    mtx.unlock();
}

//race condition, sometimes correct function other not
void *wrapper(void *arg) {
  Task top = tq.top();
  //std::cout<<top.d.interval<<std::endl;
  Task t = *(Task*) arg;
  std::cout<<"the wrapper for task: "<< t.d.interval<<std::endl;
  t.d.t();
  scheduleTask(t, tq);
  return NULL;
}

void runTask(Task t) {
  pthread_t thread;
  int id;
  std::cout<<"Thread created"<<std::endl;
  id = pthread_create(&thread, NULL, wrapper,(void*)&(t));
  if(id){
    std::cout<<"Error: enable to create thread, " << id << std::endl;
    exit(-1);
  }
}

void *searchToRun(void *taskQueue){
    //1 thread for current executing task
    //once task is finished add it back to queue with new run_time = timenow + interval
    //should mutex around queue while thread is running?

    Task topTask;
    tq = *(TaskQueue*) taskQueue;

    while(!tq.empty()){
      time_t timenow;
      timenow = time(NULL);
      Task topTask;
      mtx.lock();    //system call, expensive
      if (!tq.empty()) {
        topTask = tq.top();
        if(timenow >= topTask.run_time){
          std::cout << "Run things" << std::endl;
          runTask(topTask);
          tq.pop();
          mtx.unlock();
        } else {   //if no tasks set to run yet sleep and check again
          mtx.unlock();
          sleep(5);   //sleep for 1 second, check queue for tasks to run again
          continue;
        }
      }
    }
    return NULL;
  }

//determines the TCP connection time
void task1(){
  int sfd, s;
  char * sql;
  const char * inetAddr;
  float time_elapsed;
  struct timeval start, stop;

  if ((s = getaddrinfo("www.google.com", "https", &hints, &result)) != 0){
    std::cout<< stderr <<std::endl;
  }

  for (rp = result; rp != NULL; rp = rp->ai_next) {
       sfd = socket(rp->ai_family, rp->ai_socktype,rp->ai_protocol);
       if (sfd == -1)
           continue;
       gettimeofday(&start, NULL);
       if (connect(sfd, rp->ai_addr, rp->ai_addrlen) == 0){
         gettimeofday(&stop, NULL);
         break;    /* Success */
       }
       close(sfd);
   }

   if (rp == NULL) {               /* No address succeeded */
       fprintf(stderr, "Cannot connect\n");
       exit(EXIT_FAILURE);
   }
   //ms
   time_elapsed = ((stop.tv_sec - start.tv_sec) + ((stop.tv_usec - start.tv_usec)/1000.0));
   std::cout<< "TCP connection time: " << time_elapsed << "s" << std::endl;
   sql = "INSERT INTO METRICS (ID,TCP TIME) " \
          "VALUES (1, time_elapsed);" ;
}
//ICMP ping echo request
void task2(){
  std::cout<<"Task 2 executing" <<std::endl;
  //ping with system call is troublesome because it doesnt return 1 record
  //also handling fork, waitpid, etc, resulting in trouble trying to kill the process
  //system("ping 8.8.8.8")
}


int main(int argc, char *argv[]){
    int rc, val;
    char * sql;
    char *zErrMsg = 0;
    pthread_t thread;
    sqlite3 *db;

    Task firstTask;
    Task secTask;

    //setup SQLite DB
    val =  sqlite3_open("test.db", &db);
    if(val){
      std::cout<< "Can't open database: " << stderr <<  sqlite3_errmsg(db) << std::endl;
      return(0);
    }else{
      std::cout << "Opened DB sucessfully" << std::endl;
    }
//sql statement creation of table
    sql = "CREATE TABLE METRICS(" \
            "ID INT PRIMARY KEY      NOT NULL,"\
            "TCP TIME       INT      NOT NULL);";
    val = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    if (val != SQLITE_OK){
      std::cout << "SQL error: " << zErrMsg << std::endl;
    }else{
      std::cout << "Table created sucessfully" << std::endl;
    }

    firstTask.d.interval = 15;
    firstTask.d.t = *task1;

    secTask.d.interval = 17;
    secTask.d.t = *task2;

    scheduleTask(firstTask, tq);
    scheduleTask(secTask, tq);

    Task top;
    top = tq.top();
    //std::cout<<top.d.interval<<std::endl;

    //thread needed to continually search through priority queue for tasks to run
    //the priority queue is basically a "pending" tasks list
    rc = pthread_create (&thread, NULL, searchToRun, (void*)&tq);
    if (rc){
      std::cout<<"Error: unable to create thread."<<std::endl;
      exit(-1);
    }
    pthread_exit(NULL);
}
