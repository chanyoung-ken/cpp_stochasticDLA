#include "simulation.hpp"
#include <cmath>
#include <random>
#include <vector>

// 이웃 파티클 확인
bool check_neighbor_particles(
    const std::vector<int>& grid,
    int x, int y, int z,
    int L, int total_height
) {
    int neighbors[6][3] = {
        {0,0,1},{0,0,-1},
        {0,1,0},{0,-1,0},
        {1,0,0},{-1,0,0}
    };

    for (auto& n : neighbors) {
        int nx = (x + n[0] + L) % L;
        int ny = (y + n[1] + L) % L;
        int nz = z + n[2];
        if (nz >= 0 && nz < total_height) {
            if (grid[nx * L * total_height + ny * total_height + nz] > 0) {
                return true;
            }
        }
    }
    return false;
}


// -------------------------------------------------------
//  "이미 점유된 셀은 이동 불가"를 적용한 random_walk_3d
// -------------------------------------------------------
bool random_walk_3d(
    std::vector<int>& grid, 
    int L, int total_height, 
    double p_initial,
    int x, int y, int z,
    int step, double& current_time, double dt, 
    std::mt19937& gen, 
    std::uniform_real_distribution<double>& dist
) {
    // (1) 초기 증착 시도
    current_time += dt;
    if (z < 0 || z >= total_height) {
        // 범위 밖 => 탈출
        return true; 
    } else {
        // 이웃에 입자가 있으면 확률적으로 증착
        if (check_neighbor_particles(grid, x, y, z, L, total_height)) {
            double rnd = dist(gen);
            if (rnd < p_initial) {
                set_grid(grid, x, y, z, step, L, total_height);
                return false;
            }
        }
    }

    // 이동 방향 후보 (6가지)
    int moves[6][3] = {
        {0,0,1},{0,0,-1},
        {0,1,0},{0,-1,0},
        {1,0,0},{-1,0,0}
    };

    // (2) 무한 루프 -> 무작위 이동 반복
    while (true) {
        current_time += dt;

        // 이동할 방향 하나 선택
        int move_idx = std::uniform_int_distribution<int>(0,5)(gen);

        // 다음 좌표 계산
        int nx = (x + moves[move_idx][0] + L) % L;
        int ny = (y + moves[move_idx][1] + L) % L;
        int nz = z + moves[move_idx][2];

        // (2-a) 범위 밖 => 탈출
        if (nz < 0 || nz >= total_height) {
            return true;
        }

        // (2-b) "이미 점유된 셀인지" 검사 => 만약 점유돼 있으면 '벽'으로 간주하고 이동 불가
        int pos_new = nx * L * total_height + ny * total_height + nz;
        if (grid[pos_new] > 0) {
            // 이미 다른 입자가 있는 셀 -> 이동 스킵
            // 이 위치로는 이동하지 않고, while 루프 처음으로 돌아가서 '다른 방향' 시도
            //혹시나 중복증착을 방지하기위함
            continue;
        }

        // (2-c) 바닥 검사 -> z=0이면 바로 증착
        if (nz == 0) {
            set_grid(grid, nx, ny, nz, step, L, total_height);
            return false; 
        }

        // (2-d) 위 조건에 걸리지 않았다면, 이동 확정
        x = nx;
        y = ny;
        z = nz;

        // (2-e) 이동한 위치에서 이웃체크 -> 확률증착
        if (check_neighbor_particles(grid, x, y, z, L, total_height)) {
            double rnd = dist(gen);
            if (rnd < p_initial) {
                set_grid(grid, x, y, z, step, L, total_height);
                return false;
            }
        }
        // 이후 while(true) 계속 -> 다음 시간 스텝에서 다시 이동 시도
    }

    return true; 
}
