#include <iostream>
#include <vector>
#include <chrono>
#include <cstdlib> // Include for srand() and rand()
#include <time.h>

const int VECTOR_SIZE = 800000000;
const int NUM_RUNS = 10;

// Generate a vector of random integers
void generate_int_vector(std::vector<int>& vec) {
    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (auto& val : vec) {
        val = rand();
    }
}

// Sum of integers using SSE2
int sum_int_sse2(const std::vector<int>& a) {
    int sum[4] = {0, 0, 0, 0};
    __asm__ __volatile__ (
        "pxor %%xmm1, %%xmm1\n\t" // Initialize sum to zero
        :
        :
        : "%xmm1"
    );
    for (int i = 0; i < VECTOR_SIZE; i += 4) {
        __asm__ __volatile__ (
            "movdqu (%0), %%xmm0\n\t"
            "paddd %%xmm0, %%xmm1\n\t"
            :
            : "r" (&a[i])
            : "%xmm0", "%xmm1"
        );
    }
    __asm__ __volatile__ (
        "movdqu %%xmm1, %0\n\t"
        : "=m" (sum)
        :
        : "%xmm1"
    );
    return sum[0] + sum[1] + sum[2] + sum[3];
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

    double total_diff_sse2_avx = 0;
    double total_diff_sse2_avx2 = 0;
    double total_diff_avx_avx2 = 0;

    for (int run = 0; run < NUM_RUNS; ++run) {
        generate_int_vector(c);

        std::cout << "Run "<< run+1 << ":"<< std::endl;

        auto start = std::chrono::high_resolution_clock::now();
        int result_int_sse2 = sum_int_sse2(c);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_int_sse2 = end - start;
        std::cout << "SSE2 Int Sum Result: " << result_int_sse2 << " Time: " << elapsed_int_sse2.count() << " seconds" << std::endl;

        start = std::chrono::high_resolution_clock::now();
        int result_int_avx = sum_int_avx(c);
        end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_int_avx = end - start;
        std::cout << "AVX Int Sum Result: " << result_int_avx << " Time: " << elapsed_int_avx.count() << " seconds" << std::endl;

        start = std::chrono::high_resolution_clock::now();
        int result_int_avx2 = sum_int_avx2(c);
        end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed_int_avx2 = end - start;
        std::cout << "AVX2 Int Sum Result: " << result_int_avx2 << " Time: " << elapsed_int_avx2.count() << " seconds" << std::endl << std::endl;

        // Calculate and accumulate percentage differences
        double percent_diff_sse2_avx = ((elapsed_int_sse2.count() - elapsed_int_avx.count()) / elapsed_int_sse2.count()) * 100;
        double percent_diff_sse2_avx2 = ((elapsed_int_sse2.count() - elapsed_int_avx2.count()) / elapsed_int_sse2.count()) * 100;
        double percent_diff_avx_avx2 = ((elapsed_int_avx.count() - elapsed_int_avx2.count()) / elapsed_int_avx.count()) * 100;

        total_diff_sse2_avx += percent_diff_sse2_avx;
        total_diff_sse2_avx2 += percent_diff_sse2_avx2;
        total_diff_avx_avx2 += percent_diff_avx_avx2;
    }

    // Calculate averages
    double avg_diff_sse2_avx = total_diff_sse2_avx / NUM_RUNS;
    double avg_diff_sse2_avx2 = total_diff_sse2_avx2 / NUM_RUNS;
    double avg_diff_avx_avx2 = total_diff_avx_avx2 / NUM_RUNS;
    std::cout << "---------------------------Average of 10 Runs-------------------------------" << std::endl;
    std::cout << "SSE2 vs AVX: " << avg_diff_sse2_avx << "% runtime difference"<< std::endl;
    std::cout << "SSE2 vs AVX2: " << avg_diff_sse2_avx2 << "% runtime difference"<< std::endl;
    std::cout << "AVX vs AVX2: " << avg_diff_avx_avx2 << "% runtime difference"<< std::endl;


    return 0;
}