/*
-- FILE HEADER --

VOXEL DATA STRUCTURE

Voxel material structure (stored in first 2 ints in voxel data):
1) [0-8 R][9-16 G][17-24 B][25-29 - alpha][30 - full flag][31 - tree node flag]
2) [0-4 blend mode][5-6 reserved][7 if set, all lighting is ignored][8-22 bits - normal]

GPU voxel data structure:
4096 bytes per page, no more than 1 chunk per page
- starting with chunk root, each chunk is 1 or more continuous pages

2 i32 per voxel for voxels (tree leaves)
10 i32 per voxel for tree nodes

Structure of chunk memory span:
- root (3 i32)
- tree nodes span
	Can store both tree nodes and voxels, leaves are tree nodes, that lost all children, in this case it still occupies 10 i32
- voxel (leaf) span
	Reallocation is very heavy, all tree nodes must be iterated to find ones, pointing to leaves and update.
	Leaves, marked as empty, must be cleaned up during memory optimization pass.

Reallocation:
	1. Reallocate buffer to newly required size.
	2. Iterate over all tree nodes, for each tree node, check if it is pointing into old voxel (leaf) span, for each such node, add difference between old and new span offset to pointer
	3. Move voxel (leaf) span using `memmove`

Memory optimization pass:
	1. Allocate new buffer, equal in size to the current one
	2. Do a calculating pass: recursively determine memory for remaining tree nodes and voxels (leaves)
	3. Do copying pass: recursively copy remaining tree nodes and voxels (leaves)

LOCAL ARRAY OPTIMIZATION
  When using local arrays with random access (non-constant indices), we are experiencing major performance dropdown.
It is happening, because such local arrays are stored in local memory instead of registers. To optimize this for small sized
(usually < 10) arrays, we use if conditions for all indices instead:

    if (index == 0) arr[0] ...
    else if (index == 1) arr[1] ...
    else if (index == 2) arr[2] ...
    ...

  Relying on compile-time optimizations, we can turn this into a for loop:

    for (int i = 0; i < *array_size_constant*; i++) {
        if (i == index) {
            arr[i] ...
            break;
        }
    }
*/


#ifndef VOXEL_ENGINE_RAYCAST_INC
#define VOXEL_ENGINE_RAYCAST_INC

#include "common.inc"
#include "raytrace/types.inc"
#include "raytrace/material.inc"

// Raycast stack optimization settings, search LOCAL ARRAY OPTIMIZATION
#define OPTIMIZE_RAYTRACE_STACK
#define OPTIMIZE_RAYTRACE_STACK_DISABLE_LOW_SCALES

#define MAX_STEPS_PER_RAY ${raytrace.max_steps_per_ray}u
#define MATERIAL_SPAN_MERGE_COEF ${raytrace.material_span_merge_coef}


// contains all voxel data, indexed
COMPUTE_SHADER_BUFFER(${world.chunk_data_buffer}, readonly, u_voxel_buffer_t, u_voxel_buffer, {
    // each voxel tree node contains following:
    // 0) 0-29 bits are voxel color (10 bits per component), then 2 flags: is voxel empty, does voxel have children
    // 1) some additional data
    // 2-9) relative pointers to child voxels for idx 0-7 (if has children), pointers are given relative to bit #0 in voxel, if pointer is 0 - voxel is empty
    //
    // chunk root structure:
    // 0) 0xC0000000
    // 1) reserved
    // 2) pointer to root voxel
    uint data[];
})

COMPUTE_SHADER_BUFFER(${world.chunk_map_buffer}, readonly, u_voxel_chunk_map_t, u_voxel_chunk_map, {
    int data[];
})

COMPUTE_SHADER_BUFFER(${world.chunk_fetch_buffer}, , u_voxel_fetch_map_t, u_voxel_fetch_map, {
    uint data[];
})


