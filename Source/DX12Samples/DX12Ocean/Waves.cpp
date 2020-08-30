#include "Waves.hpp"
#include <ppl.h>
#include <algorithm>
#include <cassert>

using namespace DirectX;


Waves::Waves(int m, int n, float dx, float dt, float speed, float damping)
{
	_numRows = m;
    _numCols = n;

    _vertexCount = m*n;
    _triangleCount = (m - 1)*(n - 1) * 2;

    _timeStep = dt;
    _spatialStep = dx;

    float d = damping*dt + 2.0f;
    float e = (speed*speed)*(dt*dt) / (dx*dx);
    _k1 = (damping*dt - 2.0f) / d;
    _k2 = (4.0f - 8.0f*e) / d;
    _k3 = (2.0f*e) / d;

    _prevSolution.resize(m*n);
    _currSolution.resize(m*n);
    _normals.resize(m*n);
    _tangentX.resize(m*n);

    // Generate grid vertices in system memory.

    float halfWidth = (n - 1)*dx*0.5f;
    float halfDepth = (m - 1)*dx*0.5f;
    for(int i = 0; i < m; ++i)
    {
        float z = halfDepth - i*dx;
        for(int j = 0; j < n; ++j)
        {
            float x = -halfWidth + j*dx;

            _prevSolution[i*n + j] = XMFLOAT3(x, 0.0f, z);
            _currSolution[i*n + j] = XMFLOAT3(x, 0.0f, z);
            _normals[i*n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
            _tangentX[i*n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
        }
    }
    // _numRows = m;
    // _numCols = n;

    // _vertexCount = m * n;
    // _triangleCount = (m - 1) * (n - 1) * 2;

    // _timeStep = dt;
    // _spatialStep = dx;

    // float d = damping * dt + 2.0f;
    // float e  = (speed * speed) * (dt * dt) / (dx * dx);
    // _k1 = (damping * dt - 2.0f) /d;
    // _k2 = (4.0f - 8.0f * e) / d;
    // _k3 = (2.0f * e) / d;

    // _prevSolution.resize(_vertexCount);
    // _currSolution.resize(_vertexCount);
    // _normals.resize(_vertexCount);
    // _tangentX.resize(_vertexCount);

    // float halfWidth = (n - 1) * dx * 0.5f;
    // float halfDepth = (m - 1) * dx * 0.5f;
    // for (size_t i = 0; i < m; i++)
    // {
    //     float z = halfDepth - i * dx;
    //     for (size_t j = 0; j < n; j++)
    //     {
    //         float x = -halfWidth + j * dx;
    //         _prevSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
    //         _currSolution[i * n + j] = XMFLOAT3(x, 0.0f, z);
    //         _normals[i * n + j]  = XMFLOAT3(0.0f, 1.0f, 0.0f);
    //         _tangentX[i * n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);

    //     }
        
    // }
}

Waves::~Waves()
{

}


int Waves::rowCount() const 
{
    return _numRows;
}

int Waves::columnCount() const 
{
    return _numCols;
}

int Waves::vertexCount() const
{
    return _vertexCount;
}

int Waves::triangleCount() const
{
    return _triangleCount;
}

float Waves::width() const
{
    return _numCols * _spatialStep;
}

float Waves::depth() const
{
    return _numRows * _spatialStep;
}

void Waves::update(float dt)
{
    // static float t = 0;

    // t += dt;

    // if(t >= _timeStep)
    // {
    //     concurrency::parallel_for(1, _numRows - 1, [this](int i)
    //     {
    //        for (size_t j = 1; j < _numCols - 1; j++)
    //        {
    //             _prevSolution[i * _numCols + j].y = 
    //                 _k1 * _prevSolution[i * _numCols + j].y +
    //                 _k2 * _prevSolution[i * _numCols + j].y +
    //                 _k3 * (_currSolution[( i + 1 ) * _numCols + j].y + 
	// 				     _currSolution[(i - 1) * _numCols + j].y + 
	// 				     _currSolution[i * _numCols + j + 1].y + 
	// 					 _currSolution[i * _numCols + j - 1].y);

    //        }
           
    //     });

    //     std::swap(_prevSolution, _currSolution);
    //     t = 0.0f;

    //     concurrency::parallel_for(1, _numRows - 1, [this](int i)
    //     {
    //         for (size_t j = 1; j < _numCols - 1; j++)
    //         {
    //             /* code */
    //             float l = _currSolution[i * _numCols + j - 1].y;
	// 			float r = _currSolution[i * _numCols + j + 1].y;
	// 			float t = _currSolution[(i - 1) * _numCols + j].y;
	// 			float b = _currSolution[(i + 1) * _numCols + j].y;
	// 			_normals[i * _numCols + j].x = -r + l;
	// 			_normals[i * _numCols + j].y = 2.0f * _spatialStep;
	// 			_normals[i * _numCols + j].z = b - t;

	// 			XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&_normals[i * _numCols + j]));
	// 			XMStoreFloat3(&_normals[i * _numCols + j], n);

	// 			_tangentX[i * _numCols + j] = XMFLOAT3(2.0f * _spatialStep, r - l, 0.0f);
	// 			XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&_tangentX[i * _numCols + j]));
	// 			XMStoreFloat3(&_tangentX[i * _numCols+j], T);
    //         }
            
    //     });
    // }

    static float t = 0;

	// Accumulate time.
	t += dt;

	// Only update the simulation at the specified time step.
	if( t >= _timeStep )
	{
		// Only update interior points; we use zero boundary conditions.
		concurrency::parallel_for(1, _numRows - 1, [this](int i)
		//for(int i = 1; i < _numRows-1; ++i)
		{
			for(int j = 1; j < _numCols-1; ++j)
			{
				// After this update we will be discarding the old previous
				// buffer, so overwrite that buffer with the new update.
				// Note how we can do this inplace (read/write to same element) 
				// because we won't need prev_ij again and the assignment happens last.

				// Note j indexes x and i indexes z: h(x_j, z_i, t_k)
				// Moreover, our +z axis goes "down"; this is just to 
				// keep consistent with our row indices going down.

				_prevSolution[i*_numCols+j].y = 
					_k1*_prevSolution[i*_numCols+j].y +
					_k2*_currSolution[i*_numCols+j].y +
					_k3*(_currSolution[(i+1)*_numCols+j].y + 
					     _currSolution[(i-1)*_numCols+j].y + 
					     _currSolution[i*_numCols+j+1].y + 
						 _currSolution[i*_numCols+j-1].y);
			}
		});

		// We just overwrote the previous buffer with the new data, so
		// this data needs to become the current solution and the old
		// current solution becomes the new previous solution.
		std::swap(_prevSolution, _currSolution);

		t = 0.0f; // reset time

		//
		// Compute normals using finite difference scheme.
		//
		concurrency::parallel_for(1, _numRows - 1, [this](int i)
		//for(int i = 1; i < _numRows - 1; ++i)
		{
			for(int j = 1; j < _numCols-1; ++j)
			{
				float l = _currSolution[i*_numCols+j-1].y;
				float r = _currSolution[i*_numCols+j+1].y;
				float t = _currSolution[(i-1)*_numCols+j].y;
				float b = _currSolution[(i+1)*_numCols+j].y;
				_normals[i*_numCols+j].x = -r+l;
				_normals[i*_numCols+j].y = 2.0f*_spatialStep;
				_normals[i*_numCols+j].z = b-t;

				XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&_normals[i*_numCols+j]));
				XMStoreFloat3(&_normals[i*_numCols+j], n);

				_tangentX[i*_numCols+j] = XMFLOAT3(2.0f*_spatialStep, r-l, 0.0f);
				XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&_tangentX[i*_numCols+j]));
				XMStoreFloat3(&_tangentX[i*_numCols+j], T);
			}
		});
	}
}

void Waves::disturb(int i, int j, float magnitude)
{
	// Don't disturb boundaries.
	assert(i > 1 && i < _numRows - 2);
	assert(j > 1 && j < _numCols - 2);

	float halfMag = 0.5f*magnitude;

	// Disturb the ijth vertex height and its neighbors.
	_currSolution[i * _numCols + j].y       += magnitude;
	_currSolution[i * _numCols + j + 1].y   += halfMag;
	_currSolution[i * _numCols + j - 1].y   += halfMag;
	_currSolution[(i + 1) * _numCols + j].y += halfMag;
	_currSolution[(i - 1) * _numCols + j].y += halfMag;


    // assert(i > 1 && i < _numRows-2);
	// assert(j > 1 && j < _numCols-2);

	// float halfMag = 0.5f*magnitude;

	// // Disturb the ijth vertex height and its neighbors.
	// _currSolution[i*_numCols+j].y     += magnitude;
	// _currSolution[i*_numCols+j+1].y   += halfMag;
	// _currSolution[i*_numCols+j-1].y   += halfMag;
	// _currSolution[(i+1)*_numCols+j].y += halfMag;
	// _currSolution[(i-1)*_numCols+j].y += halfMag;
}