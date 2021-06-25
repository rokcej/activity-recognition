const DATA_URL = "http://192.168.100.33/data";
const HISTORY_URL = "http://192.168.100.33/history";

// WebGL vertex shader source code
const vsSource = `#version 300 es
	in vec3 aPos;
	in vec3 aNorm;
	uniform mat4 uPVMat;
	uniform mat4 uMMat;

	out vec3 vPos;
	out vec3 vNorm;

	vec3 scale = vec3(0.8, 0.1, 1.0);

	void main() {
		vec4 pos = uMMat * vec4((scale * aPos), 1.0);
		gl_Position = uPVMat * pos;
		vPos = vec3(pos);
		vNorm = mat3(transpose(inverse(uMMat))) * aNorm;
	}
`;
// WebGL fragment shader source code
const fsSource = `#version 300 es
	precision highp float;

	#define NUM_LIGHTS 2

	struct Light {
		vec3 color;
		vec3 pos;
		float intensity;
		float ambient;
	};

	in vec3 vPos;
	in vec3 vNorm;
	uniform vec3 uCameraPos;

	out vec4 oColor;

	Light lights[NUM_LIGHTS] = Light[NUM_LIGHTS](
		Light(
			vec3(1.0, 0.8, 0.5),
			vec3(-2.0, 5.0, -1.0),
			1.0, 0.3
		),
		Light(
			vec3(1.0, 0.6, 0.375),
			vec3(3.0, -6.0, -2.0),
			0.8, 0.0
		)
	);

	float shininess = 16.0;
	vec2 attenuation = vec2(0.1, 0.001);

	void main() {
		vec3 color = vec3(0.0);
		for (int i = 0; i < NUM_LIGHTS; ++i) {
			// Blinn-Phong reflection model
			vec3 lightDir = lights[i].pos - vPos;
			float dist = length(lightDir);
			lightDir = normalize(lightDir);
			vec3 viewDir  = normalize(uCameraPos - vPos);
			vec3 halfDir  = normalize(lightDir + viewDir);

			float specular = pow(max(dot(vNorm, halfDir), 0.0), shininess);
			float diffuse = max(dot(vNorm, lightDir), 0.0);

			color += (lights[i].ambient + (diffuse + specular) * (lights[i].intensity / (1.0 + dist * attenuation.x + dist * dist * attenuation.y))) * lights[i].color;
		}
		oColor = vec4(color, 1.0);
	}	
`;

// Compile a WebGL shader
function createShader(gl, type, source) {
	const shader = gl.createShader(type);
	gl.shaderSource(shader, source);
	gl.compileShader(shader);
	if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
		console.log(gl.getShaderInfoLog(shader));
		gl.deleteShader(shader)
		return null;
	}
	return shader;
}
// Create a WebGL program
function createProgram(gl, vertexShader, fragmentShader) {
	const program = gl.createProgram();
	gl.attachShader(program, vertexShader);
	gl.attachShader(program, fragmentShader);
	gl.linkProgram(program);
	if (!gl.getProgramParameter(program, gl.LINK_STATUS)) {
		console.log(gl.getProgramInfoLog(program));
		gl.deleteProgram(program);
		return null;
	}
	return program;
}

// Matrix functions
// N.B. WebGL matrices are column-major
function normalizeVector(v) {
	const invLen = 1.0 / Math.sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
	v[0] *= invLen;
	v[1] *= invLen;
	v[2] *= invLen;
	return v;
}

function perspectiveMatrix(fovyDeg, aspect, near, far) {
	const fovyRad = fovyDeg * Math.PI / 180.0;
	const f = 1.0 / Math.tan(fovyRad * 0.5);
	const rangeInv = 1.0 / (near - far);
	return new Float32Array([
		f / aspect, 0,                         0,  0,
		0,          f,                         0,  0,
		0,          0,   (near + far) * rangeInv, -1,
		0,          0, 2 * near * far * rangeInv,  0
	]);
}

