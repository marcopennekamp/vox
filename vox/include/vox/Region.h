#ifndef VOX_REGION_H_
#define VOX_REGION_H_

#include <vox/vox.h>


namespace vox {

class Region {
private:
    VoxPos x_;
    VoxPos y_;
    VoxPos z_;

    VoxSize width_;
    VoxSize height_;
    VoxSize depth_;

public:
    Region () {
        x_ = 0;
        y_ = 0;
        z_ = 0;

        width_ = 0;
        height_ = 0;
        depth_ = 0;
    }

    Region (VoxPos x, VoxPos y, VoxPos z, VoxSize width, VoxSize height, VoxSize depth) {
        x_ = x;
        y_ = y;
        z_ = z;

        width_ = width;
        height_ = height;
        depth_ = depth;
    }

    inline bool SizeEquals (const Region& region) {
        return region.width () == width_ &&
            region.height () == height_ &&
            region.depth () == depth_;
    }

    inline bool PositionEquals (const Region& region) {
        return region.x () == x_ &&
            region.y () == y_ &&
            region.z () == z_;
    }

    inline bool operator== (const Region& region) {
        return SizeEquals (region) && PositionEquals (region);
    }

    inline const VoxPos x () const { return x_; }
    inline const VoxPos y () const { return y_; }
    inline const VoxPos z () const { return z_; }

    inline const VoxSize width () const { return width_; }
    inline const VoxSize height () const { return height_; }
    inline const VoxSize depth () const { return depth_; }

    inline const VoxPos x_end () const { return x_ + width_; }
    inline const VoxPos y_end () const { return y_ + height_; }
    inline const VoxPos z_end () const { return z_ + depth_; }

    inline const VoxVolume volume () const { return width_ * height_ * depth_; }
};

}


#endif  /* VOX_REGION_H_ */