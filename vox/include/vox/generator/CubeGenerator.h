#ifndef VOX_GENERATOR_CUBEGENERATOR_H_
#define VOX_GENERATOR_CUBEGENERATOR_H_

#include <coin/gl.h>

#include <vox/Volume.h>
#include <vox/util/RawList.h>


namespace vox {

template <typename VoxelType, typename VolumeType, typename IndexType = GLuint, VoxelType kEmptyCubeIndex = 0>
class CubeGenerator {
public:
    struct Vertex {
        GLfloat x, y, z;
        GLfloat normal_x, normal_y, normal_z;
        GLfloat texture_id;

        Vertex (GLfloat x, GLfloat y, GLfloat z,
                GLfloat normal_x, GLfloat normal_y, GLfloat normal_z,
                GLfloat texture_id) {
            this->x = x;
            this->y = y;
            this->z = z;
            this->normal_x = normal_x;
            this->normal_y = normal_y;
            this->normal_z = normal_z;
            this->texture_id = texture_id;
        }
    };

private:
    /*
     * A bitmask for the cube merger to identify already merged quads.
     * Should be allocated on the stack!
     */
    template<VoxSize kHeight>
    class Layer {
    public:
        typedef bool T;

        inline static size_t GetIndex (VoxPos x, VoxPos y) {
            return y * kHeight + x;
        }

        inline static bool Get (T* layer, VoxPos x, VoxPos y) {
            return layer[GetIndex (x, y)];
        }

        inline static void Set (T* layer, VoxPos x, VoxPos y, bool flag) {
            layer[GetIndex (x, y)] = flag;
        }
    };

    u64 vertices_generated_;
    u32 runs_;
    u32 average_vertex_count_;
    u32 expected_vertex_count_;
    bool update_;

    RawList<Vertex> vertices_;
    RawList<IndexType> indices_;

    template<typename T>
    inline void ReserveCapacity (RawList<T>& list, size_t needed) {
        if (needed > list.size ()) {
            list.Resize (needed);
        }
    }


public:
    CubeGenerator () {
        vertices_generated_ = 0; /* The maximum possible face count. */
        runs_ = 0;
        update_ = false;
    }

    ~CubeGenerator () {
        
    }

    void UpdateExpectedVertexCount () {
        const float kVertexCountUpdateTreshold = 0.2f;

        average_vertex_count_ = (u32) vertices_generated_ / runs_;

        bool update_expected = false;
        u32 diff = average_vertex_count_ - expected_vertex_count_;

        /* Update the expected vertex count when the treshold is passed. */
        if (diff < 0.0f) { /* Decrease size. */
            if ((u32) ((float) average_vertex_count_ * (1.0f + kVertexCountUpdateTreshold)) < expected_vertex_count_) {
                update_expected = true;
            }
        }else if (diff > 0.0f) { /* Increase size. */
            if ((u32) ((float) average_vertex_count_ * (1.0f - kVertexCountUpdateTreshold)) > expected_vertex_count_) {
                update_expected = true;
            }
        }

        if (update_expected) {
            expected_vertex_count_ = average_vertex_count_;
            update_ = true;
        }
    }