function lookAtMatrix(eye, center, up) {
	const z = [ // eye - center
		eye[0] - center[0],
		eye[1] - center[1],
		eye[2] - center[2]
	]
	normalizeVector(z);

	const x = [ // vectorProduct(up, z)
		up[1] * z[2] - up[2] * z[1],
		up[2] * z[0] - up[0] * z[2],
		up[0] * z[1] - up[1] * z[0]
	]
	normalizeVector(x);

	const y = [ // vectorProduct(z, x)
		z[1] * x[2] - z[2] * x[1],
		z[2] * x[0] - z[0] * x[2],
		z[0] * x[1] - z[1] * x[0]
	]
	normalizeVector(y);

	const d = [ // dotProduct({x, y, z}, -eye)
		-(x[0] * eye[0] + x[1] * eye[1] + x[2] * eye[2]),
		-(y[0] * eye[0] + y[1] * eye[1] + y[2] * eye[2]),
		-(z[0] * eye[0] + z[1] * eye[1] + z[2] * eye[2])
	]

	return new Float32Array([
		x[0], y[0], z[0], 0,
		x[1], y[1], z[1], 0,
		x[2], y[2], z[2], 0,
		d[0], d[1], d[2], 1
	]);
}

function rotationXMatrix(rad) {
	const s = Math.sin(rad);
	const c = Math.cos(rad);
	return new Float32Array([
		1,  0, 0, 0,
		0,  c, s, 0,
		0, -s, c, 0,
		0,  0, 0, 1
	]);
}

function rotationYMatrix(rad) {
	const s = Math.sin(rad);
	const c = Math.cos(rad);
	return new Float32Array([
		c, 0, -s, 0,
		0, 1,  0, 0,
		s, 0,  c, 0,
		0, 0,  0, 1
	]);
}

function rotationZMatrix(rad) {
	const s = Math.sin(rad);
	const c = Math.cos(rad);
	return new Float32Array([
		 c, s, 0, 0,
		-s, c, 0, 0,
		 0, 0, 1, 0,
		 0, 0, 0, 1
	]);
}

function scalingMatrix(sx, sy, sz) {
	return new Float32Array([
		sx, 0, 0, 0,
		0, sy, 0, 0,
		0, 0, sz, 0,
		0, 0, 0, 1
	]);
}

function translationMatrix(tx, ty, tz) {
	return new Float32Array([
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		tx, ty, tz, 1
	]);
}

function identityMatrix() {
	return new Float32Array([
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	]);
}

function multiplyMatrix(A, B, out) {
	if (!out)
		out = new Float32Array(16)
	for (let i = 0; i < 4; ++i) {
		for (let j = 0; j < 4; ++j) {
			let sum = 0;
			for (let k = 0; k < 4; ++k)
				sum += A[k * 4 + i] * B[j * 4 + k];
			out[j * 4 + i] = sum;
		}
	}
	return out;
}

