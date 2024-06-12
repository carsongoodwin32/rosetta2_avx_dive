#include <iostream>
#include <vector>
#include <cstdlib> // Include for srand() and rand()

const int VECTOR_SIZE = 800000000;

// Generate a vector of random integers
void generate_int_vector(std::vector<int>& vec) {
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (auto& val : vec) {
        val = rand();
    }
}

// Sum of integers using AVX2 (256-bit integer operations)
int sum_int_avx2(const std::vector<int>& a) {
    int sum[8] = {0, 0, 0, 0, 0, 0, 0, 0};
    __asm__ __volatile__ (
        "vpxor %%ymm1, %%ymm1, %%ymm1\n\t" // Initialize sum to zero
        :
        :
        : "%ymm1"
    );
    for (int i = 0; i < VECTOR_SIZE; i += 8) {
        __asm__ __volatile__ (
            "vmovdqu (%0), %%ymm0\n\t"
            "vpaddd %%ymm0, %%ymm1, %%ymm1\n\t"
            :
            : "r" (&a[i])
            : "%ymm0", "%ymm1"
        );
    }
    __asm__ __volatile__ (
        "vmovdqu %%ymm1, %0\n\t"
        : "=m" (sum)
        :
        : "%ymm1"
    );
    return sum[0] + sum[1] + sum[2] + sum[3] + sum[4] + sum[5] + sum[6] + sum[7];
}

int main() {
    std::vector<int> c(VECTOR_SIZE);
    generate_int_vector(c);
    int result_int_avx2 = sum_int_avx2(c);
    return result_int_avx2;
}