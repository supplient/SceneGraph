#pragma once

// TODO C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v11.1\include\crt\common_functions.h
// line 160, 161
//	I commented these two lines to pass the compling,
//	But actually I don't know why and what it is.

namespace sim {
	constexpr int	n = 64;
	constexpr float lConst = 2.0f / float(n);
	constexpr float lCutConst = lConst * 1.41421356f;
	constexpr float lBendConst = lConst * 2;
	constexpr float m = 0.008f; // kg
	constexpr float gravityConstant = 9.8f;
	constexpr float timestep = 0.001f;
};
