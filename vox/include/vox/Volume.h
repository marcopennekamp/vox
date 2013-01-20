#ifndef VOX_VOLUME_H_
#define VOX_VOLUME_H_

#include <string.h>

#include <vox/vox.h>
#include <vox/Region.h>


namespace vox {

template<typename Type, VoxSize kWidth, VoxSize kHeight, VoxSize kDepth>
class Volume {
private:
    static const VoxArea kLayerSize = kWidth * kDepth;
    static const VoxVolume kVolumeSize = kLayerSize * kHeight;
    static const size_t kDataSize = kVolumeSize * sizeof (Type);

    Type* data_;

public:
    Volume (const bool clear_data) {
        data_ = new Type[kVolumeSize];
        if (clear_data) {
            memset (data_, 0x00, kDataSize);
        }
    }

    ~Volume () {
        delete[] data_;
    }


    inline const size_t GetVoxelIndex (const VoxPos x, const VoxPos y, const VoxPos z) const {
        return y * kLayerSize + z * kWidth + x;
    }

    inline bool PositionOutOfBounds (VoxPos x, VoxPos y, VoxPos z) {
        return x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_;
    }
    
    Type GetVoxel (const VoxPos x, const VoxPos y, const VoxPos z) const {
        return data_[GetVoxelIndex (x, y, z)];
    }

    void SetVoxel (const VoxPos x, const VoxPos y, const VoxPos z, const Type voxel) {
        data_[GetVoxelIndex (x, y, z)] = voxel;
    }

    void SetVoxelsInRegion (const Region& region, const Type voxel) {
        const VoxPos x_end = region.x_end ();
        const VoxPos y_end = region.y_end ();
        const VoxPos z_end = region.z_end ();
        for (VoxPos y = region.y (); y < y_end; ++y) {
            for (VoxPos z = region.z (); z < z_end; ++z) {
                for (VoxPos x = region.x (); x < x_end; ++x) {
                    SetVoxel (x, y, z, voxel);
                }
            }
        }
    }
    

    inline Type* data () const { return data_; }
    inline const size_t data_size () const { return kDataSize; } 
    inline const VoxSize width () const { return kWidth; }
    inline const VoxSize height () const { return kHeight; }
    inline const VoxSize depth () const { return kDepth; }
    inline const VoxVolume volume () const { return kVolumeSize; }
};

}


#endif  /* VOX_VOLUME_H_ */
