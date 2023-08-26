#include "ThreadPool.h"
#include <memory>
#include <iostream>


bool make_thread = true;

void quicksort(std::shared_ptr<ThreadPool> pool, int* array, long left, long right) {
    if (left >= right) {
        return;
    }

    long left_bound = left;
    long right_bound = right;
    long middle = array[(left_bound + right_bound) / 2];

    do {
        while (array[left_bound] < middle) {
            left_bound++;
        }
        while (array[right_bound] > middle) {
            right_bound--;
        }

        if (left_bound <= right_bound) {
            std::swap(array[left_bound], array[right_bound]);
            left_bound++;
            right_bound--;
        }
    } while (left_bound <= right_bound);

    if (make_thread && (right_bound - left > 10000)) {
        if ((right_bound - left_bound) > 100000) {
            auto promisePtr = std::make_shared<std::promise<void>>();
            auto f = promisePtr->get_future();

            auto task = [&, promisePtr]() {
                quicksort(pool, array, left, right_bound);
                promisePtr->set_value();
            };

            pool->push_task(task);
            quicksort(pool, array, left_bound, right);

            f.wait();
        }
        else {
            quicksort(pool, array, left, right_bound);
            quicksort(pool, array, left_bound, right);
        }
    }
    else {
        quicksort(pool, array, left, right_bound);
        quicksort(pool, array, left_bound, right);
    }
}

int main(int argc, char* argv[]) {
    srand(0);
    long arr_size = 100000000;
    int* array = new int[arr_size];

    for (long i = 0; i < arr_size; i++) {
        array[i] = rand() % 500000;
    }

    auto pool = std::make_shared<ThreadPool>();

    time_t start, end;

    // Многопоточный запуск
    time(&start);
    quicksort(pool, array, 0, arr_size);
    time(&end);

    double seconds = difftime(end, start);
    printf("The time: %f seconds\n", seconds);

    for (long i = 0; i < arr_size - 1; i++) {
        if (array[i] > array[i + 1]) {
            std::cout << "Unsorted" << std::endl;
            break;
        }
    }

    for (long i = 0; i < arr_size; i++) {
        array[i] = rand() % 500000;
    }

    // Однопоточный запуск
    make_thread = false;
    time(&start);
    quicksort(pool, array, 0, arr_size);
    time(&end);

    seconds = difftime(end, start);
    printf("The time: %f seconds\n", seconds);
    
    return 0;
}
