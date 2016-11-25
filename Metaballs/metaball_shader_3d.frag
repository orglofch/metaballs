# version 120

/** The maximum number of metaballs that can be passed to this shader. */
#define MAX_METABALLS 80

#define INF 1.0 / 0.0;

/** Container for metaballs, may not be full. */
uniform vec4 metaballs[MAX_METABALLS];

/** The actual number of metaballs. */
uniform int metaball_count;

/** The threshold charge for a meta surface. */
uniform float threshold;

/** The camera origin. */
uniform vec3 origin;

/** The time / iteration. */
uniform int time;

uniform mat4 camera_matrix;

/**
 * Calculates the intersections with the given metaball sphere, storing it in passed argument.
 *
 * @param ro The origin of the cast ray.
 * @param dir The direction of the cast ray.
 * @param metaball_index The index of the metaball sphere to intersect.
 * @param intersections The structures for storing the intersection results.
 * @return The number of intersections found.
 */
int findSphereIntersection(vec3 ro, vec3 dir, int metaball_index, inout float roots[2]) {
	// TODO(orglofch): This still returns negatives, which we don't care about.
	vec3 from_center = ro - metaballs[metaball_index].xyz;

	float a = dot(dir, dir);
	float b = 2.0 * dot(from_center, dir);
	float c = dot(from_center, from_center) 
		- metaballs[metaball_index].w*metaballs[metaball_index].w;

	if (a == 0) {
		if (b == 0) {
			return 0;
		} else {
			roots[0] = -c / b;
			return 1;
		}
	} else {
		float det = b*b - 4.0 * a*c;
		if (det < 0.0) {
			return 0;
		} else {
			float q = -(b + sign(b)*sqrt(det)) / 2.0;
			roots[0] = q / a;

			if (q != 0) {
				roots[1] = c / q;
			} else {
				roots[1] = roots[0];
			}
			return 2;
		}
	}
	return 0;
}

void main()
{
	vec3 pixel_in_world = (vec4(gl_FragCoord.xy, 0, 1) * camera_matrix).xyz;
	vec3 rd = normalize(pixel_in_world - origin);

	// Find intersection with the ray and the metaball spheres.
	// The charge function we use = 0 for dist > metaball radius
	// so we can ignore metaballs which fall outside this range.
	int active_metaballs[MAX_METABALLS];
	int intersection_count = 0;

	// Keep track of the min and max t values so we only have to
	// iterate between the two points these define.
	// TODO(orglofch): Store all intersections and their type
	// E.g. ENTER, EXIT, so we can skip gaps between influence spheres.
	float min_t = INF;
	float max_t = 0.0;

	for (int i = 0; i < metaball_count; ++i) {
		float roots[2];
		int num_roots = findSphereIntersection(origin, rd, i, roots);
		if (num_roots > 0) {
			active_metaballs[intersection_count++] = i;
			if (num_roots == 1) {
				max_t = max(max_t, roots[0]);
				min_t = min(min_t, roots[0]);
			} else if (num_roots == 2) {
				max_t = max(max_t, max(roots[0], roots[1]));
				min_t = min(min_t, min(roots[0], roots[1]));
			}
		}
	}

	// If there are no intersection then use the backgrounds colour.
	if (intersection_count == 0) {
		gl_FragColor = vec4(0);
		return;
	}

	// Ray march between [min_t, max_t] to approximate if this
	// ray ever intersects a meta surface.
	float cur_t = min_t;
	while (cur_t < max_t) {
		vec3 pos = origin + rd * cur_t;
		float step_charge = 0.0;
		for (int i = 0; i < intersection_count; ++i) {
			vec4 metaball = metaballs[active_metaballs[i]];
			float dist = distance(metaball.xyz, pos.xyz);
			float r = dist / metaball.w;
			if (r <= 1 && r >= 0) {
				step_charge += 1.0 - r*r*r * (r * (r * 6.0 - 15.0) + 10.0);
			}
			if (step_charge >= 0.4) {
				float dist = 1 - (distance(origin, pos) - 400.0) / 500.0;
				dist = dist * dist;
				gl_FragColor = vec4(dist / 2, dist / 2, dist, 1);
				return;
			}
		}
		// Take larger steps the lower our step charge (the farther we are
		// from the meta surface)
		float step = min(1.0, step_charge);
		cur_t += 6.0 - 6.0 * step*step*step * (step * (step * 6.0 - 15.0) + 10.0);
	}
	gl_FragColor = vec4(0);
}