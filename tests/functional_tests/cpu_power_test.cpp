// Copyright (c) 2014-2022, The Monero Project
// 
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are
// permitted provided that the following conditions are met:
// 
// 1. Redistributions of source code must retain the above copyright notice, this list of
//    conditions and the following disclaimer.
// 
// 2. Redistributions in binary form must reproduce the above copyright notice, this list
//    of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 
// 3. Neither the name of the copyright holder nor the names of its contributors may be
//    used to endorse or promote products derived from this software without specific
//    prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
// THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

const char * descr = 
"This program prints the time (ms) needed to calculate a mathematical challenge. It's purpose \n\
is to be able to use the result to compare the CPU power available on the machine it's being executed \n\
and under the given circumstances, which can differ for the same machine. For example: \n\
The printed value will be different when using 2 of 2 cores, when there an another process \n\
running on one of the cores core, and when no other intense process running. \n\
 \n\
The program expects a one argument: a numerical value of the threads to start the calculations on. \n\
 \n\
Prints: \n\
Time to calculate a mathematical challenge in MILLISECONDS. \n\
 \n\
Returns: \n\
0 on success, \n\
1 on a missing argument, \n\
2 on an incorrect format of the argument.\n";

#include <iostream>
#include <future>
#include <vector>
#include <sstream>
#include <chrono>

using namespace std;

/**
Uses Leibniz Formula for Pi.
https://en.wikipedia.org/wiki/Leibniz_formula_for_%CF%80
*/
static double calcPi(const size_t max_iter)
{
    const double n = 4;
    double pi = 0;
    double d = 1;
    for (size_t i = 1; i < max_iter; ++i)
    {
        const double a = 2.0 * (i % 2) - 1.0;
        pi += a * n / d;
        d += 2;
    }
    return pi;
}

int main(int argc, const char ** argv)
{
    const size_t max_iter = 1e9;
    if (argc < 2)
    {
        cout << "Please pass the number of threads to run.\n";
        
        cout << '\n' << descr << '\n';
        return 1;
    }
    
    // Convert argument to an integer.
    int numThreads = 1;
    const char * numThreadsStr = argv[1];
    std::istringstream iss(numThreadsStr);
    if (! (iss >> numThreads) )
    {
        cout << "Incorrect format of number of threads = '" << numThreadsStr << "'\n";
        return 2;
    }
    // Run the calculation in parallel.
    std::vector<std::future<double>> futures;
    for(int i = 0; i < numThreads; ++i) 
    {
        futures.push_back(std::async(calcPi, max_iter));
    }    
    
    // Start measuring the time.
    const std::chrono::steady_clock::time_point tbegin = std::chrono::steady_clock::now();
    for(auto & e : futures) 
    {
       e.get();
    }
    // Stop measuring the time.
    const std::chrono::steady_clock::time_point tend = std::chrono::steady_clock::now();
    
    // Print the measured duration.
    cout << std::chrono::duration_cast<std::chrono::milliseconds>(tend - tbegin).count() << std::endl;
    return 0;
}

