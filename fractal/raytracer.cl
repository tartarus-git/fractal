__kernel void traceRays(__write_only image2d_t frame, uint frameWidth, uint frameHeight, float3 cameraPos, float3 cameraRotation) {
    int x = get_global_id(0);
    if (x >= frameWidth) { return; }
    int2 coords = (int2)(x, get_global_id(1));

    write_imageui(frame, coords, (uint4)(0, 255, 0, 1));
}