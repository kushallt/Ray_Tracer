#include "bvh_accelerator.h"

int BVHAccelerator::flattenBVHTree(BVHbuildnode *node, int *offset){
    LinearBVHNode *linearnode = &nodes[*offset];
    linearnode->bounds = node->bounds;
    int newoffset = (*offset)++;

    if(node->nhittables > 0){
        linearnode->hittablesOffset = node->firstoffset;
        linearnode->nhittables = node->nhittables;
    }
    else{
        linearnode->axis = node->splitAxis;
        linearnode->nhittables = 0;
        flattenBVHTree(node->children[0], offset);
        linearnode->secondChildOffset = flattenBVHTree(node->children[1], offset);
    }
    return newoffset;
}