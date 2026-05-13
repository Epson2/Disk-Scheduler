# Disk-Scheduler
A high-performance disk scheduling simulator written in C that implements multiple classic operating system disk scheduling algorithms. The simulator processes disk I/O requests, determines servicing order based on the selected algorithm, and calculates total seek distance and completion time.

# Project Description

This project simulates how an operating system manages disk I/O requests using different scheduling strategies. Given a sequence of requests with arrival times and target tracks, the simulator determines the order in which requests are serviced while tracking:

- Total seek distance
- Total completion time
- Disk head movement
- Request servicing order

# The project demonstrates key operating system concepts including:

- Disk scheduling
- Seek optimization
- Data structures
- Scheduling fairness
- Performance tradeoffs
- Supported Algorithms
# Algorithm	Description
- FCFS:	Services requests in arrival order
- SSTF:	Services the closest pending request
- SCAN:	Moves like an elevator across the disk
- C-SCAN:	Circular SCAN that wraps around
- LOOK:	SCAN without traveling to disk edges
- C-LOOK:	Circular LOOK that wraps to pending requests

# Features
- ⚡ Efficient Treap-based request management
- 📀 Simulates realistic disk head movement
- 🧠 Supports directional scheduling algorithms
- 📈 Calculates total seek distance and completion time
- ⏱ Handles dynamic request arrivals over time
- 🛠 Written in clean modular C
- 📚 Extensive inline comments explaining logic
- 🚀 Optimized for fast insertion and lookup operations
  
# Tech Stack
- Language: C
- Concepts: Operating Systems, Scheduling Algorithms, Data Structures
- Data Structure: Treap (Randomized Balanced BST)
- Compiler: GCC / Clang

# Input Format

Example:

1. FCFS 100 50
2. 3
3. 5 55
4. 5 40
5. 20 60

# Explanation:

- Algorithm DiskSize InitialHead
- NumberOfRequests
- ArrivalTime Track
- ArrivalTime Track
...
# Example Output

1. 55 40 60
2. 30
3. 30

# Where:

- First line = servicing order
- Second line = total seek distance
- Third line = total completion time