uint lookupVoxelChunk(ivec3 position, ivec3 map_offset, ivec3 map_size, int fetch_weight) {
    position -= map_offset;
    if (position.x >= 0 && position.y >= 0 && position.z >= 0 && position.x < map_size.x && position.y < map_size.y && position.z < map_size.z) {
        int index = position.x + map_size.x * (position.y + map_size.y * position.z);
        if (fetch_weight > 0) {
            u_voxel_fetch_map.data[index] = max(u_voxel_fetch_map.data[index], uint(fetch_weight));
        }
        return uint(u_voxel_chunk_map.data[index + 6]);
    }
    return uint(-1);
}

struct RaycastResultSpan {
    uvec3 mat;
    vec2 span;
};

struct RaycastResult {
    RaycastResultSpan spans[6];
    int count;

    float distance;
    vec3 end;
    uint steps;
};

struct MaterialRaycastData {
    uvec3 last_mat;
    vec3 t_last_side_v;
    float t_chunk_end;
    bool in_voxel;
};

struct ChunkRaycastData {
    ivec3 pos;
    ivec3 pos_step;
    vec3 rayL;
    vec3 rayS;
};

struct RaycastStackNode {
    uint voxel_pointer;
    float t_max;
};

void _addRaycastSpan(
    inout RaycastResult result,
    Ray ray,
    vec3 t_side_v,
    vec3 t_last_side_v,
    uint voxel_index,
    uvec3 mat
) {
    // Calculate ray length
    float t_side_min = max(t_side_v.x, max(t_side_v.y, t_side_v.z));
    float t_last_side_min = max(t_last_side_v.x, max(t_last_side_v.y, t_last_side_v.z));
    result.distance = t_side_min;

    // Initialize and add 3 bit normal to voxel material (first bit is sign, next two are 1, 2 or 3 = x, y or z)
    uint normal = 0u;
    if (t_last_side_v.x >= t_last_side_min) normal = 2u | (floatBitsToUint(ray.ray.x) >> 31);
    else if (t_last_side_v.y >= t_last_side_min) normal = 4u | (floatBitsToUint(ray.ray.y) >> 31);
    else if (t_last_side_v.z >= t_last_side_min) normal = 6u | (floatBitsToUint(ray.ray.z) >> 31);
    mat.z |= normal;

    // Use local array compile-time optimization (search LOCAL ARRAY OPTIMIZATION)
    int index = result.count++;
    for (int i = 0; i < 6; i++) {
        if (i == index) {
            // If we can merge new span with last - merge them
            if (i > 0 &&
                result.spans[i - 1].span.y * MATERIAL_SPAN_MERGE_COEF > t_last_side_min &&     // spans are close enough
                result.spans[i - 1].mat.xy == mat.xy &&                                        // materials match
                t_last_side_min != t_side_min                                                  // and this is not last span
            ) {
                result.spans[i - 1].span.y = t_side_min;
                result.count--;
            // Else initialize new span and break
            } else {
                result.spans[i].mat = mat;
                result.spans[i].span = vec2(t_last_side_min, t_side_min);
            }
            break;
        }
    }
}

RaycastResultSpan getRaycastResultSpan(RaycastResult result, int k) {
    for (int i = 0; i < 6; i++) {
        if (i == k) {
            return result.spans[i];
        }
    }
    RaycastResultSpan r;
    return r;
}

vec3 getRaycastSpanStart(Ray ray, RaycastResultSpan span) {
    return ray.start + span.span.x * ray.ray + getVoxelNormalFromMaterial(span.mat.z) * span.span.x * 1e-3;
}

uvec3 __hash3to3(uvec3 x) {
    const uint k = 1103515245u;
    x *= k;
    x = ((x >> 2u) ^ (x.yzx >> 1u) ^ x.zxy) * k;
    x = ((x >> 2u) ^ (x.yzx >> 1u) ^ x.zxy) * k;
    return x;
}

uint __hash3to1(uvec3 x) {
    const uint k = 1103515245u;
    x = __hash3to3(x);
    return (x.x >> 2u) ^ (x.y >> 1u) ^ x.z;
}

