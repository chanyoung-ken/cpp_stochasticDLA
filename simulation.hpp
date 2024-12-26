#ifndef SIMULATION_HPP
#define SIMULATION_HPP
//simulation헤더파일 .cpp파일에서 사용할 함수들 선언
#include <vector>
#include <random>

// 이웃 파티클 확인
bool check_neighbor_particles(
    const std::vector<int>& grid,
    int x, int y, int z,
    int L, int total_height
);

// 그리드에 값 설정 헬퍼
inline void set_grid(
    std::vector<int>& grid, 
    int x, int y, int z, 
    int value, int L, int total_height
) {
    grid[x * L * total_height + y * total_height + z] = value;
}

// 랜덤 워크 및 증착 로직
bool random_walk_3d(
    std::vector<int>& grid, 
    int L, int total_height, 
    double p_initial,
    int start_x, int start_y, int start_z,
    int step, double& current_time, double dt, 
    std::mt19937& gen, std::uniform_real_distribution<double>& dist
);

#endif
