#ifndef VOX_VOLUME_H_
#define VOX_VOLUME_H_

#include <string.h>

#include <coin/gl.h>

#include <vox/vox.h>
#include <vox/Region.h>


namespace vox {

template<typename Type, VoxSize kWidth, VoxSize kHeight, VoxSize kDepth>
class Volume {
private:
    Type* data_;

    GLuint x_;
    GLuint y_;
    GLuint z_;

    VoxArea* layer_x_block_count_;
    VoxArea* layer_y_block_count_;
    VoxArea* layer_z_block_count_;

public:
    static const VoxSize kWidth = kWidth;
    static const VoxSize kHeight = kHeight;
    static const VoxSize kDepth = kDepth;
    static const VoxArea kLayerSize = kWidth * kDepth;
    static const VoxVolume kVolumeSize = kLayerSize * kHeight;
    
    Volume (const GLuint x, const GLuint y, const GLuint z, const bool clear_data) {
        data_ = new Type[kVolumeSize];
        if (clear_data) {
            memset (data_, 0x00, kVolumeSize * sizeof (Type));
        }

        x_ = x;
        y_ = y;
        z_ = z;
        layer_x_block_count_ = new VoxArea[kWidth];
        layer_y_block_count_ = new VoxArea[kHeight];
        layer_z_block_count_ = new VoxArea[kDepth];
        memset (layer_x_block_count_, 0, kWidth * sizeof (VoxArea));
        memset (layer_y_block_count_, 0, kHeight * sizeof (VoxArea));
        memset (layer_z_block_count_, 0, kDepth * sizeof (VoxArea));
    }

    ~Volume () {
        delete[] data_;
        delete[] layer_x_block_count_;
        delete[] layer_y_block_count_;
        delete[] layer_z_block_count_;
    }


    inline const size_t GetVoxelIndex (const VoxPos x, const VoxPos y, const VoxPos z) const {
        return y * kLayerSize + z * kWidth + x;
    }

    inline bool PositionOutOfBounds (VoxPos x, VoxPos y, VoxPos z) const {
        return x < 0 || x >= kWidth || y < 0 || y >= kHeight || z < 0 || z >= kDepth;
    }
    
    inline Type GetVoxel (const VoxPos x, const VoxPos y, const VoxPos z, const bool check_bounds = false) const {
        if (check_bounds) {
            if (PositionOutOfBounds (x, y, z)) {
                return 0;
            }
        }
        return data_[GetVoxelIndex (x, y, z)];
    }

    void SetVoxel (const VoxPos x, const VoxPos y, const VoxPos z, const Type voxel) {
        Type& voxel_at_pos = data_[GetVoxelIndex (x, y, z)];
        if (voxel == 0) {
            if (voxel_at_pos == 0) {
                return;
            }else {
                layer_x_block_count_[x] -= 1;
                layer_y_block_count_[y] -= 1;
                layer_z_block_count_[z] -= 1;
            }
        }else { /* voxel != 0 */
            if (voxel_at_pos == 0) {
                layer_x_block_count_[x] += 1;
                layer_y_block_count_[y] += 1;
                layer_z_block_count_[z] += 1;
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

    inline bool IsLayerXEmpty (const VoxPos x) const {
        return layer_x_block_count_[x] == 0;
    }

    inline bool IsLayerYEmpty (const VoxPos y) const {
        return layer_y_block_count_[y] == 0;
    }

    inline bool IsLayerZEmpty (const VoxPos z) const {
        return layer_z_block_count_[z] == 0;
    }


    inline Type* data () const { return data_; }
    inline GLuint x () const { return x_; }
    inline GLuint y () const { return y_; }
    inline GLuint z () const { return z_; }
    
    inline static const size_t data_size () { return kVolumeSize * sizeof (Type); } 
    inline static const VoxSize width () { return kWidth; }
    inline static const VoxSize height () { return kHeight; }
    inline static const VoxSize depth () { return kDepth; }
    inline static const VoxArea area () { return kLayerSize; }
    inline static const VoxVolume volume () { return kVolumeSize; }
};

}


#endif  /* VOX_VOLUME_H_ */
