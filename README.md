# OS Lab Projects

This repository contains implementations of four operating system concepts:

## 1. Simple Shell Implementation

A custom shell implementation that handles:
- Built-in commands:
  - `cd`: Change directory
  - `echo`: Display arguments
  - `export`: Set environment variables
  - Other common shell commands
- External command execution
- Background command processing (`&` syntax)

**Implementation Notes:**
- Uses fork/exec for external commands
- Handles environment variables
- Implements basic job control for background processes

## 2. Threaded Matrix Operations

Demonstrates different threading approaches for matrix operations:
1. Whole matrix processed by single thread
2. Row-wise threading (each thread handles a row)
3. Element-wise threading (each thread handles an element)

Key Features:
- Performance comparison between approaches
- Time measurement for each implementation
- Example operations: matrix multiplication, addition, etc.

## 3. Mutual Exclusion with Train Example

Illustrates mutex synchronization through a train station simulation:
- Passengers wait for train arrival (blocking)
- Train arrives and broadcasts to waiting passengers
- Controlled boarding using mutexes
- Multiple synchronization scenarios:
  - Single train, multiple passengers
  - Capacity constraints
  - Multiple trains scenario

## 4. RedLock Distributed Locking Example

Implementation of the RedLock algorithm using:
- 5 Docker containers as Redis nodes
- Distributed lock acquisition requiring majority (≥3 nodes)
- Lock release/retry mechanism if acquisition fails

Implementation Details:
- Docker setup scripts for Redis nodes
- Client implementation demonstrating lock acquisition
- Handling of network partitions and failures
- Exponential backoff for retry mechanism

## Setup and Usage

Each project has its own directory with detailed build and run instructions. General requirements:

- Linux environment
- GCC for C programs
- Docker for RedLock example
- Python (some examples may use Python)

See individual project READMEs for specific requirements and usage examples.
