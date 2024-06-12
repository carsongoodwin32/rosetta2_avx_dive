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

// Sum of integers using AVX (128-bit integer operations)
int sum_int_avx(const std::vector<int>& a) {
    int sum[4] = {0, 0, 0, 0};
    __asm__ __volatile__ (
        "vpxor %%xmm1, %%xmm1, %%xmm1\n\t" // Initialize sum to zero
        :
        :
        : "%xmm1"
    );
    for (int i = 0; i < VECTOR_SIZE; i += 4) {
        __asm__ __volatile__ (
            "vmovdqu (%0), %%xmm0\n\t"
            "vpaddd %%xmm0, %%xmm1, %%xmm1\n\t"
            :
            : "r" (&a[i])
            : "%xmm0", "%xmm1"
        );
    }
    __asm__ __volatile__ (
        "vmovdqu %%xmm1, %0\n\t"
        : "=m" (sum)
        :
        : "%xmm1"
    );
    return sum[0] + sum[1] + sum[2] + sum[3];
}

int main() {
    std::vector<int> c(VECTOR_SIZE);
    generate_int_vector(c);
    int result_int_avx = sum_int_avx(c);
    return result_int_avx;
}