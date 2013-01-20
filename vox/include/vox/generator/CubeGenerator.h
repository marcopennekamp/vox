#ifndef VOX_GENERATOR_CUBE_GENERATOR_H_
#define VOX_GENERATOR_CUBE_GENERATOR_H_

#include <vector>

#include <coin/gl.h>

#include <vox/Volume.h>


namespace vox {

template <typename VoxelType, typename VolumeType, typename IndexType = GLuint, VoxelType kEmptyCubeIndex = 0>
class CubeGenerator {
public:
    struct Vertex {
        GLfloat x, y, z;
        GLfloat normal_x, normal_y, normal_z;
        GLfloat texture_id;
    };

private:
    struct CubeFace {
        VoxPos x, y, z;
        VoxSize width, height;
        VoxelType voxel;

        CubeFace (const VoxPos x, const VoxPos y, const VoxPos z, 
            const VoxSize width, const VoxSize height, const VoxelType voxel) {
            this->x = x;
            this->y = y;
            this->z = z;
            this->width = width;
            this->height = height;
            this->voxel = voxel;
        }

        ~CubeFace () {
        }
    };

    VoxSize last_volume_size_;

    std::vector<Vertex> vertices_;
    std::vector<IndexType> indices_;

    CubeFace* top_faces_;
    size_t top_faces_index_;

    inline size_t GetLayerIndex (VoxSize height, VoxPos x, VoxPos y) {
        return y * height + x;
    }
    
    inline u8 GetLayerFlag (u8* layer, VoxSize height, VoxPos x, VoxPos y) {
        return layer[GetLayerIndex (height, x, y)];
    }

    inline void SetLayerFlag (u8* layer, VoxSize height, VoxPos x, VoxPos y, u8 element) {
        layer[GetLayerIndex (height, x, y)] = element;
    }


public:
    CubeGenerator () {
        top_faces_ = NULL;
        last_volume_size_ = 0;
    }

    ~CubeGenerator () {
        if (top_faces_ != NULL) free (top_faces_);
    }

    void PrintLayer (u8* layer, VolumeType& volume) {
        for (int lz = 0; lz < volume.depth (); ++lz) {
            for (int lx = 0; lx < volume.width (); ++lx) {
                printf ("%u ", GetLayerFlag (layer, volume.width (), lx, lz));
            }
            printf ("\n");
        }
        printf ("\n\n");
    }

