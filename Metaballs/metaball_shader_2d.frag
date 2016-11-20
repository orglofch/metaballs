// Container for metaballs, may not be full.
uniform vec4 metaballs[100];

// The actual number of metaballs.
uniform int metaball_count;

// The treshold charge.
uniform float threshold;

void main()
{
	float charge = 0;
	for (int i = 0; i < metaball_count; ++i) {
		float dist = length(metaballs[i].xy - gl_FragCoord.xy);
		if (dist <= 0) {
			charge = threshold;
			break;
		}
		charge += metaballs[i].w * metaballs[i].w / (dist * dist);
	}

	charge /= 500;
	if (charge > 0.8) {
		charge = pow(charge, 2);
		gl_FragColor = vec4(charge, charge, charge, 1);
	} else {
		gl_FragColor = vec4(charge, charge, charge, 1);
	}
}