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

    typedef Volume<u16, 640, 64, 640> BlockVolume;

    TimeInit ();

    u64 time = TimeNanoseconds ();
    BlockVolume volume (true);
    volume.SetVoxelsInRegion (Region (0, 0, 0, 640, 64, 640), 0x01);
    volume.SetVoxelsInRegion (Region (320, 1, 320, 160, 1, 160), 0x01);
    printf ("Volume creation took %lluns.\n", TimeNanoseconds () - time);

    CubeGenerator<u16, BlockVolume> generator;

    time = TimeNanoseconds ();
    generator.Generate (volume);
    printf ("Cube merging took %lluns.\n", TimeNanoseconds () - time);

    return 0;
}