uint getVoxelPositionHash(vec3 position, uint material_z) {
    uint scale_i = (material_z >> 3u) & 31u;
    position += getVoxelNormalFromMaterial(material_z) * 0.0000001;

    uint chunk_pos_hash = __hash3to1(uvec3(ivec3(floor(position))));
    uint voxel_pos_hash = __hash3to1((floatBitsToUint(fract(position) + 1.0) & 0x7FFFFFu) >> (scale_i + 1u));
    return __hash3to1(uvec3(chunk_pos_hash, voxel_pos_hash, 0));
}

// Raycast over one voxel chunk, main logic is placed here
// returns true, if ray has ended

bool raycastVoxelChunk(
    inout RaycastResult result,              // - result
    inout MaterialRaycastData material_data, // - struct, containing current material span
    Ray ray,                                 // incoming ray
    vec3 chunk_offset,                       // offset of the chunk (position of the lower corner of the chunk cube)
    uint chunk_pointer,                      // pointer to the chunk root
    int max_spans                            // maximum amount of returned spans of translucent voxels
) {
    // Size of float mantissa.
    int s_max = 23;

    // Precalculate values for calculating t.
    // p(t) = p + t * d
    // d = ray.ray, p = ray.start;
    // t_v(v) = (1 / d) * v + (-p / d)
    // t_v(v) = t_coef * v + t_bias;
    vec3 t_coef = 1.0 / -abs(ray.ray);
    vec3 t_bias = -ray.start / ray.ray;

    // All algorithm assumes, that all ray components are negative.
    // Determine, in which octant ray is directed, for positive axis directions flip position axes.
    int octant_mask = 7;
    if (ray.ray.x > 0.0) { octant_mask ^= 1; chunk_offset.x = -(chunk_offset.x + 0.5) - 0.5; }
    if (ray.ray.y > 0.0) { octant_mask ^= 2; chunk_offset.y = -(chunk_offset.y + 0.5) - 0.5; }
    if (ray.ray.z > 0.0) { octant_mask ^= 4; chunk_offset.z = -(chunk_offset.z + 0.5) - 0.5; }

    // Initialize stack and current node.
    RaycastStackNode stack[24];
    RaycastStackNode current;
    current.voxel_pointer = chunk_pointer;

    // Initialize other variables to start at the chunk root.
    vec3 pos = chunk_offset;
    int scale_i = s_max;
    float scale_exp = 1.0;
    int idx = octant_mask;

    // Determine t_min and t_max for the root chunk cube positions.
    vec3 t_start_v = (pos + scale_exp) * t_coef + t_bias;
    vec3 t_end_v = pos * t_coef + t_bias;
    float t_min = max(0.0, max(t_start_v.x, max(t_start_v.y, t_start_v.z)));
    current.t_max = min(t_end_v.x, min(t_end_v.y, t_end_v.z));

    // Store t_max, to use it, in case material span is ended after we advance to next chunk into non-existing chunk
    material_data.t_chunk_end = current.t_max;

    // Start the loop.
    while (result.steps++ < MAX_STEPS_PER_RAY) {
        bool on_mat_to_air_edge = false;

        // T value of current parent voxel cube corner.
        vec3 t_corner_v = pos * t_coef + t_bias;
        float t_corner_max = min(t_corner_v.x, min(t_corner_v.y, t_corner_v.z));

        // Check we intersect the parent voxel
        if (t_min < current.t_max) {
            // Get voxel header integer at the current voxel pointer.
            uint voxel_header = u_voxel_buffer.data[current.voxel_pointer];

            // If voxel marked as full, we hit it.
            if ((voxel_header & 0xC0000000u) == 0x40000000u) {
                material_data.in_voxel = true;

                // Get voxel material and check for material change
                uvec2 voxel_material = uvec2(voxel_header, u_voxel_buffer.data[current.voxel_pointer + 1u]);
                if (voxel_material != material_data.last_mat.xy) {
                    // Calculate t value at the highest corner of the cube, from this side the ray is going at the cube.
                    vec3 t_side_v = (pos + scale_exp) * t_coef + t_bias;

                    // If last material is not 0, add span of it
                    if (material_data.last_mat.x != 0u) {
                        _addRaycastSpan(result, ray, t_side_v, material_data.t_last_side_v, current.voxel_pointer, material_data.last_mat);
                    }

                    // If we have reached span limit, or voxel is fully opaque with blend mode 0,
                    // add last material with zero-length span and return true (ray end has been reached).
                    if ((voxel_material & uvec2(0x3E000000u, 0x000000Fu)) == uvec2(31u << 25u, 0u) || result.count >= max_spans - 1) {
                        _addRaycastSpan(result, ray, t_side_v, t_side_v, current.voxel_pointer, uvec3(voxel_material, scale_i << 3));
                        return true;
                    }

                    // Update last material
                    material_data.last_mat = uvec3(voxel_material, scale_i << 3);
                    material_data.t_last_side_v = t_side_v;
                }
            // If we were in voxel, it means we advanced out of it
            } else if (material_data.in_voxel) {
                on_mat_to_air_edge = true;
            }

            // Get max t value of current child voxel cube.
            float t_voxel_max = min(current.t_max, t_corner_max);

            // If voxel has children and we intersect the current child
            if ((voxel_header & 0x80000000u) != 0u && t_min < t_voxel_max && scale_i > 0) {
                // PUSH

                // Get child pointer for current idx and check it exists.
                uint child = u_voxel_buffer.data[current.voxel_pointer + 2u + uint(idx ^ octant_mask)];
                if (child != 0u) {
                    // Put current voxel_pointer and t_max on stack at the current scale
                    #ifdef OPTIMIZE_RAYTRACE_STACK
                        // We use binary search to reduce search time of required index, search LOCAL ARRAY OPTIMIZATION
                        if (scale_i > 12) {
                            if (scale_i > 18) {
                                if (scale_i > 21) {
                                    if (scale_i > 22) {
                                        stack[23] = current;
                                    } else {
                                        stack[22] = current;
                                    }
                                } else {
                                    if (scale_i > 20) {
                                        stack[21] = current;
                                    } else {
                                        if (scale_i > 19) {
                                            stack[20] = current;
                                        } else {
                                            stack[19] = current;
                                        }
                                    }
                                }
                            } else {
                                if (scale_i > 16) {
                                    if (scale_i > 17) {
                                        stack[18] = current;
                                    } else {
                                        stack[17] = current;
                                    }
                                } else {
                                    if (scale_i > 14) {
                                        if (scale_i > 15) {
                                            stack[16] = current;
                                        } else {
                                            stack[15] = current;
                                        }
                                    } else {
                                        if (scale_i > 13) {
                                            stack[14] = current;
                                        } else {
                                            stack[13] = current;
                                        }
                                    }
                                }
                            }
                        } else {
                            if (scale_i > 6) {
                                if (scale_i > 10) {
                                    if (scale_i > 11) {
                                        stack[12] = current;
                                    } else {
                                        stack[11] = current;
                                    }
                                } else {
                                    if (scale_i > 8) {
                                        if (scale_i > 9) {
                                            stack[10] = current;
                                        } else {
                                            stack[9] = current;
                                        }
                                    } else {
                                        if (scale_i > 7) {
                                            stack[8] = current;
                                        } else {
                                            stack[7] = current;
                                        }
                                    }
                                }
                            } else {
                                #ifndef OPTIMIZE_RAYTRACE_STACK_DISABLE_LOW_SCALES
                                if (scale_i > 4) {
                                    if (scale_i > 5) {
                                        stack[6] = current;
                                    } else {
                                        stack[5] = current;
                                    }
                                } else {
                                    if (scale_i > 2) {
                                        if (scale_i > 3) {
                                            stack[4] = current;
                                        } else {
                                            stack[3] = current;
                                        }
                                    } else {
                                        if (scale_i > 1) {
                                            stack[2] = current;
                                        } else if (scale_i > 0) {
                                            stack[1] = current;
                                        } else {
                                            stack[0] = current;
                                        }
                                    }
                                }
                                #endif
                            }
                        }
                    #else
                        stack[scale_i] = current;
                    #endif
                    current.voxel_pointer += child;

                    // Go down 1 scale and reset idx.
                    scale_i--;
                    scale_exp = scale_exp * 0.5f;
                    idx = 0;

                    // Get t values of the center of the current cube for each axis. T_min is evaluated at the lowest cube corner,
                    // if for some axis t value is larger than t_min, it means that for this axis it has greater offset.
                    // Considering this, calculate new idx and position of the cube, we entered.
                    vec3 t_center_v = scale_exp * t_coef + t_corner_v; // (pos + scale_exp * .5) * t_coef + t_bias;
                    if (t_center_v.x > t_min) { idx ^= 1; pos.x += scale_exp; }
                    if (t_center_v.y > t_min) { idx ^= 2; pos.y += scale_exp; }
                    if (t_center_v.z > t_min) { idx ^= 4; pos.z += scale_exp; }

                    // Update t max and continue.
                    current.t_max = t_voxel_max;
                    continue;
                }
            }
        }

        // If we advanced out of material and have not pushed, it means, that we need to end current material span
        if (on_mat_to_air_edge) {
            material_data.in_voxel = false;

            // Calculate t value at the highest corner of the cube, from this side the ray is going at the cube.
            vec3 t_side_v = (pos + scale_exp) * t_coef + t_bias;

            // Add last material span (material_data.in_voxel was true, so we advanced into material
            _addRaycastSpan(result, ray, t_side_v, material_data.t_last_side_v, current.voxel_pointer, material_data.last_mat);

            // Set material to zero
            material_data.last_mat = uvec3(0u);
            material_data.t_last_side_v = t_side_v;
        }

        // ADVANCE

        // If t_value for some axis is at the lowest cube corner, advance that axis and update step mask and then idx.
        int step_mask = 0;
        if (t_corner_v.x <= t_corner_max) { step_mask ^= 1; pos.x -= scale_exp; }
        if (t_corner_v.y <= t_corner_max) { step_mask ^= 2; pos.y -= scale_exp; }
        if (t_corner_v.z <= t_corner_max) { step_mask ^= 4; pos.z -= scale_exp; }
        idx ^= step_mask;

        // Update t min.
        t_min = t_corner_max;

        // If we stepped out of current cube (because all ray direction axes are negative, we bring this check down to one bitwise operation)
        if ((idx & step_mask) != 0) {
            // POP

            // Do a lot of float bit operation magic to pop out and advance at the same time.
            // At the current moment we stepped out of the previous cube and already in the next one.
            // See image for more details - https://i.imgur.com/LulaRme.png

            // Make current cube offset to be in range [1; 2], this way float power bits will always have the same first
            // 9 bits - sign and exponent. This way the difference between coordinates will always be in mantissa.
            pos = fract(pos) + 1.0;

            // Now for each coordinate, if we made step along it, find changed bits. The highest changed bit position will be equal
            // to scale of the cube, we popped & advanced into, because it will indicate largest fraction we stepped over.
            // Example: 1.75 - 0.0625 = 101100..., 1.75 = 110000..., highest difference bit is at position 21, so we are popped into scale 0.25, which is true.
            uint differing_bits = 0u;
            if ((step_mask & 1) != 0) differing_bits |= floatBitsToUint(pos.x) ^ floatBitsToUint(pos.x + scale_exp);
            if ((step_mask & 2) != 0) differing_bits |= floatBitsToUint(pos.y) ^ floatBitsToUint(pos.y + scale_exp);
            if ((step_mask & 4) != 0) differing_bits |= floatBitsToUint(pos.z) ^ floatBitsToUint(pos.z + scale_exp);

            // Calculate the position of highest changed bit, differing_bits is equal to 2^highest_bit + ...,
            // when converted to float, its exponent will be equal to highest_bit + 127, so we extract it.
            scale_i = (floatBitsToInt(float(differing_bits)) >> 23) - 127;


            // This is just float(2^(scale_i - 23)) using some float magic
            scale_exp = intBitsToFloat((scale_i + 127 - s_max) << 23);

            // Now, using scale, extract voxel_pointer and t_max from stack at this scale
            #ifdef OPTIMIZE_RAYTRACE_STACK
                // We use binary search to reduce search time of required index, search LOCAL ARRAY OPTIMIZATION
                if (scale_i > 12) {
                    if (scale_i > 18) {
                        if (scale_i > 21) {
                            if (scale_i > 22) {
                                current = stack[23];
                            } else {
                                current = stack[22];
                            }
                        } else {
                            if (scale_i > 20) {
                                current = stack[21];
                            } else {
                                if (scale_i > 19) {
                                    current = stack[20];
                                } else {
                                    current = stack[19];
                                }
                            }
                        }
                    } else {
                        if (scale_i > 16) {
                            if (scale_i > 17) {
                                current = stack[18];
                            } else {
                                current = stack[17];
                            }
                        } else {
                            if (scale_i > 14) {
                                if (scale_i > 15) {
                                    current = stack[16];
                                } else {
                                    current = stack[15];
                                }
                            } else {
                                if (scale_i > 13) {
                                    current = stack[14];
                                } else {
                                    current = stack[13];
                                }
                            }
                        }
                    }
                } else {
                     if (scale_i > 6) {
                         if (scale_i > 10) {
                             if (scale_i > 11) {
                                 current = stack[12];
                             } else {
                                 current = stack[11];
                             }
                         } else {
                             if (scale_i > 8) {
                                 if (scale_i > 9) {
                                     current = stack[10];
                                 } else {
                                     current = stack[9];
                                 }
                             } else {
                                 if (scale_i > 7) {
                                     current = stack[8];
                                 } else {
                                     current = stack[7];
                                 }
                             }
                         }
                     } else {
                         #ifndef OPTIMIZE_RAYTRACE_STACK_DISABLE_LOW_SCALES
                         if (scale_i > 4) {
                             if (scale_i > 5) {
                                 current = stack[6];
                             } else {
                                 current = stack[5];
                             }
                         } else {
                             if (scale_i > 2) {
                                 if (scale_i > 3) {
                                     current = stack[4];
                                 } else {
                                     current = stack[3];
                                 }
                             } else {
                                 if (scale_i > 1) {
                                     current = stack[2];
                                 } else if (scale_i > 0) {
                                     current = stack[1];
                                 } else {
                                     current = stack[0];
                                 }
                             }
                         }
                         #endif
                     }
                 }

            #else
                current = stack[scale_i];
            #endif

            // This will floor position to current scale, so it will be pointing to the lowest corner of the cube (see image above)
            int shx = floatBitsToInt(pos.x) >> scale_i;
            int shy = floatBitsToInt(pos.y) >> scale_i;
            int shz = floatBitsToInt(pos.z) >> scale_i;
            pos.x = intBitsToFloat(shx << scale_i);
            pos.y = intBitsToFloat(shy << scale_i);
            pos.z = intBitsToFloat(shz << scale_i);

            // Use first bit after floored bits to calculate new idx.
            idx = (shx & 1) | ((shy & 1) << 1) | ((shz & 1) << 2);

            // Restore position.
            pos = chunk_offset + fract(pos);

            // If we popped out of chunk, return false, because the end is not reached yet
            if (scale_i >= s_max) {
                return false;
            }
        }
    }

    // We have reached step limit
    return true;
}

