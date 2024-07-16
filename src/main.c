#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include "subprocess.h"
#include <poll.h>


#define BUFFER_SIZE 128
#define GDB_CMD "arm-none-eabi-gdb"

enum channel {
    IN = 0,
    OUT,
    ERR,
    NONE
};


static void communicate(struct subprocess_s* sp, FILE* log)
{
    struct pollfd fds[3];
    ssize_t length;
    char buf[BUFFER_SIZE];
    enum channel reported_channel = NONE;
    FILE* sub_out = subprocess_stdout(sp);
    FILE* sub_err = subprocess_stderr(sp);
    FILE* sub_in = subprocess_stdin(sp);

    if((NULL == sub_out) || (NULL == sub_err) || (NULL == sub_in))
    {
        fprintf(stderr, "ERROR: streams are NULL!\n");
        fprintf(log, "ERROR: streams are NULL!\n");
        return;
    }

    memset(buf, 0, sizeof(buf));

    fds[0].fd = STDIN_FILENO;
    fds[0].events = POLLIN;
    fds[1].fd = fileno(sub_out);
    fds[1].events = POLLIN;
    fds[2].fd = fileno(sub_err);
    fds[2].events = POLLIN;


    while(1)
    {
        int ret;
        // wait for new data on stdin, sub_out and sub_err
        ret = poll(fds, 3, 10);
        if(-1 == ret)
        {
            fprintf(log, "ERROR: select() failed!\n");
            return;
        }
        if(0 == ret)
        {
            // timeout -> ignore
            fflush(stdout);
        }

        // stdin
        if (fds[0].revents & POLLERR)  // poll ERROR
        {
            fprintf(log, "ERROR: poll error on stdin\n");
            return;
        }
        if (fds[0].revents & POLLHUP)  // hang up
        {
            fprintf(log, "ERROR: hang up on stdin\n");
            return;
        }

        if (fds[0].revents & POLLNVAL)  // invalid request fd not open
        {
            fprintf(log, "ERROR: poll invalid on stdin\n");
            return;
        }

        // sub_out
        if (fds[1].revents & POLLERR)  // poll ERROR
        {
            fprintf(log, "ERROR: poll error on sub_out\n");
            return;
        }
        if (fds[1].revents & POLLHUP)  // hang up
        {
            fprintf(log, "INFO: hang up on sub_out\n");
            return;
        }

        if (fds[1].revents & POLLNVAL)  // invalid request fd not open
        {
            fprintf(log, "ERROR: poll invalid on sub_out\n");
            return;
        }

        // sub_err
        if (fds[2].revents & POLLERR)  // poll ERROR
        {
            fprintf(log, "ERROR: poll error on sub_err\n");
            return;
        }
        if (fds[2].revents & POLLHUP)  // hang up
        {
            fprintf(log, "ERROR: hang up on sub_err\n");
            return;
        }

        if (fds[2].revents & POLLNVAL)  // invalid request fd not open
        {
            fprintf(log, "ERROR: poll invalid on sub_err\n");
            return;
        }

        // stdin -> sub_in
        if (fds[0].revents & POLLIN)
        {
            // receives something on stdin -> sub_in
            do{
                length = read(STDIN_FILENO, buf, BUFFER_SIZE);
                if(0 < length)
                {
                    if(IN != reported_channel)
                    {
                        fprintf(log, "\nIN : ");
                        reported_channel = IN;
                    }
                    fwrite(buf, length, 1, log);
                    fwrite(buf, length, 1, sub_in);
                    fflush(log);
                    fflush(sub_in);
                }
            } while(BUFFER_SIZE == length);
        }

        // sub_out -> stdout
        if (fds[1].revents & POLLIN)
        {
            // receives something on sub_out -> stdout
            do{
                length = read(fileno(sub_out), buf, BUFFER_SIZE);
                if(0 < length)
                {
                    if(OUT != reported_channel)
                    {
                        fprintf(log, "\nOUT : ");
                        reported_channel = OUT;
                    }
                    fwrite(buf, length, 1, log);
                    fwrite(buf, length, 1, stdout);
                    fflush(log);
                    fflush(stdout);
                }
            }  while(BUFFER_SIZE == length);
        }

        // sub_err -> stderr
        if (fds[2].revents & POLLIN)
        {
            // receives something on sub_err -> stderr
            do{
                length = read(fileno(sub_err), buf, BUFFER_SIZE);
                if(0 < length)
                {
                    if(ERR != reported_channel)
                    {
                        fprintf(log, "\nERR : ");
                        reported_channel = ERR;
                    }
                    fwrite(buf, length, 1, log);
                    fwrite(buf, length, 1, stderr);
                    fflush(log);
                    fflush(stderr);
                }
            } while(BUFFER_SIZE == length);
        }
    }
}

int main(int argc, char * argv[])
{
    int i;
    struct subprocess_s gdb_sp;
    //  123456789012345678901234567890123
    // "24-07-11_12-23_31_wrapper_log.txt"
    char log_filename[40];

    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    memset(log_filename, 0, sizeof(log_filename));

    snprintf(log_filename, sizeof(log_filename),
        "%02d-%02d-%02d_%02d-%02d-%02d_wrapper_log.txt",
        (tm.tm_year%100), tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

    // open log file
    FILE* log = fopen(&log_filename[0], "w");
    if(NULL == log)
    {
        fprintf(stderr, "ERROR: can't open %s\n", &log_filename[0]);
        fprintf(log, "ERROR: can't open %s\n", &log_filename[0]);
        fclose(log);
        return -1;
    }

    fprintf(log, "gdb wrapper log file\n");

    // report command line parameters
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
        fprintf(stderr, "ERROR: can't open %s, check if its is installed\n", GDB_CMD);
        fprintf(log, "ERROR: can't open %s, check if its is installed\n", GDB_CMD);
        fclose(log);
        return -2;
    }

    communicate(&gdb_sp, log);

    int ret = -1;
    subprocess_join(&gdb_sp, &ret);
    if(ret != 0 ) {
        fprintf(stderr, "ERROR: could not join GDB process!\n");
        fprintf(log, "ERROR: could not join GDB process!\n");
        fclose(log);
        return -3;
    }

    assert(!subprocess_destroy(&gdb_sp));
    fprintf(log, "Done!\n");
    fclose(log);
    return EXIT_SUCCESS;
}
