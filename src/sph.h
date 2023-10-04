#pragma once

#include <iostream>
#include <vector>
#include <SimpleMath.h>
using namespace DirectX;

struct Particle {
	SimpleMath::Vector3 position;
	float density;
	float mass;
	float pressure;
	SimpleMath::Vector3 force;
	SimpleMath::Vector3 velocity;
	SimpleMath::Vector3 acceleration;
};

class SPH {
public:
	std::vector<Particle> m_particles;
	float m_h = 0.6;
	float m_nu = 1;
	float m_lambda = 0;
	int m_n = 0;

	// pressure constant
	float m_k = 0.1;
	float m_polytropicIndex = 1;
	float m_star_mass = 2;
	float m_star_radius = 1;

	
	SPH(int n) {
		m_particles.resize(n);
	};

	void Update(float dt);
	void UpdateDensity();
	void UpdateAcceleration();
	SimpleMath::Vector3 GradKernel(SimpleMath::Vector3 v, float smoothing, float distance);
	void Init();
	float Kernel(float distance, float smoothing);
};