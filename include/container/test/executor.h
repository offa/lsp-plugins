/*
 * executor.h
 *
 *  Created on: 11 февр. 2019 г.
 *      Author: sadko
 */

#ifndef INCLUDE_CONTAINER_TEST_EXECUTOR_H_
#define INCLUDE_CONTAINER_TEST_EXECUTOR_H_

#include <container/test/types.h>
#include <container/test/config.h>
#include <data/cstorage.h>
#include <errno.h>

#ifdef PLATFORM_WINDOWS
    #include <processthreadsapi.h>
#endif

#ifdef PLATFORM_UNIX_COMPATIBLE
    #include <unistd.h>
    #include <sys/wait.h>
    #include <sys/stat.h>
    #include <fcntl.h>
#endif /* PLATFORM_UNIX_COMPATIBLE */

#ifdef PLATFORM_LINUX
    #include <mcheck.h>
#endif /* PLATFORM_LINUX */

namespace lsp
{
    class TestExecutor
    {
        protected:
            typedef struct task_t
            {
#if defined(PLATFORM_WINDOWS)
                PROCESS_INFORMATION pid;
#else /* POSIX */
                pid_t               pid;
#endif /* PLATFORM_WINDOWS */
                struct timespec     submitted;
                test::Test         *test;
                status_t            result;
            } task_t;

        private:
            size_t              nTotal;
            size_t              nTasksMax;
            size_t              nTasksActive;
            double              fOverall;
            task_t             *vTasks;
            config_t           *pCfg;
            stats_t            *pStats;

        protected:
            status_t    launch_test(test::Test *test);
            status_t    wait_for_children();
            status_t    launch(test::UnitTest *test);
            status_t    launch(test::PerformanceTest *test);
            status_t    launch(test::ManualTest *test);

            // Platform-dependent routines
            status_t    submit_task(task_t *task);
            status_t    wait_for_child(task_t **task);
            status_t    set_timeout(double timeout);
            status_t    kill_timeout();
            void        start_memcheck(test::Test *test);
            void        end_memcheck();

        public:
            explicit TestExecutor()
            {
                nTotal          = 0;
                nTasksMax       = 0;
                nTasksActive    = 0;
                fOverall        = 0.0f;
                vTasks          = NULL;
                pCfg            = NULL;
                pStats          = NULL;
            }

            ~TestExecutor()
            {
                wait();
            }

        public:
            /**
             * Configure test launcher
             * @param config launcher configuration
             * @param stats pointer to statistics structure
             * @return status of operation
             */
            status_t init(config_t *config, stats_t *stats);

            /**
             * Wait for completion of all child processes
             * @return  status of operation
             */
            status_t wait();

            /**
             * Submit test for execution
             * @param test test for execution
             * @return status of operation
             */
            status_t submit(test::Test *test);
    };

    status_t TestExecutor::init(config_t *config, stats_t *stats)
    {
        if (config->fork)
        {
            size_t threads  = (config->threads > 0) ? config->threads : 1;
            if (config->mode != UTEST)
                threads         = 1;

            task_t *tasks   = new task_t[threads];
            if (tasks == NULL)
                return STATUS_NO_MEM;

            nTasksMax       = threads;
            nTasksActive    = 0;
            vTasks          = tasks;
        }
        else
        {
            nTasksMax       = 0;
            nTasksActive    = 0;
            vTasks          = NULL;
        }

        pCfg        = config;
        pStats      = stats;

        return STATUS_OK;
    }

    status_t TestExecutor::wait()
    {
        if (pCfg->is_child)
            return STATUS_OK;

        while (nTasksActive > 0)
        {
            status_t res    = wait_for_children();
            if (res != STATUS_OK)
                return res;
        }

        return STATUS_OK;
    }

