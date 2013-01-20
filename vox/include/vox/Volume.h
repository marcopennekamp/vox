#ifndef VOX_VOLUME_H_
#define VOX_VOLUME_H_

#include <string.h>

#include <vox/vox.h>
#include <vox/Region.h>


namespace vox {

template<typename Type, VoxSize kWidth, VoxSize kHeight, VoxSize kDepth>
class Volume {
private:
    Type* data_;

    VoxArea* layer_block_count_;

public:
    static const VoxSize kWidth = kWidth;
    static const VoxSize kHeight = kHeight;
    static const VoxSize kDepth = kDepth;
    static const VoxArea kLayerSize = kWidth * kDepth;
    static const VoxVolume kVolumeSize = kLayerSize * kHeight;
    
    Volume (const bool clear_data) {
        data_ = new Type[kVolumeSize];
        if (clear_data) {
            memset (data_, 0x00, kVolumeSize * sizeof (Type));
        }

        layer_block_count_ = new VoxArea[kHeight];
        memset (layer_block_count_, 0, kHeight * sizeof (VoxArea));
    }

    ~Volume () {
        delete[] data_;
        delete[] layer_block_count_;
    }


    inline const size_t GetVoxelIndex (const VoxPos x, const VoxPos y, const VoxPos z) const {
        return y * kLayerSize + z * kWidth + x;
    }

    inline bool PositionOutOfBounds (VoxPos x, VoxPos y, VoxPos z) const {
        return x < 0 || x >= width_ || y < 0 || y >= height_ || z < 0 || z >= depth_;
    }
    
    inline Type GetVoxel (const VoxPos x, const VoxPos y, const VoxPos z) const {
        return data_[GetVoxelIndex (x, y, z)];
    }

    void SetVoxel (const VoxPos x, const VoxPos y, const VoxPos z, const Type voxel) {
        Type& voxel_at_pos = data_[GetVoxelIndex (x, y, z)];
        if (voxel == 0) {
            if (voxel_at_pos == 0) {
                return;
            }else {
                layer_block_count_[y] -= 1;
            }
        }else { /* voxel != 0 */
            if (voxel_at_pos == 0) {
                layer_block_count_[y] += 1;
            }
        }
        voxel_at_pos = voxel;
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


    inline bool IsLayerEmpty (const VoxPos y) const {
        return layer_block_count_[y] == 0;
    }

    inline u32 CountLayersWithVoxels () const {
        u32 count = 0;
        for (VoxPos y = 0; y < height (); ++y) {
            if (layer_block_count_[y] != 0) ++count;
        }
        return count;
    }
    

    inline Type* data () const { return data_; }
    inline static const size_t data_size () { return kVolumeSize * sizeof (Type); } 
    inline static const VoxSize width () { return kWidth; }
    inline static const VoxSize height () { return kHeight; }
    inline static const VoxSize depth () { return kDepth; }
    inline static const VoxArea area () { return kLayerSize; }
    inline static const VoxVolume volume () { return kVolumeSize; }
};

}


#endif  /* VOX_VOLUME_H_ */
