# version 120

/** The maximum number of metaballs that can be passed to this shader. */
#define MAX_METABALLS 75

/** Container for metaballs, may not be full. */
uniform vec4 metaballs[MAX_METABALLS];

/** The actual number of metaballs. */
uniform int metaball_count;

/** The treshold charge. */
uniform float threshold;

void main()
{
	float charge = 0.0;
	for (int i = 0; i < metaball_count; ++i) {
		float dist = distance(metaballs[i].xy, gl_FragCoord.xy);
		if (dist == 0.0) {
			charge = threshold;
			break;
		}
		float r = dist / metaballs[i].w;
		charge += 1 / (r*r);
	}

	charge /= 1000.0;
	if (charge > 0.9) {
		charge = pow(charge, 3);
		gl_FragColor = vec4(charge / 2, charge / 2, charge, 1);
	} else {
		gl_FragColor = vec4(charge / 2, charge / 2, charge, 1);
	}
}