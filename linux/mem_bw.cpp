#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <chrono>
#include <iostream>
#include <random>
#define ARRAY_SIZE (1024 * 1024 * 4) // array_size * 4 = total size in bytes

int dummy;

void useInt(int value)
{
    dummy += value;
}
int use_result_dummy;
void use_pointer(void *result) { use_result_dummy += *((int *)result); }

#define ONE p = (char **)*p;
#define FIVE ONE ONE ONE ONE ONE
#define TEN FIVE FIVE
#define FIFTY TEN TEN TEN TEN TEN
#define HUNDRED FIFTY FIFTY
int sum = 0;

void wr()
{
    auto *array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    // Initialize the array
    for (size_t i = 0; i < ARRAY_SIZE; ++i)
    {
        array[i] = i;
    }

    // Perform a write operation with loop unrolling and macro usage
    size_t num_elements = ARRAY_SIZE / 128;
    auto p = reinterpret_cast<char *>(array);
    auto start_time = std::chrono::high_resolution_clock::now();
    // for (size_t j = 0; j < ITERATIONS; ++j) {
    //         array[j] = j;
    // }
    for (size_t j = 0; j < num_elements; ++j)
    {
#define DOIT(i) p[i] = 0xABCDEF00;
        DOIT(0)
        DOIT(1)
        DOIT(2)
        DOIT(3)
        DOIT(4) DOIT(5) DOIT(6) DOIT(7) DOIT(8) DOIT(9) DOIT(10) DOIT(11) DOIT(12) DOIT(13) DOIT(14) DOIT(15)
            DOIT(16) DOIT(17) DOIT(18) DOIT(19) DOIT(20) DOIT(21) DOIT(22) DOIT(23) DOIT(24) DOIT(25) DOIT(26) DOIT(27) DOIT(28) DOIT(29) DOIT(30) DOIT(31)
                DOIT(32) DOIT(33) DOIT(34) DOIT(35) DOIT(36) DOIT(37) DOIT(38) DOIT(39) DOIT(40) DOIT(41) DOIT(42) DOIT(43) DOIT(44) DOIT(45) DOIT(46) DOIT(47)
                    DOIT(48) DOIT(49) DOIT(50) DOIT(51) DOIT(52) DOIT(53) DOIT(54) DOIT(55) DOIT(56) DOIT(57) DOIT(58) DOIT(59) DOIT(60) DOIT(61) DOIT(62) DOIT(63)
                        DOIT(64) DOIT(65) DOIT(66) DOIT(67) DOIT(68) DOIT(69) DOIT(70) DOIT(71) DOIT(72) DOIT(73) DOIT(74) DOIT(75) DOIT(76) DOIT(77) DOIT(78) DOIT(79)
                            DOIT(80) DOIT(81) DOIT(82) DOIT(83) DOIT(84) DOIT(85) DOIT(86) DOIT(87) DOIT(88) DOIT(89) DOIT(90) DOIT(91) DOIT(92) DOIT(93) DOIT(94) DOIT(95)
                                DOIT(96) DOIT(97) DOIT(98) DOIT(99) DOIT(100) DOIT(101) DOIT(102) DOIT(103) DOIT(104) DOIT(105) DOIT(106) DOIT(107) DOIT(108) DOIT(109) DOIT(110) DOIT(111)
                                    DOIT(112) DOIT(113) DOIT(114) DOIT(115) DOIT(116) DOIT(117) DOIT(118) DOIT(119) DOIT(120) DOIT(121) DOIT(122) DOIT(123) DOIT(124) DOIT(125) DOIT(126) DOIT(127);
        p += 128;
    }
    auto end_time = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> elapsed_time = end_time - start_time;

    double bandwidth_mb_per_sec = ((ARRAY_SIZE * sizeof(int)) / (elapsed_time.count() * 1e6));

    printf("Write Bandwidth: %.2f MB/s\n", bandwidth_mb_per_sec);
    free(array);
}
#undef DOIT

