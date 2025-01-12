#include <iostream>
#include <thread>
#include <cstdlib>  // For rand() and srand()
#include <ctime>   // For seeding rand()
#include <chrono>
#include <mutex>
#include <condition_variable>

const int BUFFER_SIZE = 1024;   // Size of the circular buffer

// Circular Buffer class
class CircularBuffer {
public:
    CircularBuffer() : buffer(new int[BUFFER_SIZE]), write_pointer(0), read_pointer(0) {}
    ~CircularBuffer() { delete[] buffer; }

    // Write audio data to the circular buffer
    void write(int sample) {
        std::unique_lock<std::mutex> lock(mutex_);  // Lock the mutex
        // Wait until there is space in the buffer
        while ((write_pointer + 1) % BUFFER_SIZE == read_pointer) {
            not_full_.wait(lock);  // Wait until the buffer is not full
        }
        buffer[write_pointer] = sample;
        write_pointer = (write_pointer + 1) % BUFFER_SIZE;

        // Notify one waiting reader that data is available
        not_empty_.notify_one();
    }

    // Read audio data from the circular buffer
    int read() {
        std::unique_lock<std::mutex> lock(mutex_);  // Lock the mutex
        // Wait until there is data in the buffer
        while (is_empty()) {
            not_empty_.wait(lock);  // Wait until the buffer is not empty
        }
        int sample = buffer[read_pointer];
        read_pointer = (read_pointer + 1) % BUFFER_SIZE;

        // Notify one waiting writer that space is available
        not_full_.notify_one();

        return sample;
    }

    // Check if the buffer is empty
    bool is_empty() {
        return write_pointer == read_pointer;
    }

private:
    int* buffer;                            // Pointer to the buffer array
    int write_pointer;                      // Write pointer
    int read_pointer;                       // Read pointer
    std::mutex mutex_;                      // Mutex for thread safety
    std::condition_variable not_empty_;     // Condition variable for not empty
    std::condition_variable not_full_;      // Condition variable for not full
};

// Thread function to generate and write random audio data
void generate_and_write_audio(CircularBuffer* buffer) {
    srand(static_cast<unsigned>(std::time(0)));  // Seed rand()

    while (true) {
        int sample = rand() % 32768;  // Generate a random 16-bit sample
        buffer->write(sample);
    }
}

// Thread function to read and print audio data
void read_and_print_audio(CircularBuffer* buffer) {
    while (true) {
        int sample = buffer->read();
        std::cout << sample << " ";
    }
}

int main() {
    CircularBuffer audio_buffer;

    std::thread writer_thread(generate_and_write_audio, &audio_buffer);
    std::thread reader_thread(read_and_print_audio, &audio_buffer);

    // Run the threads indefinitely (you should add proper termination logic in a real application)
    writer_thread.join();
    reader_thread.join();

    return 0;
}
