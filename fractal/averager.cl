__kernel void doAverage(__read_only image2d_t beforeAverageFrame, uint beforeAverageFrameWidth, uint beforeAverageFrameHeight, 
						ushort samplesPerPixelSideLength, __write_only image2d_t frame, uint frameWidth, uint frameHeight) {

	int x = get_global_id(0);
	if (x >= frameWidth) { return; }                    // WARNING: Comparison of signed with unsigned. Only works when both are positive like they are here.
	int2 coords = (int2)(x, get_global_id(1));          // REASON: signed is converted into unsigned, causing negative numbers to be super large.

	int2 beforeAverageCoords = coords * samplesPerPixelSideLength;

	uint4 color = (uint4)(0, 0, 0, 1);
	for (ushort y = 0; y < samplesPerPixelSideLength; y++) {
		for (ushort x = 0; x < samplesPerPixelSideLength; x++) {
			color += read_imageui(beforeAverageFrame, (int2)(beforeAverageCoords.x + x, beforeAverageCoords.y + y));
		}
	}

	write_imageui(frame, coords, color / (samplesPerPixelSideLength * samplesPerPixelSideLength));

}