    status_t TestExecutor::submit(test::Test *test)
    {
        const char *tclass  = (pCfg->mode == UTEST) ? "unit test" :
                              (pCfg->mode == PTEST) ? "performance test" :
                              "manual test";

        // Do we need to fork() ?
        if ((!pCfg->fork) || (vTasks == NULL) || (nTasksMax <= 0))
            return launch_test(test);

        // Wait for empty task descriptor
        while (nTasksActive >= nTasksMax)
        {
            status_t res    = wait_for_children();
            if (res != STATUS_OK)
                return res;
        }

        if (!pCfg->is_child)
        {
            printf("\n--------------------------------------------------------------------------------\n");
            printf("Launching %s '%s'\n", tclass, test->full_name());
            printf("--------------------------------------------------------------------------------\n");
        }

        fflush(stdout);
        fflush(stderr);

        // Allocate new task descriptor
        task_t *task        = &vTasks[nTasksActive++];
        clock_gettime(CLOCK_REALTIME, &task->submitted); // Remember start time of the test
        task->test          = test;
        task->result        = STATUS_OK;

        // Launch the nested process
        status_t res        = submit_task(task);
        if (res != STATUS_OK)
            --nTasksActive;
        return res;
    }

    status_t TestExecutor::wait_for_children()
    {
        struct timespec ts;
        const char *test    = (pCfg->mode == UTEST) ? "Unit test" :
                              (pCfg->mode == PTEST) ? "Performance test" :
                              "Manual test";

        // Try to wait for child task
        task_t *task        = NULL;
        status_t res        = wait_for_child(&task);
        if ((res != STATUS_OK) || (task == NULL))
            return res;

        // Get execution time
        clock_gettime(CLOCK_REALTIME, &ts);
        double time = (ts.tv_sec - task->submitted.tv_sec) + (ts.tv_nsec - task->submitted.tv_nsec) * 1e-9;
        printf("%s '%s' has %s, execution time: %.2f s\n",
                test, task->test->full_name(), (task->result == 0) ? "succeeded" : "failed", time);

        // Update statistics
        if (pStats != NULL)
        {
            if (task->result == STATUS_OK)
                pStats->success.add(task->test);
            else
                pStats->failed.add(task->test);
        }

        // Free task descriptor
        if (task < &vTasks[--nTasksActive])
            *task   = vTasks[nTasksActive];

        return STATUS_OK;
    }

    status_t TestExecutor::launch(test::UnitTest *test)
    {
        // Set-up timer for deadline expiration
        status_t res = STATUS_OK;
        if (!pCfg->debug)
            res = set_timeout(test->time_limit());

        // Launch unit test
        if (res == STATUS_OK)
        {
            config_t *cfg = const_cast<config_t *>(pCfg);

            test->set_verbose(pCfg->verbose);
            start_memcheck(test);
            test->execute(pCfg->args.size(), const_cast<const char **>(cfg->args.get_array()));
            end_memcheck();
        }

        // Cancel and disable timer
        if ((res == STATUS_OK) && (!pCfg->debug))
        {
            status_t res = kill_timeout();
            if (res != STATUS_OK)
                return res;
        }

        // Return success
        return STATUS_OK;
    }

    status_t TestExecutor::launch(test::PerformanceTest *test)
    {
        config_t *cfg = const_cast<config_t *>(pCfg);

        // Execute performance test
        test->set_verbose(pCfg->verbose);
        start_memcheck(test);
        test->execute(pCfg->args.size(), const_cast<const char **>(cfg->args.get_array()));
        end_memcheck();

        // Output peformance test statistics
        printf("\nStatistics of performance test '%s':\n", test->full_name());
        test->dump_stats(stdout);

        // Additionally dump performance statistics to output file
        if (pCfg->outfile != NULL)
        {
            FILE *fd = fopen(pCfg->outfile, "a");
            if (fd != NULL)
            {
                fprintf(fd, "--------------------------------------------------------------------------------\n");
                fprintf(fd, "Statistics of performance test '%s':\n\n", test->full_name());
                test->dump_stats(fd);
                fprintf(fd, "\n");
                fflush(fd);
                fclose(fd);
            }
        }

        test->free_stats();

        return STATUS_OK;
    }

    status_t TestExecutor::launch(test::ManualTest *test)
    {
        config_t *cfg = const_cast<config_t *>(pCfg);
        // Execute performance test
        test->set_verbose(pCfg->verbose);
        start_memcheck(test);
        test->execute(pCfg->args.size(), const_cast<const char **>(cfg->args.get_array()));
        end_memcheck();

        return STATUS_OK;
    }