    void Generate (VolumeType& volume, float* voxel_texture_ids) {
        /* Clear any data. */
        vertices_.ResetIterator ();
        indices_.ResetIterator ();

        /* Resize vectors if the expected face count is different. 
            Don't consider the amount of layers here, 
            because that would make the average calculation worse. */
        if (update_) {
            // size_t allocation_size = 0;
            const size_t vertices_size = expected_vertex_count_ * 4;
            const size_t indices_size = expected_vertex_count_ * 6;

            vertices_.Resize (vertices_size);
            indices_.Resize (indices_size);
            
            // allocation_size += vertices_size * sizeof (Vertex);
            // allocation_size += indices_size * sizeof (IndexType);
            // printf ("Approximate number of bytes allocated: %llu\n", allocation_size);

            update_ = false;
        }

        const u32 filled_layer_count = volume.CountLayersWithVoxels ();

        /* Generate top faces. */
        for (VoxPos y = 0; y < volume.height (); ++y) {

            /* Skip empty layers. */
            if (volume.IsLayerEmpty (y)) {
                continue;
            }

            typedef Layer<VolumeType::kDepth> LayerY;
            LayerY::T layer [VolumeType::kWidth * VolumeType::kDepth];

            /* Fill layer information. */
            for (VoxPos z = 0; z < volume.depth (); ++z) {
                for (VoxPos x = 0; x < volume.width (); ++x) {
                    VoxelType voxel = volume.GetVoxel (x, y, z);

                    /* Air voxels can be ignored. */
                    if (voxel == 0) {
                        LayerY::Set (layer, x, z, true);
                        continue;
                    }

                    /* When a cube is above, this quad is invalid. */
                    {
                        const VoxPos y_above = y + 1;
                        if (y_above < volume.height () && volume.GetVoxel (x, y_above, z) > 0) {
                            LayerY::Set (layer, x, z, true);
                            continue;
                        }
                    }

                    /* Otherwise, clear the layer value. */
                    LayerY::Set (layer, x, z, false);
                }
            }

            /* Generate faces. */
            for (VoxPos z = 0; z < volume.depth (); ++z) {
                for (VoxPos x = 0; x < volume.width (); ) {
                    /* Continue if the voxel is already in some quad or invalid. */
                    if (LayerY::Get (layer, x, z)) {
                        ++x;
                        continue;
                    }

                    VoxelType voxel = volume.GetVoxel (x, y, z);

                    /* Get maximum adjacent z. */
                    VoxPos z_end = z + 1;
                    for (; z_end < volume.depth (); ++z_end) {
                        if (LayerY::Get (layer, x, z_end) ||
                            volume.GetVoxel (x, y, z_end) != voxel) break;
                    }

                    /* Get maximum adjacent x. */
                    VoxPos x_end = x + 1;
                    for (; x_end < volume.width (); ++x_end) {
                        if (LayerY::Get (layer, x_end, z) ||
                            volume.GetVoxel (x_end, y, z) != voxel) break;
                    }

                    /* Check enclosed voxels on z axis. */
                    for (VoxPos search_x = x + 1; search_x < x_end; ++search_x) {
                        for (VoxPos search_z = z + 1; search_z < z_end; ++search_z) {
                            if (LayerY::Get (layer, search_x, search_z)
                                || volume.GetVoxel (search_x, y, search_z) != voxel) {
                                z_end = search_z;
                                break;
                            }
                        }
                    }

                    /* Check enclosed voxels on x axis. */
                    for (VoxPos search_z = z + 1; search_z < z_end; ++search_z) {
                        for (VoxPos search_x = x + 1; search_x < x_end; ++search_x) {
                            if (LayerY::Get (layer, search_x, search_z)
                                || volume.GetVoxel (search_x, y, search_z) != voxel) {
                                x_end = search_x;
                                break;
                            }
                        }
                    }

                    /* Width and depth. */
                    const VoxSize width = x_end - x;
                    const VoxSize depth = z_end - z;

                    /* Add vertices. */
                    ReserveCapacity (vertices_, vertices_.iterator () + 4);

                    float texture_id = voxel_texture_ids[voxel];
                    IndexType vertex_0 = (IndexType) vertices_.iterator ();
                    vertices_.Next () = Vertex (x, y, z,            0.0f, 0.0f, 0.0f, texture_id);
                    vertices_.Next () = Vertex (x_end, y, z,        0.0f, 0.0f, 0.0f, texture_id);
                    vertices_.Next () = Vertex (x_end, y, z_end,    0.0f, 0.0f, 0.0f, texture_id);
                    vertices_.Next () = Vertex (x, y, z_end,        0.0f, 0.0f, 0.0f, texture_id);

                    /* Add indices. */
                    ReserveCapacity (indices_, indices_.iterator () * 6);

                    indices_.Next () = vertex_0;
                    indices_.Next () = vertex_0 + 1;
                    indices_.Next () = vertex_0 + 2;
                    indices_.Next () = vertex_0 + 2;
                    indices_.Next () = vertex_0 + 3;
                    indices_.Next () = vertex_0;

                    /* Mark layer. */
                    for (VoxPos mark_z = z; mark_z < z_end; ++mark_z) {
                        memset (layer + LayerY::GetIndex (x, mark_z), 0x01, width);
                    }

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

        runs_ += 1;
        vertices_generated_ += vertices_.iterator ();

        UpdateExpectedVertexCount ();
    }

    inline RawList<Vertex>& vertices () const { return vertices_; }
    inline RawList<IndexType>& indices () const { return indices_; }
};

}


#endif  /* VOX_GENERATOR_CUBEGENERATOR_H_ */