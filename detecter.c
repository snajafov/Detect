/* ////////////////
Operating System Architecture project
Irada Bunyatova ////
and             ////
Sanan Najafov   ////
*///////////////////
// final version
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>


// defining default values
#define DFT_TIME_FMT NULL
#define DFT_INTERVAL 10000
#define DFT_LIMIT 0
#define DFT_DETECT_RETURN 0

// in case of incorrect usage, print the usage and exit the program
void print_usage(void){
    fprintf(stderr,"\nCorrect usage: \ndetect [-t format] [-i interval] [-l limit] [-c] prog arg ...\n"
    "-t : specified time format \n"
    "-i : interval (in milliseconds)\n"
    "-l : limit of launch number\n"
    "-c : detects return code \n");
    exit(1);
}

// printing time in specified format (timeFmt)
void print_time (const char *timeFmt){
    time_t timer = time(NULL) ;
    struct tm *tm;
    tm = localtime(&timer);
    if( tm == NULL){
        fprintf(stderr,"Localtime error .. \n");
    }
    char buf[1024] ;

    if (timeFmt != NULL){
        if (strftime (buf, 1024, timeFmt, tm) == 0){
            fprintf(stderr, "Strftime error .. ]n") ;
            exit(1);
        }
        printf ("%s\n", buf) ;
    }
}

//function that prints output 
void print_output(char *prevOutput,char *curOutput,size_t prevlen,size_t curlen){
        if (prevOutput == NULL){                      //if there is no previous output 
            fwrite(curOutput, curlen, 1, stdout) ;
        } 
        else if (prevlen != curlen || memcmp(prevOutput,curOutput,curlen) != 0){ // if current Output is different than previous
            fwrite(curOutput,curlen,1,stdout);
        }
        free(prevOutput) ;          
}

// prints exit code only if -c option included and if current exit code is different from previous
void print_exit(int prevReturn,int curReturn,int n){             
         if (n== 0) printf ("exit %d\n", curReturn) ;              // if it is first iteration
         else if (prevReturn != -1 && curReturn != prevReturn) 
            printf ("exit %d\n", curReturn) ;
}

// 
void get_output(int fd,char **curOutput,size_t *curLength,int *curReturn){
    size_t bufLength = 0 ;			
    *curLength = 0 ;
    *curOutput = NULL ;
    ssize_t n;

    do{
		bufLength += 1024;  
		*curOutput = realloc(*curOutput, bufLength) ; // reallocating the current output
		if (*curOutput == NULL) perror("realloc");
		n = read (fd,*curOutput + *curLength,1024) ;  
		if (n > 0)
			*curLength += n ;                         // current length will be changed
    } while(n > 0) ;
    
    int status;
    if(wait(&status) == -1) perror("wait");           // wait exits with error code
    if(!WIFEXITED(status)) perror("exit");
    *curReturn = WEXITSTATUS(status) ;                 // current exit status will be changed
}
int main(int argc,char *argv[]){
    int opt;
    // giving default values to some variables
    int limit            = DFT_LIMIT;
    int interval         = DFT_INTERVAL*1000;
    int detectReturn     = DFT_DETECT_RETURN;
    const char *timeFmt  = DFT_TIME_FMT;


    while (( opt = getopt (argc, argv, "+t:i:l:c")) != -1){
        switch (opt){
            case 't' :
                timeFmt = optarg;
                break ;
            case 'i' :
                interval = atoi(optarg)*1000;
                if (interval <= 0)
                    print_usage();
                break ;
            case 'l' :
                limit = atoi(optarg);
                if (limit < 0)
                    print_usage() ;
                break ;
            case 'c' :
                detectReturn = 1 ;
                break ;
            default :
                print_usage() ;
        }
    }

    // Expected argument after options
    if (optind >= argc) {        // in case of incorrect usage, print usage     
        print_usage();          
    }

    int tube[2] ;
    int n = 0 ;                           // number of iterations
    char *curOutput,*prevOutput = NULL ; // no previous output for first iteration
    int  prevReturn,curReturn;
    size_t curLength, prevLenght;

    // while number of iteratioins is less than limit or there is no limit
     while (n < limit || limit == 0) {  
        pipe(tube) ;                     // creating a pipe
        int pid = fork ();
        switch (pid){
            case -1 :
                fprintf(stderr,"Fork error at iteration %d..\n",n);
                exit(1);
                break ;
            case 0 :                              // CHILD process
                close(tube[0]) ;
                dup2(tube[1], 1) ;
                close(tube[1]) ;
                if( execvp (argv[optind],argv+optind) == -1){
                    fprintf(stderr,"Exec error at iteration %d \n",n);
                    exit(1);
                }
                break ;
            default :                            // PARENT process
                close(tube[1]) ;
                get_output(tube[0],&curOutput,&curLength,&curReturn);
                close(tube[0]) ;

                print_time(timeFmt);
                print_output(prevOutput,curOutput,prevLenght,curLength);
                if(detectReturn == 1) print_exit(prevReturn,curReturn,n); // if user asked to detect return code
                
                // changing all previous values to current ones
                prevReturn = curReturn ;
                prevLenght = curLength ;
                prevOutput= curOutput;
                usleep (interval) ;
                n++ ;
                wait(NULL);
        }
    }

    free (prevOutput);
    return 0;
}
