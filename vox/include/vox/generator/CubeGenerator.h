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
        GLfloat padding;

        Vertex (VolumeType& volume, const float kCubeSize,
                GLfloat x, GLfloat y, GLfloat z,
                GLfloat normal_x, GLfloat normal_y, GLfloat normal_z,
                GLfloat texture_id) {
            this->x = x + volume.x () * kCubeSize;
            this->y = y + volume.y () * kCubeSize;
            this->z = z + volume.z () * kCubeSize;
            this->normal_x = normal_x;
            this->normal_y = normal_y;
            this->normal_z = normal_z;
            this->texture_id = texture_id;
        }

        void Print () {
            printf ("(%f, %f, %f) (%f, %f, %f) : %f\n", x, y, z, normal_x, normal_y, normal_z, texture_id);
        }
    };

private:
    static const int kLayerTypeX = 0;
    static const int kLayerTypeY = 1;
    static const int kLayerTypeZ = 2;

    /*
     * A bitmask for the cube merger to identify already merged quads.
     * Should be allocated on the stack!
     */
    template<int kLayerType, VoxSize kWidth, VoxSize kHeight>
    class Layer {
    public:
        static const VoxSize kWidth = kWidth;
        static const VoxSize kHeight = kHeight;

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

        inline static void TransformIndex (VoxPos lx, VoxPos ly, VoxPos axis_coordinate, VoxPos& x, VoxPos& y, VoxPos& z) {
            switch (kLayerType) {
            case kLayerTypeX:
                x = axis_coordinate;
                y = ly;
                z = lx;
                break;
            case kLayerTypeY:
                x = lx;
                y = axis_coordinate;
                z = ly;
                break;
            case kLayerTypeZ:
                x = lx;
                y = ly;
                z = axis_coordinate;
                break;
            }
        }

        inline static VoxelType GetVoxel (VolumeType& volume, VoxPos lx, VoxPos ly, VoxPos axis_coordinate) {
            VoxPos x, y, z;
            TransformIndex (lx, ly, axis_coordinate, x, y, z);
            return volume.GetVoxel (x, y, z);
        }

        static void Print (T* layer) {
            for (VoxPos ly = 0; ly < kHeight; ++ly) {
                for (int lx = 0; lx < kWidth; ++lx) {
                    printf ("%u ", Get (layer, lx, ly));
                }
                printf ("\n");
            }
            printf ("\n\n");
        }

    };

    // TODO(Marco): When the volumes change (i.e. when the world position is completely changed), 
    //                  we have to RESET the following data, since it will be less accurate.
    //              One solution for this could be to only include the last 256 vertex counts.
    //              Another, maybe better, solution is to resize the lists according to the biggest
    //                  volume. That is, the volume with the most vertices.
    //              This approach could also apply to EBOs, since theoretically we only need one EBO
    //                  for all volumes instead of one EBO per volume.
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

    template<typename LayerType>
    inline void SetLayerFlags (VolumeType& volume, typename LayerType::T* layer, const VoxPos axis_coordinate, const VoxSize axis_size, const int direction) {
        for (VoxPos ly = 0; ly < LayerType::kHeight; ++ly) {
            for (VoxPos lx = 0; lx < LayerType::kWidth; ++lx) {
                VoxelType voxel = LayerType::GetVoxel (volume, lx, ly, axis_coordinate);

                /* Air voxels can be ignored. */
                if (voxel == kEmptyCubeIndex) {
                    LayerType::Set (layer, lx, ly, true);
                    continue;
                }

                /* Cube in front/back, quad invalid. */
                const VoxPos axis_neighbour = axis_coordinate + direction;
                if (axis_neighbour < axis_size && LayerType::GetVoxel (volume, lx, ly, axis_neighbour) != kEmptyCubeIndex) {
                    LayerType::Set (layer, lx, ly, true);
                }else { /* Clear. */
                    LayerType::Set (layer, lx, ly, false);
                }
            }
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

    inline void UpdateExpectedVertexCount () {
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

    static const int kMergeAreaXNegative = -1;
    static const int kMergeAreaYNegative = -2;
    static const int kMergeAreaZNegative = -3;
    static const int kMergeAreaXPositive = 1;
    static const int kMergeAreaYPositive = 2;
    static const int kMergeAreaZPositive = 3;
    static const int kMergeAreaX = 1;
    static const int kMergeAreaY = 2;
    static const int kMergeAreaZ = 3;

    template<int kMergeType, int layer_type, VoxSize axis_size, VoxSize layer_x_size, VoxSize layer_y_size>
    class MergeArea {
    public:
        inline static void Do (CubeGenerator* gen, VolumeType& volume, float* voxel_texture_ids, const float kCubeSize) {
            const int direction = (kMergeType > 0) ? 1 : -1;
            const VoxPos axis_offset = (direction > 0) ? 1 : 0;

            for (VoxPos axis_coord = 0; axis_coord < axis_size; ++axis_coord) {
                /* Skip empty layers. */
                switch (kMergeType) {
                case kMergeAreaXPositive:
                case kMergeAreaXNegative:
                    if (volume.IsLayerXEmpty (axis_coord)) continue;
                    break;

                case kMergeAreaYPositive:
                case kMergeAreaYNegative:
                    if (volume.IsLayerYEmpty (axis_coord)) continue;
                    break;

                case kMergeAreaZPositive:
                case kMergeAreaZNegative:
                    if (volume.IsLayerZEmpty (axis_coord)) continue;
                    break;
                }

                typedef Layer<layer_type, layer_x_size, layer_y_size> LayerType;
                LayerType::T layer [layer_x_size * layer_y_size];

                /* Fill layer information. */
                gen->SetLayerFlags<LayerType> (volume, layer, axis_coord, axis_size, direction);

                /* if (kMergeType == kMergeAreaYPositive) {
                    printf ("Layer Flags: %u\n", axis_coord);
                    LayerType::Print (layer);
                } */

                /* Generate faces. */
                for (VoxPos ly = 0; ly < LayerType::kHeight; ++ly) {
                    for (VoxPos lx = 0; lx < LayerType::kWidth; ) {
                        if (LayerType::Get (layer, lx, ly)) {
                            ++lx;
                            continue;
                        }

                        const VoxelType voxel = LayerType::GetVoxel (volume, lx, ly, axis_coord);

                        /* Get maximum adjacent layer_y. */
                        VoxPos ly_end = ly + 1;
                        for (; ly_end < LayerType::kHeight; ++ly_end) {
                            if (LayerType::Get (layer, lx, ly_end) ||
                                LayerType::GetVoxel (volume, lx, ly_end, axis_coord) != voxel) break;
                        }

                        /* Get maximum adjacent layer_x. */
                        VoxPos lx_end = lx + 1;
                        for (; lx_end < LayerType::kWidth; ++lx_end) {
                            if (LayerType::Get (layer, lx_end, ly) ||
                                LayerType::GetVoxel (volume, lx_end, ly, axis_coord) != voxel) break;
                        }

                        /* Check enclosed voxels on z axis. */
                        for (VoxPos slx = lx + 1; slx < lx_end; ++slx) {
                            for (VoxPos sly = ly + 1; sly < ly_end; ++sly) {
                                if (LayerType::Get (layer, slx, sly) ||
                                    LayerType::GetVoxel (volume, slx, sly, axis_coord) != voxel) {
                                    ly_end = sly;
                                    break;
                                }
                            }
                        }

                        /* Check enclosed voxels on x axis. */
                        for (VoxPos sly = ly + 1; sly < ly_end; ++sly) {
                            for (VoxPos slx = lx + 1; slx < lx_end; ++slx) {
                                if (LayerType::Get (layer, slx, sly) ||
                                    LayerType::GetVoxel (volume, slx, sly, axis_coord) != voxel) {
                                    lx_end = slx;
                                    break;
                                }
                            }
                        }

                        /* Width and depth. */
                        const VoxSize width = lx_end - lx;
                        const VoxSize height = ly_end - ly;

                        /* Add vertices. */
                        gen->ReserveCapacity (gen->vertices (), gen->vertices ().iterator () + 4);

                        const float texture_id = voxel_texture_ids[voxel];
                        const IndexType vertex_0 = (IndexType) gen->vertices ().iterator ();

                        const GLfloat face_x = lx * kCubeSize;
                        const GLfloat face_y = ly * kCubeSize;
                        const GLfloat face_x_end = face_x + width * kCubeSize;
                        const GLfloat face_y_end = face_y + height * kCubeSize;
                        const GLfloat face_axis_coord = (axis_coord + axis_offset) * kCubeSize;

                        switch (kMergeType) {
                        case kMergeAreaXPositive: /* Right. */
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_axis_coord, face_y, face_x,            1.0f, 0.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_axis_coord, face_y_end, face_x,        1.0f, 0.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_axis_coord, face_y_end, face_x_end,    1.0f, 0.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_axis_coord, face_y, face_x_end,        1.0f, 0.0f, 0.0f, texture_id);
                            break;
                        case kMergeAreaXNegative: /* Left. */
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_axis_coord, face_y, face_x,            -1.0f, 0.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_axis_coord, face_y, face_x_end,        -1.0f, 0.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_axis_coord, face_y_end, face_x_end,    -1.0f, 0.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_axis_coord, face_y_end, face_x,        -1.0f, 0.0f, 0.0f, texture_id);
                            break;
                        case kMergeAreaYPositive: /* Top. */
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x, face_axis_coord, face_y,            0.0f, 1.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x, face_axis_coord, face_y_end,        0.0f, 1.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x_end, face_axis_coord, face_y_end,    0.0f, 1.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x_end, face_axis_coord, face_y,        0.0f, 1.0f, 0.0f, texture_id);
                            break;
                        case kMergeAreaYNegative: /* Bottom. */
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x, face_axis_coord, face_y,            0.0f, -1.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x_end, face_axis_coord, face_y,        0.0f, -1.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x_end, face_axis_coord, face_y_end,    0.0f, -1.0f, 0.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x, face_axis_coord, face_y_end,        0.0f, -1.0f, 0.0f, texture_id);
                            break;
                        case kMergeAreaZPositive: /* Back. */
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x, face_y, face_axis_coord,            0.0f, 0.0f, 1.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x_end, face_y, face_axis_coord,        0.0f, 0.0f, 1.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x_end, face_y_end, face_axis_coord,    0.0f, 0.0f, 1.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x, face_y_end, face_axis_coord,        0.0f, 0.0f, 1.0f, texture_id);                            
                            break;
                        case kMergeAreaZNegative: /* Front. */
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x, face_y, face_axis_coord,            0.0f, 0.0f, -1.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x, face_y_end, face_axis_coord,        0.0f, 0.0f, -1.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x_end, face_y_end, face_axis_coord,    0.0f, 0.0f, -1.0f, texture_id);
                            gen->vertices ().Next () = Vertex (volume, kCubeSize, face_x_end, face_y, face_axis_coord,        0.0f, 0.0f, -1.0f, texture_id);
                            break;
                        }

                        /* Add indices. */
                        gen->ReserveCapacity (gen->indices (), gen->indices ().iterator () * 6);

                        gen->indices ().Next () = vertex_0;
                        gen->indices ().Next () = vertex_0 + 1;
                        gen->indices ().Next () = vertex_0 + 2;
                        gen->indices ().Next () = vertex_0 + 2;
                        gen->indices ().Next () = vertex_0 + 3;
                        gen->indices ().Next () = vertex_0;

                        /* Mark layer. */
                        for (VoxPos mark_y = ly; mark_y < ly_end; ++mark_y) {
                            memset (layer + LayerType::GetIndex (lx, mark_y), 0x01, width);
                        }

                        /* Offset x and z. */
                        if (lx == 0 && lx_end == LayerType::kWidth) { /* Rectangular full-fill. */
                            ly = ly_end - 1; /* "-1" before "++ly". */
                            lx = 0;
                            break;
                        }else {
                            lx = lx_end;
                        }
                    }
                }

                /* if (kMergeType == kMergeAreaYPositive) {
                    LayerType::Print (layer);
                } */
            }
        }
    };

    void Generate (VolumeType& volume, float* voxel_texture_ids, const float kCubeSize) {
        /* Clear any data. */
        vertices_.ResetIterator ();
        indices_.ResetIterator ();

        /* Resize the lists if the expected face count is different. 
            Don't consider the amount of layers here, 
            because that would make the average calculation worse. */
        if (update_) {
            const size_t vertices_size = expected_vertex_count_ * 4;
            const size_t indices_size = expected_vertex_count_ * 6;

            vertices_.Resize (vertices_size);
            indices_.Resize (indices_size);

            update_ = false;
        }
        
        // TODO(Marco): Wow, what a mess.
        MergeArea<kMergeAreaXPositive, kLayerTypeX, VolumeType::kWidth, VolumeType::kDepth, VolumeType::kHeight>::Do (this, volume, voxel_texture_ids, kCubeSize);
        MergeArea<kMergeAreaXNegative, kLayerTypeX, VolumeType::kWidth, VolumeType::kDepth, VolumeType::kHeight>::Do (this, volume, voxel_texture_ids, kCubeSize);
        MergeArea<kMergeAreaYPositive, kLayerTypeY, VolumeType::kHeight, VolumeType::kWidth, VolumeType::kDepth>::Do (this, volume, voxel_texture_ids, kCubeSize);
        MergeArea<kMergeAreaYNegative, kLayerTypeY, VolumeType::kHeight, VolumeType::kWidth, VolumeType::kDepth>::Do (this, volume, voxel_texture_ids, kCubeSize);
        MergeArea<kMergeAreaZPositive, kLayerTypeZ, VolumeType::kDepth, VolumeType::kWidth, VolumeType::kHeight>::Do (this, volume, voxel_texture_ids, kCubeSize);
        MergeArea<kMergeAreaZNegative, kLayerTypeZ, VolumeType::kDepth, VolumeType::kWidth, VolumeType::kHeight>::Do (this, volume, voxel_texture_ids, kCubeSize);

        runs_ += 1;
        vertices_generated_ += vertices_.iterator ();

        printf ("Memory used for vertices: %u byte\n", vertices_.iterator () * sizeof (Vertex) + indices ().iterator () * sizeof (GLuint)); 

        UpdateExpectedVertexCount ();
    }

    inline RawList<Vertex>& vertices () { return vertices_; }
    inline RawList<IndexType>& indices () { return indices_; }
};

}


#endif  /* VOX_GENERATOR_CUBEGENERATOR_H_ */