void rd()
{
    auto *array = (int *)malloc(ARRAY_SIZE * sizeof(int));

    // Initialize the array
    for (size_t i = 0; i < ARRAY_SIZE; ++i)
    {
        array[i] = i;
    }

    // Perform a write operation with loop unrolling and macro usage
    size_t num_elements = ARRAY_SIZE / 128;
    auto p = reinterpret_cast<char *>(array);
    auto start_time = std::chrono::high_resolution_clock::now();

    for (size_t j = 0; j < num_elements; ++j)
    {
        sum +=
#define DOIT(i) p[i] +
            DOIT(0) DOIT(1) DOIT(2) DOIT(3) DOIT(4) DOIT(5) DOIT(6) DOIT(7) DOIT(8) DOIT(9) DOIT(10) DOIT(11) DOIT(12) DOIT(13) DOIT(14) DOIT(15)
                DOIT(16) DOIT(17) DOIT(18) DOIT(19) DOIT(20) DOIT(21) DOIT(22) DOIT(23) DOIT(24) DOIT(25) DOIT(26) DOIT(27) DOIT(28) DOIT(29) DOIT(30) DOIT(31)
                    DOIT(32) DOIT(33) DOIT(34) DOIT(35) DOIT(36) DOIT(37) DOIT(38) DOIT(39) DOIT(40) DOIT(41) DOIT(42) DOIT(43) DOIT(44) DOIT(45) DOIT(46) DOIT(47)
                        DOIT(48) DOIT(49) DOIT(50) DOIT(51) DOIT(52) DOIT(53) DOIT(54) DOIT(55) DOIT(56) DOIT(57) DOIT(58) DOIT(59) DOIT(60) DOIT(61) DOIT(62) DOIT(63)
                            DOIT(64) DOIT(65) DOIT(66) DOIT(67) DOIT(68) DOIT(69) DOIT(70) DOIT(71) DOIT(72) DOIT(73) DOIT(74) DOIT(75) DOIT(76) DOIT(77) DOIT(78) DOIT(79)
                                DOIT(80) DOIT(81) DOIT(82) DOIT(83) DOIT(84) DOIT(85) DOIT(86) DOIT(87) DOIT(88) DOIT(89) DOIT(90) DOIT(91) DOIT(92) DOIT(93) DOIT(94) DOIT(95)
                                    DOIT(96) DOIT(97) DOIT(98) DOIT(99) DOIT(100) DOIT(101) DOIT(102) DOIT(103) DOIT(104) DOIT(105) DOIT(106) DOIT(107) DOIT(108) DOIT(109) DOIT(110) DOIT(111)
                                        DOIT(112) DOIT(113) DOIT(114) DOIT(115) DOIT(116) DOIT(117) DOIT(118) DOIT(119) DOIT(120) DOIT(121) DOIT(122) DOIT(123) DOIT(124) DOIT(125) DOIT(126) p[127];

        p += 128;
    }
    // for (size_t j = 0; j < ITERATIONS; ++j) {
    //     sum+=array[j];
    // }
    useInt(sum);
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;

    double bandwidth_mb_per_sec = ((ARRAY_SIZE * sizeof(int)) / (elapsed_time.count() * 1e6));
    printf("Read Bandwidth: %.2f MB/s\n", bandwidth_mb_per_sec);
    free(array);
}
#undef DOIT

