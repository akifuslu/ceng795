#pragma once
#include "miniply.h"
#include <iostream>

// Very basic triangle mesh struct, for example purposes
struct TriMesh {
  // Per-vertex data
  float* pos     = nullptr; // has 3 * numVerts elements.
  float* uv      = nullptr; // if non-null, has 2 * numVerts elements.
  uint32_t numVerts   = 0;

  // Per-index data
  int* indices   = nullptr;
  uint32_t numIndices = 0; // number of indices = 3 times the number of triangles.
};


static TriMesh* load_trimesh_from_ply(const char* filename)
{
  miniply::PLYReader reader(filename);
  if (!reader.valid()) {
    return nullptr;
  }

  uint32_t indexes[3];
  bool gotVerts = false, gotFaces = false;

  TriMesh* trimesh = new TriMesh();
  while (reader.has_element() && (!gotVerts || !gotFaces)) {
    if (reader.element_is(miniply::kPLYVertexElement) && reader.load_element() && reader.find_pos(indexes)) {
      trimesh->numVerts = reader.num_rows();
      trimesh->pos = new float[trimesh->numVerts * 3];
      reader.extract_properties(indexes, 3, miniply::PLYPropertyType::Float, trimesh->pos);
      if (reader.find_texcoord(indexes)) {
        trimesh->uv = new float[trimesh->numVerts * 2];
        reader.extract_properties(indexes, 2, miniply::PLYPropertyType::Float, trimesh->uv);
      }
      gotVerts = true;
    }
    else if (reader.element_is(miniply::kPLYFaceElement) && reader.load_element() && reader.find_indices(indexes)) {
      bool polys = reader.requires_triangulation(indexes[0]);
      if (polys && !gotVerts) {
        fprintf(stderr, "Error: need vertex positions to triangulate faces.\n");
        break;
      }
      if (polys) {
        trimesh->numIndices = reader.num_triangles(indexes[0]) * 3;
        trimesh->indices = new int[trimesh->numIndices];
        reader.extract_triangles(indexes[0], trimesh->pos, trimesh->numVerts, miniply::PLYPropertyType::Int, trimesh->indices);
      }
      else {
        trimesh->numIndices = reader.num_rows() * 3;
        trimesh->indices = new int[trimesh->numIndices];
        reader.extract_list_property(indexes[0], miniply::PLYPropertyType::Int, trimesh->indices);
      }
      gotFaces = true;
    }
    if (gotVerts && gotFaces) {
      break;
    }
    reader.next_element();
  }

  if (!gotVerts || !gotFaces) {
    delete trimesh;
    return nullptr;
  }

  return trimesh;
}