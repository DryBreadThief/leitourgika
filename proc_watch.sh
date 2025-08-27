#!/bin/bash

cleanup() {
	echo "Caught SIGINT, terminating..."
	pkill -INT -f "./main processes.txt"
	rm processes.txt
    exit 1
}
trap cleanup SIGINT

# Retrieve top 10 processes
echo "Retrieving top 10 processes"
processes=$(ps -eo pid,pcpu,pmem,rss,vsize,stat,ppid --sort -%cpu | head -11)

# Save processes to a file
echo "$processes" | sed 's/  */ /g' | sed 's/^ *//' > processes.txt

# Process each line
echo "$processes" | while read -r line; do
    #skip header
    if [[ "$line" == PID* ]]; then
        continue
    fi

    # Get process status and parent process ID
    stat=$(echo "$line" | awk '{print $6}')
    ppid=$(echo "$line" | awk '{print $7}')

    # Check if the process is a zombie
    if [[ "$stat" == *Z* ]]; then
        echo "ZOMBIE PROCESS ALARM: parent process to kill: $ppid"
		kill -KILL $ppid
    fi

done 

gcc main.c -o main
./main processes.txt