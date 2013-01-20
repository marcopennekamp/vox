#ifdef __WIN32__
#include <Windows.h>
#endif

#include <stdio.h>

#include <coin/coin.h>
#include <coin/utils/time.h>

#include <vox/Volume.h>
#include <vox/generator/CubeGenerator.h>

using namespace coin;
using namespace vox;


int main (int argv, char** argc) {
    #ifdef __WIN32__
    ULONG_PTR affinity_mask;
    ULONG_PTR process_affinity_mask;
    ULONG_PTR system_affinity_mask;

    if (!GetProcessAffinityMask (GetCurrentProcess(), &process_affinity_mask, &system_affinity_mask)) return 0;

    /* Run on the first core. */
    affinity_mask = (ULONG_PTR)1 << 0;
    if (affinity_mask & process_affinity_mask) SetThreadAffinityMask (GetCurrentThread (), affinity_mask);
    #endif

    typedef Volume<u16, 32, 32, 32> BlockVolume;
    typedef Volume<u16, 64, 64, 64> BlockVolumeBig;

    TimeInit ();

    float texture_ids [] = {
        0.0f,
        0.0f
    };

    u64 time = TimeNanoseconds ();
    BlockVolume volume (true);
    volume.SetVoxelsInRegion (Region (0, 0, 0, 32, 1, 32), 0x01);
    volume.SetVoxelsInRegion (Region (4, 1, 4, 8, 1, 8), 0x01);
    printf ("Volume creation took %lluns.\n", TimeNanoseconds () - time);

    time = TimeNanoseconds ();
    BlockVolumeBig big_volume (true);
    big_volume.SetVoxelsInRegion (Region (0, 0, 0, 64, 1, 64), 0x01);
    big_volume.SetVoxelsInRegion (Region (16, 1, 16, 32, 1, 32), 0x01);
    printf ("Big volume creation took %lluns.\n", TimeNanoseconds () - time);

    CubeGenerator<u16, BlockVolume> generator;
    CubeGenerator<u16, BlockVolumeBig> big_generator;

    u64 sum = 0;
    for (int i = 0; i < 64; ++i) {
        time = TimeNanoseconds ();
        generator.Generate (volume, texture_ids);
        u64 diff = TimeNanoseconds () - time;
        sum += diff;
        printf ("Cube merging took %lluns.\n", diff);
    }
    printf ("In sum: %lluns.\n", sum);

    sum = 0;
    for (int i = 0; i < 8; ++i) {
        time = TimeNanoseconds ();
        big_generator.Generate (big_volume, texture_ids);
        u64 diff = TimeNanoseconds () - time;
        sum += diff;
        printf ("Big cube merging took %lluns.\n", diff);
    }
    printf ("In sum: %lluns.\n", sum);

    return 0;
}