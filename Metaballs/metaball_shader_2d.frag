# version 120

/** The maximum number of charges that can be passed to this shader. */
#define MAX_CHARGES 75

/** Container for charges, may not be full. */
uniform vec4 charges[MAX_CHARGES];

/** The actual number of charges. */
uniform int charge_count;

/** The treshold charge. */
uniform float threshold;

void main()
{
	float charge = 0.0;
	for (int i = 0; i < charge_count; ++i) {
		float dist = distance(charges[i].xy, gl_FragCoord.xy);
		if (dist == 0.0) {
			charge = threshold;
			break;
		}
		float r = dist / charges[i].w;
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