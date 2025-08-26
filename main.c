#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FILE *fp;
#define LINE_SIZE 1024

int main(int argc, char const *argv[]) {

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







    
    fclose(fp);

    return 0;
}