#version 400 core
#define	HERMITE_GMT			1
#define	BEZIER_GMT			2
#define	BEZIER_BERNSTEIN	3

layout (isolines, equal_spacing, ccw) in;

uniform mat4	matModelView;
uniform mat4	matProjection;
uniform int		curveType;
uniform int		controlPointsNumber;

// Új: Uniform tömb a pontoknak (128 pontig)
uniform vec3 u_ControlPoints[128];
/** hermite és bezier mátrixok írják le a Hermite ívet és Bezier görbét oszlopfolytonosan megadva. */
/** hermite and bezier matrices describe the Hermite and Bezier curves where they are column continous. */
// https://gyires.inf.unideb.hu/mobiDiak/Tornai-Robert/Fejezetek-a-szamitogepi-grafikabol/fej_graf.pdf
// https://www.cs.colostate.edu/~cs410/yr2016fa/more_progress/cs410_F16_Lecture18.pdf
// https://www.cs.umd.edu/~reastman/slides/L19P01ParametricCurves.pdf
// http://cgpp.net/file/cgpp3e_ch22W.pdf
const mat4x4	hermite	= mat4x4( 2, -2,  1,  1,
								 -3,  3, -2, -1,
								  0,  0,  1,  0,
								  1,  0,  0,  0);
// https://en.wikipedia.org/wiki/B%C3%A9zier_curve#Explicit_definition
// https://hu.wikipedia.org/wiki/B%C3%A9zier-g%C3%B6rbe
const mat4x4	bezier	= mat4x4(-1,  3, -3,  1,
								  3, -6,  3,  0,
								 -3,  3,  0,  0,
								  1,  0,  0,  0);
/** A megadott paraméterû görbe pontjainak meghatározása. */
/** Determining the points of a curve of the given parameters. */
vec3 GMT(const mat4x3 G, const mat4x4 M, const float t) {
	mat4x3	C = G * M;
	vec4	T = vec4(t * t * t, t * t, t, 1.0f);
	vec3	P = C * T;

	return P;
}
/** N alatt R kiszámolása a lehetséges átrendezéses optimalizációval.
The definition of N choose R is to compute the two products and divide one with the other,
(N * (N - 1) * (N - 2) * ... * (N - R + 1)) / (1 * 2 * 3 * ... * R)
However, the multiplications may become too large really quick and overflow existing data type.
The implementation trick is to reorder the multiplicationand divisions as,
(N) / 1 * (N - 1) / 2 * (N - 2) / 3 * ... * (N - R + 1) / R
It's guaranteed that at each step the results is divisible 
(for n continuous numbers, one of them must be divisible by n, so is the product of these numbers).
For example, for N choose 3, at least one of the N, N - 1, N - 2 will be a multiple of 3, 
and for N choose 4, at least one of N, N - 1, N - 2, N - 3 will be a multiple of 4. */
float NCR(int n, int r) {
/** Binomiális együttható. */
/** Binomial coefficient. */
	if (r == 0) return 1;
/** Extra computation saving for large R, using property: N choose R = N choose (N - R) */
//	if (r > n / 2) return NCR(n, n - r); // glsl does NOT support recursive optimization!
	double result = 1.0f;

	for (int k = 1; k <= r; ++k) {
		result *= n - k + 1;
		result /= k;
	}

	return float(result);
}
/** n-ed fokú Bernstein bázis polinom. */
/** It will be the Bernstein basis polynomial of degree n. */
float blending(int n, int i, float t) {
    // Kézzel kezeljük a 0^0 eseteket a stabilitásért
    float term1 = (i == 0) ? 1.0f : pow(t, float(i));
    float term2 = (n == i) ? 1.0f : pow(1.0f - t, float(n - i));
    
    return NCR(n, i) * term1 * term2;
}
/** Bezier görbe pontja t paraméterhez. */
/** Computes Bezier curve point for parameter t. */
vec3 BezierCurve(float t) {
    vec3 nextPoint = vec3(0.0);
    for (int i = 0; i < controlPointsNumber; i++) {
        // gl_in[i].gl_Position HELYETT u_ControlPoints[i]
        nextPoint += blending(controlPointsNumber - 1, i, t) * u_ControlPoints[i];
    }
    return nextPoint;
}

void main() {
    float t = gl_TessCoord.x;
    vec3 finalPos = BezierCurve(t);
    gl_Position = matProjection * matModelView * vec4(finalPos, 1.0);
}