// Setup WebGL
function initWebGL() {
	const canvas = document.getElementById("canvas");
	const gl = canvas.getContext("webgl2") || canvas.getContext("webgl");
	if (!gl) {
		console.log("Unable to initialize WebGL.");
		return;
	}

	const vs = createShader(gl, gl.VERTEX_SHADER, vsSource);
	const fs = createShader(gl, gl.FRAGMENT_SHADER, fsSource);
	const prog = createProgram(gl, vs, fs);
	if (!prog) {
		console.log("Unable to create program.");
		return;
	}

	const uniforms = {};
	for (let i = 0; i < gl.getProgramParameter(prog, gl.ACTIVE_UNIFORMS); ++i) {
		const info = gl.getActiveUniform(prog, i);
		uniforms[info.name] = gl.getUniformLocation(prog, info.name);
	}

	const attribs = {};
	for (let i = 0; i < gl.getProgramParameter(prog, gl.ACTIVE_ATTRIBUTES); ++i) {
		const info = gl.getActiveAttrib(prog, i);
		attribs[info.name] = gl.getAttribLocation(prog, info.name);
	}

	const vertices = [
		// Position			// Normal
		// Front
		-1.0, -1.0, +1.0,	0.0, 0.0, +1.0,
		+1.0, -1.0, +1.0,	0.0, 0.0, +1.0,
		+1.0, +1.0, +1.0,	0.0, 0.0, +1.0,
		-1.0, +1.0, +1.0,	0.0, 0.0, +1.0,
		// Back
		-1.0, -1.0, -1.0,	0.0, 0.0, -1.0,
		-1.0, +1.0, -1.0,	0.0, 0.0, -1.0,
		+1.0, +1.0, -1.0,	0.0, 0.0, -1.0,
		+1.0, -1.0, -1.0,	0.0, 0.0, -1.0,
		// Top
		-1.0, +1.0, -1.0,	0.0, +1.0, 0.0,
		-1.0, +1.0, +1.0,	0.0, +1.0, 0.0,
		+1.0, +1.0, +1.0,	0.0, +1.0, 0.0,
		+1.0, +1.0, -1.0,	0.0, +1.0, 0.0,
		// Bottom
		-1.0, -1.0, -1.0,	0.0, -1.0, 0.0,
		+1.0, -1.0, -1.0,	0.0, -1.0, 0.0,
		+1.0, -1.0, +1.0,	0.0, -1.0, 0.0,
		-1.0, -1.0, +1.0,	0.0, -1.0, 0.0,
		// Right
		+1.0, -1.0, -1.0,	+1.0, 0.0, 0.0,
		+1.0, +1.0, -1.0,	+1.0, 0.0, 0.0,
		+1.0, +1.0, +1.0,	+1.0, 0.0, 0.0,
		+1.0, -1.0, +1.0,	+1.0, 0.0, 0.0,
		// Left
		-1.0, -1.0, -1.0,	-1.0, 0.0, 0.0,
		-1.0, -1.0, +1.0,	-1.0, 0.0, 0.0,
		-1.0, +1.0, +1.0,	-1.0, 0.0, 0.0,
		-1.0, +1.0, -1.0,	-1.0, 0.0, 0.0,
	];

	const indices = [
		0,  1,  2,  0,  2,  3,  // Front
		4,  5,  6,  4,  6,  7,  // Back
		8,  9,  10, 8,  10, 11, // Top
		12, 13, 14, 12, 14, 15, // Bottom
		16, 17, 18, 16, 18, 19, // Right
		20, 21, 22, 20, 22, 23, // Left
	]

	const vbo = gl.createBuffer();
	gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
	gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(vertices), gl.STATIC_DRAW);

	const vao = gl.createVertexArray();
	gl.bindVertexArray(vao)
	gl.enableVertexAttribArray(attribs["aPos"]);
	gl.vertexAttribPointer(attribs["aPos"], 3, gl.FLOAT, false, 6 * 4, 0);
	gl.enableVertexAttribArray(attribs["aNorm"]);
	gl.vertexAttribPointer(attribs["aNorm"], 3, gl.FLOAT, false, 6 * 4, 3 * 4);

	const ebo = gl.createBuffer();
	gl.bindBuffer(gl.ELEMENT_ARRAY_BUFFER, ebo);
	gl.bufferData(gl.ELEMENT_ARRAY_BUFFER, new Uint8Array(indices), gl.STATIC_DRAW);

	const camera = {
		pos: new Float32Array([0, 0.8, 2.5]),
		center: new Float32Array([0, 0, 0]),
		up: new Float32Array([0, 1, 0]),
	};

	const projMat = perspectiveMatrix(60, canvas.width / canvas.height, 0.1, 100);
	const viewMat = lookAtMatrix(camera.pos, camera.center, camera.up)
	const pvMat = multiplyMatrix(projMat, viewMat, identityMatrix(),);

	gl.enable(gl.DEPTH_TEST);
	gl.depthFunc(gl.LEQUAL);

	function update() {
		fetch(DATA_URL)
			.then(response => response.json())
			.then(data => requestAnimationFrame(() => { draw(data) }));
	}

	function draw(data) {
		gl.clearColor(0, 0, 0, 1);
		gl.clearDepth(1.0);
		gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

		gl.useProgram(prog);
		gl.bindVertexArray(vao);

		const modelMat = multiplyMatrix(rotationXMatrix(data.rot[0] * Math.PI / 180), rotationZMatrix(-data.rot[1] * Math.PI / 180))

		gl.uniform3fv(uniforms["uCameraPos"], camera.pos);
		gl.uniformMatrix4fv(uniforms["uPVMat"], false, pvMat);
		gl.uniformMatrix4fv(uniforms["uMMat"], false, modelMat);

		gl.drawElements(gl.TRIANGLES, indices.length, gl.UNSIGNED_BYTE, 0)

		//requestAnimationFrame(draw);
		update();
	}
	//requestAnimationFrame(draw);
	update();
}

