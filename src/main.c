#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "subprocess.h"

#define BUFFER_SIZE 32
#define GDB_CMD "arm-none-eabi-gdb"


static void communicate(struct subprocess_s* sp)
{
    FILE* sub_out = subprocess_stdout(sp);
    FILE* sub_err = subprocess_stderr(sp);
    FILE* sub_in = subprocess_stdin(sp);


    //while(1)
    {
        // wait for new date on stdin, sub_out and sub_err
        // select();

        // std_in -> sub_in
        // sub_out -> std_out
        // sub_err -> std_err
    }

    /*
        fgets(stdout_buf, BUFFER_SIZE, p_stdout);
        printf("%s", stdout_buf);
        fgets(stderr_buf, BUFFER_SIZE, p_stdout);
        printf("%s", stderr_buf);

        // int a = fprintf(p_stdin, "-break-insert main");
        // printf("%d\n", a);

        fgets(stdout_buf, BUFFER_SIZE, p_stdout);
        printf("%s", stdout_buf);
    */
}

int main(int argc, char * argv[])
{
    int i;
    struct subprocess_s gdb_sp;
    //  123456789012345678901234567890123
    // "24-07-11_12-23_31_wrapper_log.txt"
    char log_filename[40];
    char stdout_buf[BUFFER_SIZE];
    char stderr_buf[BUFFER_SIZE];
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    memset(log_filename, 0, sizeof(log_filename));
    memset(stdout_buf, 0, sizeof(stdout_buf));
    memset(stderr_buf, 0, sizeof(stderr_buf));

    snprintf(log_filename, sizeof(log_filename),
        "%02d-%02d-%02d_%02d-%02d-%02d_wrapper_log.txt",
        (tm.tm_year%100), tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    // open log file
    FILE* log = fopen(&log_filename[0], "w");
    if(NULL == log)
    {
        fprintf(stderr, "Erro: can't open %s\n", &log_filename[0]);
        fprintf(log, "Erro: can't open %s\n", &log_filename[0]);
        fclose(log);
        return -1;
    }

    fprintf(log, "gdb wrapper log file\n");

    // report command line paraeters
    fprintf(log, "command line parameters:");
    for(i = 0; i < argc; i++)
    {
        fprintf(log, " %s", argv[i]);
    }
    fprintf(log, "\n");
    fflush(log);

    argv[0] = GDB_CMD;

    // printf("Starting new process\n");
    int gdb_status = subprocess_create((const char * const*)&(argv[0]),
                        subprocess_option_search_user_path
                        | subprocess_option_inherit_environment
                        // | subprocess_option_enable_async
                        , &gdb_sp);
    if(gdb_status != 0)
    {
        fprintf(stderr, "Erro: can't open %s, check if its is installed\n", GDB_CMD);
        fprintf(log, "Erro: can't open %s, check if its is installed\n", GDB_CMD);
        fclose(log);
        return -2;
    }

    communicate(&gdb_sp);

    int ret = -1;
    subprocess_join(&gdb_sp, &ret);
    if(ret != 0 ) {
        fprintf(stderr, "Erro: could not join GDB process!\n");
        fprintf(log, "Erro: could not join GDB process!\n");
        fclose(log);
        return -1;
    }

    assert(!subprocess_destroy(&gdb_sp));
    fprintf(log, "Done!\n");
    fclose(log);
    return EXIT_SUCCESS;
}
