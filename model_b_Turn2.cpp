#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdlib>
#include <ctime>

const int BUFFER_SIZE = 1024;
const int SAMPLE_RATE = 44100;
const int CHANNELS = 1;

class CircularBuffer {
public:
    CircularBuffer() : buffer(new int[BUFFER_SIZE]), write_pointer(0), read_pointer(0), available_samples(0) {
        mutex.lock();
    }
    ~CircularBuffer() {
        mutex.lock();
        available_samples = 0;
        write_pointer = read_pointer;
        mutex.unlock();
        delete[] buffer;
    }

    // Write audio data to the circular buffer
    void write(int sample) {
        std::unique_lock<std::mutex> lock(mutex);
        while (available_samples == BUFFER_SIZE) {
            not_full.wait(lock);
        }

        buffer[write_pointer] = sample;
        write_pointer = (write_pointer + 1) % BUFFER_SIZE;
        available_samples++;
        not_empty.notify_one();
    }

    // Read audio data from the circular buffer
    int read() {
        std::unique_lock<std::mutex> lock(mutex);
        while (available_samples == 0) {
            not_empty.wait(lock);
        }

        int sample = buffer[read_pointer];
        read_pointer = (read_pointer + 1) % BUFFER_SIZE;
        available_samples--;
        not_full.notify_one();
        return sample;
    }

    // Check if the buffer is full
    bool is_full() {
        std::unique_lock<std::mutex> lock(mutex);
        return available_samples == BUFFER_SIZE;
    }

    // Check if the buffer is empty
    bool is_empty() {
        std::unique_lock<std::mutex> lock(mutex);
        return available_samples == 0;
    }

private:
    int* buffer;
    int write_pointer;
    int read_pointer;
    int available_samples;

    std::mutex mutex;
    std::condition_variable not_empty;
    std::condition_variable not_full;
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
        if (!buffer->is_empty()) {
            int sample = buffer->read();
            std::cout << sample << " ";
        }
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