    void Generate (VolumeType& volume) {
        /* Resize vectors if volume size differs. */
        if (last_volume_size_ != volume.volume ()) {
            const u32 max_faces = volume.volume () * 3;
            vertices_.reserve (max_faces * 4);
            indices_.reserve (max_faces * 6);

            const size_t face_array_size = max_faces * sizeof (CubeFace);
            if (top_faces_ == NULL) {
                top_faces_ = (CubeFace*) malloc (face_array_size);
            }else {
                top_faces_ = (CubeFace*) realloc (top_faces_, face_array_size);
            }
        }
        vertices_.clear ();
        indices_.clear ();
        top_faces_index_ = 0;
        last_volume_size_ = volume.volume ();

        /* Generate top faces. */
        const size_t layer_size = volume.width () * volume.depth ();
        u8* layer = new u8[layer_size];
        for (VoxPos y = 0; y < volume.height (); ++y) {

            /* Fill layer information. */
            for (VoxPos z = 0; z < volume.depth (); ++z) {
                for (VoxPos x = 0; x < volume.width (); ++x) {
                    VoxelType voxel = volume.GetVoxel (x, y, z);

                    /* Air voxels can be ignored. */
                    if (voxel == 0) {
                        //printf ("Air voxel: (%u, %u, %u)\n", x, y, z);
                        SetLayerFlag (layer, volume.width (), x, z, 1);
                        continue;
                    }

                    /* When a cube is above, this quad is invalid. */
                    {
                        const VoxPos y_above = y + 1;
                        if (y_above < volume.height () && volume.GetVoxel (x, y_above, z) > 0) {
                            //printf ("Cube above: (%u, %u, %u)\n", x, y, z);
                            SetLayerFlag (layer, volume.width (), x, z, 1);
                            continue;
                        }
                    }

                    SetLayerFlag (layer, volume.width (), x, z, 0);
                }
            }

            //PrintLayer (layer, volume);

            /* Generate faces. */
            for (VoxPos z = 0; z < volume.depth (); ++z) {
                for (VoxPos x = 0; x < volume.width (); ) {
                    /* Continue if the voxel is already in some quad or invalid. */
                    if (GetLayerFlag (layer, volume.width (), x, z)) {
                        ++x;
                        continue;
                    }

                    VoxelType voxel = volume.GetVoxel (x, y, z);

                    /* Get maximum adjacent z. */
                    VoxPos z_end = z + 1;
                    for (; z_end < volume.depth (); ++z_end) {
                        if (GetLayerFlag (layer, volume.width (), x, z_end) || 
                            volume.GetVoxel (x, y, z_end) != voxel) break;
                    }

                    /* Get maximum adjacent x. */
                    VoxPos x_end = x + 1;
                    for (; x_end < volume.width (); ++x_end) {
                        if (GetLayerFlag (layer, volume.width (), x_end, z) ||
                            volume.GetVoxel (x_end, y, z) != voxel) break;
                    }

                    //printf ("Adjacent: (%u, %u, %u) (%u, %u)\n", x, y, z, z_end, x_end);

                    /* Check enclosed voxels on z axis. */
                    for (VoxPos search_x = x + 1; search_x < x_end; ++search_x) {
                        for (VoxPos search_z = z + 1; search_z < z_end; ++search_z) {
                            if (GetLayerFlag (layer, volume.width (), search_x, search_z) 
                                || volume.GetVoxel (search_x, y, search_z) != voxel) {
                                    //printf ("Break z-axis search: (%u, %u, %u) (%u, %u)\n", x, y, z, search_x, search_z);
                                z_end = search_z;
                                break;
                            }
                        }
                    }

                    /* Check enclosed voxels on x axis. */
                    for (VoxPos search_z = z + 1; search_z < z_end; ++search_z) {
                        for (VoxPos search_x = x + 1; search_x < x_end; ++search_x) {
                            if (GetLayerFlag (layer, volume.width (), search_x, search_z) 
                                || volume.GetVoxel (search_x, y, search_z) != voxel) {
                                    //printf ("Break x-axis search: (%u, %u, %u) (%u, %u)\n", x, y, z, search_x, search_z);
                                x_end = search_x;
                                break;
                            }
                        }
                    }

                    //printf ("After enclosing search: (%u, %u, %u) (%u, %u)\n", x, y, z, z_end, x_end);

                    /* Add cube face. */
                    const VoxSize width = x_end - x;
                    const VoxSize depth = z_end - z;
                    top_faces_[top_faces_index_] = CubeFace (x, y, z, width, depth, voxel);
                    ++top_faces_index_;

                    /* Mark layer. */
                    for (VoxPos mark_z = z; mark_z < z_end; ++mark_z) {
                        memset (layer + GetLayerIndex (volume.width (), x, mark_z), 0x01, width);
                    }

                    //printf ("Width: %u\n", width);
                    //PrintLayer (layer, volume);

                    /* Offset x and z. */
                    if (width == depth && x_end == volume.width ()) { /* Rectangular full-fill. */
                        z = z_end - 1; /* "-1" before "++z". */
                        x = 0;
                        break;
                    }else {
                        x = x_end;
                    }
                }
            }
        }


        /* for (size_t i = 0; i < top_faces_index_; ++i) {
            CubeFace& face = top_faces_[i];
            printf ("(%u, %u, %u) {%u, %u} : %u\n", face.x, face.y, face.z, face.width, face.height, face.voxel);
        } */
        
        delete[] layer;
    }

    std::vector<Vertex>& vertices () { return vertices_; }
    std::vector<IndexType>& indices () { return indices_; }
};

}


#endif  /* VOX_GENERATOR_CUBE_GENERATOR_H_ */