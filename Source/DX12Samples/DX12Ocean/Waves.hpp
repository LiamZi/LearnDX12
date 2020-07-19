#ifndef __WAVES_HPP__
#define __WAVES_HPP__

#include <vector>
#include <DirectXMath.h>


class Waves
{
private:
    /* data */
    int _numRows = 0;
    int _numCols = 0;
    int _vertexCount = 0;
    int _triangleCount = 0;

    float _k1 = 0.0f;
    float _k2 = 0.0f;
    float _k3 = 0.0f;

    float _timeStep = 0.0f;
    float _spatialStep = 0.0f;

    std::vector<DirectX::XMFLOAT3> _prevSolution;
    std::vector<DirectX::XMFLOAT3> _currSolution;
    std::vector<DirectX::XMFLOAT3> _normals;
    std::vector<DirectX::XMFLOAT3> _tangentX;

public:
    Waves(int m , int n, float dx, float dt, float speed, float damping);
    Waves(const Waves &rhs) = delete;
    Waves &operator=(const Waves &rhs) = delete;
    ~Waves();

    int rowCount() const;
    int columnCount() const;
    int vertexCount() const;
    int triangleCount() const;
    float width() const;
    float depth() const;

    const DirectX::XMFLOAT3 &position(const int index) const
    {
        return _currSolution[index];
    }

    const DirectX::XMFLOAT3 &normal(const int index) const
    {
        return _normals[index];
    }

    const DirectX::XMFLOAT3 &tangent(const int index) const
    {
        return _tangentX[index];
    }


    void update(float dt);
    void disturb(int i, int j, float magnitude);
};

#endif