    status_t TestExecutor::launch_test(test::Test *test)
    {
        switch (pCfg->mode)
        {
            case UTEST:
                return launch(static_cast<test::UnitTest *>(test));
            case PTEST:
                return launch(static_cast<test::PerformanceTest *>(test));
            case MTEST:
                return launch(static_cast<test::ManualTest *>(test));
            default:
                break;
        }
        return STATUS_BAD_STATE;
    }

#ifdef PLATFORM_WINDOWS
    // TODO
#endif /* PLATFORM_WINDOWS */

#ifdef PLATFORM_LINUX
    void TestExecutor::start_memcheck(test::Test *v)
    {
        if (!pCfg->mtrace)
            return;

        // Enable memory trace
        char fname[PATH_MAX];
        mkdir(pCfg->tracepath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        snprintf(fname, PATH_MAX, "%s/%s.utest.mtrace", pCfg->tracepath, v->full_name());
        fname[PATH_MAX-1] = '\0';

        fprintf(stderr, "Enabling memory trace for test '%s' into file '%s'\n", v->full_name(), fname);
        fflush(stderr);

        setenv("MALLOC_TRACE", fname, 1);

        mtrace();
    }

    void TestExecutor::end_memcheck()
    {
        // Disable memory trace
        if (pCfg->mtrace)
            muntrace();

        // Validate heap
//        mcheck_check_all();
    }
#else
    void TestExecutor::start_memcheck(test::Test *v)
    {
    }

    void TestExecutor::end_memcheck()
    {
    }
#endif /* PLATFORM_LINUX */

#ifdef PLATFORM_UNIX_COMPATIBLE
    void utest_timeout_handler(int signum)
    {
        fprintf(stderr, "Unit test time limit exceeded\n");
        exit(STATUS_TIMEOUT);
    }

    status_t TestExecutor::submit_task(task_t *task)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            pCfg->is_child  = true;
            return launch_test(task->test);
        }
        else if (pid < 0)
        {
            int error = errno;
            fprintf(stderr, "Error while spawning child process %d\n", error);
            fflush(stderr);
            return STATUS_UNKNOWN_ERR;
        }

        task->pid       = pid;
        return STATUS_OK;
    }

    status_t TestExecutor::wait_for_child(task_t **task)
    {
        task_t *ret       = NULL;
        int result;

        do
        {
            pid_t pid = waitpid(-1, &result, WUNTRACED | WCONTINUED);
            if (pid < 0)
            {
                fprintf(stderr, "Child process completion wait failed\n");
                return STATUS_UNKNOWN_ERR;
            }

            // Find the associated thread process
            ret       = NULL;
            for (size_t i=0; i<nTasksActive; ++i)
                if (vTasks[i].pid == pid)
                {
                    ret   = &vTasks[i];
                    break;
                }

            if (WIFSTOPPED(result))
                printf("Child process %d stopped by signal %d\n", int(pid), WSTOPSIG(result));
        } while ((!WIFEXITED(result)) && !WIFSIGNALED(result));

        if (WIFEXITED(result))
            ret->result   = WEXITSTATUS(result);
        else if (WIFSIGNALED(result))
            ret->result   = STATUS_KILLED;

        *task   = ret;
        return STATUS_OK;
    }

    status_t TestExecutor::set_timeout(double timeout)
    {
        struct itimerval timer;

        timer.it_interval.tv_sec    = timeout;
        timer.it_interval.tv_usec   = suseconds_t(timeout * 1e+6) % 1000000L;
        timer.it_value              = timer.it_interval;

        status_t res                = STATUS_OK;
        if (setitimer(ITIMER_REAL, &timer, NULL) != 0)
        {
            int code = errno;
            fprintf(stderr, "setitimer failed with errno=%d\n", code);
            fflush(stderr);
            res = STATUS_UNKNOWN_ERR;
        }
        signal(SIGALRM, utest_timeout_handler);

        return res;
    }

    status_t TestExecutor::kill_timeout()
    {
        struct itimerval timer;

        timer.it_interval.tv_sec    = 0;
        timer.it_interval.tv_usec   = 0;
        timer.it_value              = timer.it_interval;

        signal(SIGALRM, SIG_DFL);
        if (setitimer(ITIMER_REAL, &timer, NULL) == 0)
            return STATUS_OK;

        int code = errno;
        fprintf(stderr, "setitimer failed with errno=%d\n", code);
        fflush(stderr);
        return STATUS_UNKNOWN_ERR;
    }

#endif /* PLATFORM_UNIX_COMPATIBLE */
}


#endif /* INCLUDE_CONTAINER_TEST_EXECUTOR_H_ */