RaycastResult raycastVoxelWorld(Ray ray, int max_chunks, int max_spans, int fetch_weight) {
    ChunkRaycastData chunk_raycast;

    if (abs(ray.ray.x) < 1e-23) ray.ray.x = 1e-23 * sign(ray.ray.x);
    if (abs(ray.ray.y) < 1e-23) ray.ray.y = 1e-23 * sign(ray.ray.y);
    if (abs(ray.ray.z) < 1e-23) ray.ray.z = 1e-23 * sign(ray.ray.z);

    vec3 r2 = ray.ray * ray.ray;
    chunk_raycast.rayS = vec3(
        sqrt(1.0 + r2.y / r2.x + r2.z / r2.x),
        sqrt(1.0 + r2.x / r2.y + r2.z / r2.y),
        sqrt(1.0 + r2.x / r2.z + r2.y / r2.z)
    );

    chunk_raycast.pos = ivec3(floor(ray.start));
    chunk_raycast.pos_step = ivec3(ray.ray.x > 0.0 ? 1 : -1, ray.ray.y > 0.0 ? 1 : -1, ray.ray.z > 0.0 ? 1 : -1);

    chunk_raycast.rayL = vec3(
            (ray.ray.x > 0.0 ? (float(chunk_raycast.pos.x + 1) - ray.start.x) : (ray.start.x - float(chunk_raycast.pos.x))) * chunk_raycast.rayS.x,
            (ray.ray.y > 0.0 ? (float(chunk_raycast.pos.y + 1) - ray.start.y) : (ray.start.y - float(chunk_raycast.pos.y))) * chunk_raycast.rayS.y,
            (ray.ray.z > 0.0 ? (float(chunk_raycast.pos.z + 1) - ray.start.z) : (ray.start.z - float(chunk_raycast.pos.z))) * chunk_raycast.rayS.z
        );

    RaycastResult result;
    result.count = 0;
    result.steps = 0u;
    result.distance = POSITIVE_INF;

    MaterialRaycastData material_data;
    material_data.last_mat = uvec3(0u);
    material_data.in_voxel = false;

    ivec3 chunk_map_offset = ivec3(u_voxel_chunk_map.data[0], u_voxel_chunk_map.data[1], u_voxel_chunk_map.data[2]);
    ivec3 chunk_map_size = ivec3(u_voxel_chunk_map.data[3], u_voxel_chunk_map.data[4], u_voxel_chunk_map.data[5]);
    for (int i = 0; i < max_chunks; i++) {
        // Lookup chunk by position, and if it is found, raycast it
        uint chunk_pointer = lookupVoxelChunk(chunk_raycast.pos, chunk_map_offset, chunk_map_size, fetch_weight--);
        if (chunk_pointer != uint(-1)) {
            // If ray end has been reached, break out of loop
            if (raycastVoxelChunk(result, material_data, ray, vec3(chunk_raycast.pos), chunk_pointer, max_spans)) {
                break;
            }
        // If we advanced out of chunk, and have pending material span, finish it
        } else if (material_data.in_voxel) {
            material_data.in_voxel = false;

            // Add last material span (result.in_voxel was true, so we advanced into material
            _addRaycastSpan(result, ray, vec3(material_data.t_chunk_end), material_data.t_last_side_v, 0u, material_data.last_mat);

            // Set material to zero
            material_data.last_mat = uvec3(0u);
            material_data.t_last_side_v = vec3(material_data.t_chunk_end);
        }

        // Make DDA step to advance to next chunk
        bvec3 mask = lessThanEqual(chunk_raycast.rayL, min(chunk_raycast.rayL.yzx, chunk_raycast.rayL.zxy));
        chunk_raycast.rayL += vec3(mask) * chunk_raycast.rayS;
        chunk_raycast.pos += ivec3(mask) * chunk_raycast.pos_step;
        result.steps++;
    }

    // Calculate ray end
    result.end = ray.start + ray.ray * result.distance;

    return result;
}

#endif  // VOXEL_ENGINE_RAYCAST_INC