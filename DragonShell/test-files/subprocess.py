import os
import time

def main():
    # Print message indicating subprocess is running
    print("Running subprocess")

    # Open a file to write factors to
    try:
        with open("test-files/factors.txt", "w") as file:
            # Loop through numbers to find factors
            for i in range(1, 100000):  # Start from 1 to avoid division by zero
                for j in range(1, i + 1):  # Change condition to j <= i for valid factors
                    if i % j == 0:
                        file.write(f"Found a factor of i for i = {i}, j = {j}\n")
                        file.flush()  # Ensure the output is written immediately

                        # Optionally, sleep to simulate longer processing time
                        # time.sleep(0.0001)  # Sleep for 100 microseconds
    except IOError as e:
        print(f"Failed to open file: {e}")
        return os.EX_OSFILE  # Return appropriate error code

    return 0

if __name__ == "__main__":
    main()