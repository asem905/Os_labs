import redis
import time
import uuid
import multiprocessing
import logging
from random import random
logging.basicConfig(level=logging.INFO, format="%(asctime)s - %(levelname)s - %(message)s",
                    filename="debug.log")


client_processes_waiting = [0, 1, 1, 1, 4]
class Redlock:
    def __init__(self, redis_nodes):
        """
        Initialize Redlock with a list of Redis node addresses.
        :param redis_nodes: List of (host, port) tuples.
        """
        self.nodes = []
        db = 0
        for host, port in redis_nodes:
            self.nodes.append(redis.Redis(host=host, port=port, db=db, socket_timeout=0.5, decode_responses=True))
            db += 1

    def acquire_lock(self, resource, ttl, max_retries=50):
        """
        Try to acquire a distributed lock with a retry limit.
        :max_retries: Maximum number of retry attempts.
      param resource: The name of the resource to lock.
        :param ttl: Time-to-live for the lock in milliseconds.
        :param   :return: Tuple (lock_acquired, lock_id).
        """
        retries = 0
        while retries < max_retries:
            start_time = time.time()
            val = str(uuid.uuid4())  # Generate a unique lock ID
            locks_acquired = 0
            for node in self.nodes:
                try:
                    acquired = node.set(resource, val, nx=True, px=ttl)#nx indicates that the lock should only be set if it does not already exist and px sets the expiration time in milliseconds
                    if acquired:
                        locks_acquired += 1
                except redis.exceptions.ConnectionError:
                    logging.error(f"Failed to connect to node {node}.")
                except redis.exceptions.TimeoutError:
                    logging.error(f"Timeout while acquiring lock on node {node}.")
                except Exception as e:
                    logging.error(f"Unexpected Error on node {node}: {e}")

            time_elapsed = (time.time() - start_time) * 1000 #(time.time() - start_time) gives seconds so convert ot milliseconds

            if locks_acquired >= (len(self.nodes) // 2 + 1) and (time_elapsed <= ttl):
                logging.info(f"Lock acquired with ID: {val}")
                return True, val
            else:
                logging.warning(f"Failed to acquire lock. Retrying... (Attempt {retries + 1}/{max_retries})")
                time.sleep(random() * 4 + 1) # Random sleep between 1 and 5 seconds
                # Release any locks that were acquired
                retries += 1
                self.release_lock(resource, val)

        logging.error(f"Failed to acquire lock after {max_retries} retries.")
        return False, None

    def release_lock(self, resource, lock_id):
        """
        Release the distributed lock only if it is still valid.
        :param resource: The name of the resource to unlock.
        :param lock_id: The unique lock ID to verify ownership.
        """
        for node in self.nodes:
            try:
                current_lock_id = node.get(resource)
                if current_lock_id == lock_id:
                    node.delete(resource)
                    logging.info(f"Lock {lock_id} released on node {node}.")
                else:
                    logging.warning(f"Lock {lock_id} is no longer valid on node {node}.")
            except redis.exceptions.ConnectionError:
                logging.error(f"Failed to connect to node {node}.")


def client_process(redis_nodes, resource, ttl, client_id):
    """
    Function to simulate a single client process trying to acquire and release a lock.
    """
    time.sleep(client_processes_waiting[client_id])

    redlock = Redlock(redis_nodes)
    print(f"\nClient-{client_id}: Attempting to acquire lock...") 
    lock_acquired, lock_id = redlock.acquire_lock(resource, ttl)

    if lock_acquired:
        print(f"\nClient-{client_id}: Lock acquired! Lock ID: {lock_id}")
        # Simulate critical section
        time.sleep(3)  # Simulate some work
        redlock.release_lock(resource, lock_id)
        print(f"\nClient-{client_id}: Lock released!")
    else:
        print(f"\nClient-{client_id}: Failed to acquire lock.")

if __name__ == "__main__":
    # Define Redis node addresses (host, port)
    redis_nodes = [
        ("localhost", 63791),
        ("localhost", 63792),
        ("localhost", 63793),
        ("localhost", 63794),
        ("localhost", 63795),
    ]

    resource = "shared_resource"
    ttl = 5000  # Lock TTL in milliseconds (5 seconds)

    # Number of client processes
    num_clients = 5

    # Start multiple client processes
    processes = []
    for i in range(num_clients):
        process = multiprocessing.Process(target=client_process, args=(redis_nodes, resource, ttl, i))
        processes.append(process)
        process.start()

    for process in processes:
        process.join()