// Setup graphs
function initGraphs() {
	// Buttons
	let modeOffset = 0;
	function clickRel() {
		modeOffset = 0;
		document.getElementById("btnRel").classList.add("active")
		document.getElementById("btnAbs").classList.remove("active")
	}

	function clickAbs() {
		modeOffset = 3;
		document.getElementById("btnAbs").classList.add("active")
		document.getElementById("btnRel").classList.remove("active")
	}

	clickRel();
	document.getElementById("btnRel").addEventListener("click", clickRel);
	document.getElementById("btnAbs").addEventListener("click", clickAbs);

	// Graphs
	const labels = [];
	for (let i = 0; i < 210; ++i) {
		let t = (200 - i) / 20;
		labels.push(-t);
	}
	const dataAcc = {
		labels: labels,
		datasets: [
			{
				label: 'x',
				backgroundColor: 'rgb(255, 99, 132)',
				borderColor: 'rgb(255, 99, 132)',
				data: new Array(201)
			}, {
				label: 'y',
				backgroundColor: 'rgb(99, 255, 117)',
				borderColor: 'rgb(99, 255, 117)',
				data: new Array(201)
			}, {
				label: 'z',
				backgroundColor: 'rgb(99, 141, 255)',
				borderColor: 'rgb(99, 141, 255)',
				data: new Array(201)
			}
		]
	};
	const dataDev = {
		labels: labels,
		datasets: [
			{
				label: 'std_dev(x)',
				backgroundColor: 'rgb(255, 99, 132)',
				borderColor: 'rgb(255, 99, 132)',
				data: new Array(201)
			}, {
				label: 'std_dev(y)',
				backgroundColor: 'rgb(99, 255, 117)',
				borderColor: 'rgb(99, 255, 117)',
				data: new Array(201)
			}, {
				label: 'std_dev(z)',
				backgroundColor: 'rgb(99, 141, 255)',
				borderColor: 'rgb(99, 141, 255)',
				data: new Array(201)
			}
		]
	};
	const configAcc = {
		type: 'line',
		data: dataAcc,
		options: {
			responsive: false,
			animation: { duration: 0 },
			scales: {
				y: {
					title: { text: "acceleration [m/s^2]", display: true },
					min: -15,
					max: +15
				},
				x: {
					title: { text: "time [s]", display: true },
					display: true,
					ticks: { maxTicksLimit: 11 }
				}
			},
			elements: {	point: { radius: 0 } }
		}
	};
	const configDev = {
		type: 'line',
		data: dataDev,
		options: {
			responsive: false,
			animation: { duration: 0 },
			scales: {
				y: {
					title: { text: "standard deviation", display: true },
					min: -1,
					max: +10
				},
				x: {
					title: { text: "time [s]", display: true	},
					display: true,
					ticks: { maxTicksLimit: 11 }
				}
			},
			elements: {	point: { radius: 0 } }
		}
	};
	var chartAcc = new Chart(
		document.getElementById('chart_acc'),
		configAcc
	);
	var chartDev = new Chart(
		document.getElementById('chart_dev'),
		configDev
	);

	function getHistory() {
		fetch(HISTORY_URL)
			.then(response => response.json())
			.then(json => requestAnimationFrame(() => { updateGraphs(json) }));
	}
	
	let last_step = 0;
	const activities = ["None", "Walking", "Jumping"];
	const spanActivity = document.getElementById("activity");

	function updateGraphs(json) {
		spanActivity.textContent = activities[json["activity"]];

		let len = json["acc"][0].length;
		let off = json["current_step"] - last_step;
		let pad = 0;
		if (off > len) {
			pad = Math.min(off - len, 201);
			off = len;
		}
		for (let i = 0; i < 3; ++i) {
			if (pad > 0) {
				dataAcc.datasets[i].data = dataAcc.datasets[i].data.slice(pad).concat(new Array(pad));
				dataDev.datasets[i].data = dataDev.datasets[i].data.slice(pad).concat(new Array(pad));
			}
			dataAcc.datasets[i].data = dataAcc.datasets[i].data.slice(off).concat(json["acc"][i+modeOffset].slice(-off));
			dataDev.datasets[i].data = dataDev.datasets[i].data.slice(off).concat(json["dev"][i+modeOffset].slice(-off));
		}
		last_step = json["current_step"];
		
		chartAcc.update()
		chartDev.update()
		setTimeout(getHistory, 100);
	}

	getHistory();
}


function main() {
	initWebGL();
	initGraphs();
}

document.addEventListener("DOMContentLoaded", main);
