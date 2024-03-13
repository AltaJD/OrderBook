#include <iostream>
#include <string>
#include <chrono>
#include "data_extractor.hpp"

int main()
{
    // full path to the file should be indicated
    std::string file = "C:\\Users\\Alta_\\Desktop\\scandi.csv";
    std::string destination_file = "C:\\Users\\Alta_\\Desktop\\data.csv";
    int data_limit = 80000; // provide the limit for number of rows for testing

    // Start the timer
    auto start = std::chrono::high_resolution_clock::now();

    DataParser parser = DataParser(file, data_limit);
    parser.test_start();
    parser.show_summary();
    parser.save_orders(destination_file);


    // Stop the timer
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Execution time: " << duration.count() << " milliseconds" << std::endl;
    return 0;
}