void cp()
{
    // Allocate source and destination arrays
    int *array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    int *dst = (int *)malloc(ARRAY_SIZE * sizeof(int));

    // Check if allocations succeeded
    if (!array || !dst)
    {
        free(array); // Free whichever allocation might have succeeded
        free(dst);
        printf("Memory allocation failed\n");
        return;
    }
    // Initialize the arrays
    for (size_t i = 0; i < ARRAY_SIZE; ++i)
    {
        array[i] = i;
        dst[i] = 0; // Initialize destination array to zero
    }

    // Perform a write operation with loop unrolling and macro usage
    size_t num_elements = ARRAY_SIZE / 128;
    auto p = reinterpret_cast<char *>(array);
    auto d = reinterpret_cast<char *>(dst); // Keep original dst pointer
    auto start_time = std::chrono::high_resolution_clock::now();
    for (size_t j = 0; j < num_elements; ++j)
    {
#define DOIT(i) d[i] = p[i];
        DOIT(0)
        DOIT(1)
        DOIT(2)
        DOIT(3)
        DOIT(4) DOIT(5) DOIT(6) DOIT(7) DOIT(8) DOIT(9) DOIT(10) DOIT(11) DOIT(12) DOIT(13) DOIT(14) DOIT(15)
            DOIT(16) DOIT(17) DOIT(18) DOIT(19) DOIT(20) DOIT(21) DOIT(22) DOIT(23) DOIT(24) DOIT(25) DOIT(26) DOIT(27) DOIT(28) DOIT(29) DOIT(30) DOIT(31)
                DOIT(32) DOIT(33) DOIT(34) DOIT(35) DOIT(36) DOIT(37) DOIT(38) DOIT(39) DOIT(40) DOIT(41) DOIT(42) DOIT(43) DOIT(44) DOIT(45) DOIT(46) DOIT(47)
                    DOIT(48) DOIT(49) DOIT(50) DOIT(51) DOIT(52) DOIT(53) DOIT(54) DOIT(55) DOIT(56) DOIT(57) DOIT(58) DOIT(59) DOIT(60) DOIT(61) DOIT(62) DOIT(63)
                        DOIT(64) DOIT(65) DOIT(66) DOIT(67) DOIT(68) DOIT(69) DOIT(70) DOIT(71) DOIT(72) DOIT(73) DOIT(74) DOIT(75) DOIT(76) DOIT(77) DOIT(78) DOIT(79)
                            DOIT(80) DOIT(81) DOIT(82) DOIT(83) DOIT(84) DOIT(85) DOIT(86) DOIT(87) DOIT(88) DOIT(89) DOIT(90) DOIT(91) DOIT(92) DOIT(93) DOIT(94) DOIT(95)
                                DOIT(96) DOIT(97) DOIT(98) DOIT(99) DOIT(100) DOIT(101) DOIT(102) DOIT(103) DOIT(104) DOIT(105) DOIT(106) DOIT(107) DOIT(108) DOIT(109) DOIT(110) DOIT(111)
                                    DOIT(112) DOIT(113) DOIT(114) DOIT(115) DOIT(116) DOIT(117) DOIT(118) DOIT(119) DOIT(120) DOIT(121) DOIT(122) DOIT(123) DOIT(124) DOIT(125) DOIT(126) DOIT(127);
        p += 128;
        d += 128;
    }
    // for (size_t j = 0; j < ITERATIONS; ++j) {
    //     dst[j] = array[j];
    // }
    auto end_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed_time = end_time - start_time;
    double bandwidth_mb_per_sec = ((ARRAY_SIZE * sizeof(int)) / (elapsed_time.count() * 1e6));
    printf("Copy Bandwidth: %.2f MB/s\n", bandwidth_mb_per_sec);

    // Free allocated memory
    free(array);
    free(dst);
}
#undef DOIT
// void lat(unsigned long memsize = 0x800000, unsigned int stride = 64) {
//     unsigned long i, tmp;
//     unsigned long sec, usec;
//     char *mem =(char*)malloc(memsize);
//     unsigned int *indices;
//     unsigned long count = 1048576;
//
//     unsigned long size = memsize / stride;
//     indices = (unsigned int*)malloc(size * sizeof(int));
//
//     // 初始化索引
//     for (i = 0; i < size; i++)
//         indices[i] = i;
//
//     // 填充内存与指针引用
//     for (i = 0; i < size - 1; i++)
//         *(char **)&mem[indices[i]*stride]= (char*)&mem[indices[i+1]*stride];
//     *(char **)&mem[indices[size-1]*stride]= (char*)&mem[indices[0]*stride];
//
//     // 延迟测试
//     char **p = (char **) mem;
//     tmp = count / 100;
//
//     auto start_time = std::chrono::high_resolution_clock::now();
//     for (i = 0; i < tmp; ++i) {
//         HUNDRED;
//     }
//     auto end_time = std::chrono::high_resolution_clock::now();
//     std::chrono::duration<double> elapsed_time = end_time - start_time;
//     auto elapsed_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed_time).count();
//     double result = (double)elapsed_time_ns / (double)(tmp*100);
//     printf("Buffer size: %.5f MB, stride %d, time %f s, latency %f ns\n",
//            memsize / (1024.*1024.), stride, std::chrono::duration<double>(elapsed_time).count() , result);
//
//     // 清理
//     free(mem);
//     free(indices);
// }

// size_t
// step(size_t k)
// {
//     if (k < 1024) {
//         k = k * 2;
//     } else if (k < 4*1024) {
//         k += 1024;
//     } else {
//         size_t s;
//
//         for (s = 32 * 1024; s <= k; s *= 2)
//             ;
//         k += s / 16;
//     }
//     return (k);
// }

int main()
{
    std::cout << "Memory Bandwidth Benchmark" << std::endl;
    std::cout << "============================" << std::endl;
    std::cout << "Total Size: " << ARRAY_SIZE * sizeof(int) / (1024 * 1024) << " MB" << std::endl;
    wr();
    rd();
    cp();
    return 0;
}