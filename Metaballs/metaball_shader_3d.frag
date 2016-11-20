// Container for metaballs, may not be full.
uniform vec4 metaballs[100];

// The actual number of metaballs.
uniform int metaball_count;

// The treshold charge.
uniform float threshold;

void main()
{
	if (gl_FragCoord.x < 10) {
		gl_FragColor = vec4(1, 0, 0, 1);
	} else {
		gl_FragColor = vec4(0, 0, 1, 1);
	}
}