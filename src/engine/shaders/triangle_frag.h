static char const triangle_frag_wgsl[] = R"(
	@stage(fragment)
	fn main(@location(0) vCol : vec3<f32>) -> @location(0) vec4<f32> {
		return vec4<f32>(vCol, 1.0);
	}
)";

