#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>

FILE *fp;
#define LINE_SIZE 1024

#define THREAD_COUNT 10
pthread_t threads[THREAD_COUNT];
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t totals_lock = PTHREAD_MUTEX_INITIALIZER;

int total_pcpu = 0;
int total_pmem = 0;
int total_rss = 0;
int total_vsz = 0;

// Signal handler for cleanup
void signalHandler(int sig)
{
	printf("Caught signal %d, terminating...\n", sig);
	if (fp != NULL)
	{
		fclose(fp);
		fp = NULL;
	}

	for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		if (threads[i] != 0)
		{
			pthread_cancel(threads[i]);
		}
	}

	pthread_mutex_destroy(&file_mutex);
	pthread_mutex_destroy(&totals_lock);

	exit(sig);
}

void *process_line(void *arg) {
    char line[LINE_SIZE];

    //preventing race condition on file reading
    pthread_mutex_lock(&file_mutex);
    if (fgets(line, sizeof(char) * LINE_SIZE, fp) == NULL)
    {
        pthread_mutex_unlock(&file_mutex);
    }
    pthread_mutex_unlock(&file_mutex);


    //tokenizing the line
    char *pid = strtok(line, " ");
	char *pcpu = strtok(NULL, " ");
	char *pmem = strtok(NULL, " ");
	char *rss = strtok(NULL, " ");
	char *vsz = strtok(NULL, " ");
	char *stat = strtok(NULL, " ");
	char *ppid = strtok(NULL, " \n");

    //calculating memory pages
    int page_size = getpagesize();
	long rss_pages_kb = atol(rss) / (page_size / 1024);
	long vsz_kb = atol(vsz) / (page_size / 1024);
	printf("Stats for pid %d: RSS pages: %ld VSZ pages: %ld\n", atoi(pid), rss_pages_kb, vsz_kb);


    //preventing race condition on updating totals
    pthread_mutex_lock(&totals_lock);
    total_pcpu += atoi(pcpu);
    total_pmem += atoi(pmem);
    total_rss += atoi(rss);
    total_vsz += atoi(vsz);
    pthread_mutex_unlock(&totals_lock);

    return NULL;
}

int main(int argc, char const *argv[]) {

    // Setting up signal handler
    signal(SIGINT, signalHandler);

    // Check for process file argument
    if (argc < 2)
	{
		printf("Process file required.\n");
		return 1;
	}
    
    fp = fopen(argv[1], "r");

    // Check if file opened successfully
    if (!fp) {
        perror("Failed to open file");
        return 1;
    }

    // Skip header line
	char header[LINE_SIZE];
	if (fgets(header, sizeof(char) * LINE_SIZE, fp) == NULL)
	{
		perror("Failed to read header line");
		return 1;
	}

    // Creating thread
    printf("Current top 10 resource metrics:\n");
    for (size_t i = 0; i < THREAD_COUNT; i++)
    {
        if (pthread_create(&threads[i], NULL, process_line, (void *)i) != 0)
        {
            perror("Failed to create thread");
            return 1;
        }
    }

    // Joining threads
    for (size_t i = 0; i < THREAD_COUNT; i++)
	{
		if (pthread_join(threads[i], NULL) != 0)
		{
			perror("Failed to join thread");
			return 1;
		}
	}
    printf("SUM CPU%%: %d\nSUM MEM%%: %d\nRSS AVERAGE: %d\nVSS AVERAGE: %d\n", total_pcpu, total_pmem, total_rss / THREAD_COUNT, total_vsz / THREAD_COUNT);

    // Cleaning up
    fclose(fp);
    pthread_mutex_destroy(&file_mutex);
	pthread_mutex_destroy(&totals_lock);

    return 0;
}