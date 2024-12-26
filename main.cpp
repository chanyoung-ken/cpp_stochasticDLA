#include <iostream>              // 표준 입출력
#include <fstream>               // 파일 입출력
#include <random>               // 난수 생성
#include <vector>               // 벡터 사용
#include <string>               // 문자열
#include <filesystem>           // 디렉토리 생성 등
#include "../include/simulation.hpp" // simulation.hpp (random_walk_3d 등 함수)
#include <zlib.h>               // gzip 압축 라이브러리

/* 
 compile commands
 g++ -std=c++17 -Iinclude src/main.cpp src/simulation.cpp -o simulation -lz
 */

////////////////////////////////////////////////////////////////////////////
// gzip 압축 파일 쓰기 함수
////////////////////////////////////////////////////////////////////////////
bool write_gzip_file(const std::string& filename, const std::string& data) {
    // "wb" 모드로 gzip 파일 열기 (쓰기 모드, 바이너리)
    gzFile gz = gzopen(filename.c_str(), "wb");
    if (!gz) return false;  // 파일 열기 실패 시 false 반환

    // gzip 스트림에 data 내용을 쓰기
    gzwrite(gz, data.data(), static_cast<unsigned int>(data.size()));
    gzclose(gz);            // 파일 닫기
    return true;            // 성공 시 true
}

////////////////////////////////////////////////////////////////////////////
// 디렉토리 생성 함수
////////////////////////////////////////////////////////////////////////////
void create_output_directories(const std::string& main_directory) {
    // C++17 <filesystem>를 사용해 main_directory + "/data" 하위 디렉토리 생성
    std::filesystem::create_directories(main_directory + "/data");
}

////////////////////////////////////////////////////////////////////////////
// CSV 데이터를 gzip으로 저장하는 함수
////////////////////////////////////////////////////////////////////////////
void save_simulation_data_gz(
    const std::vector<int>& grid, 
    int L, 
    int total_height, 
    int step, 
    const std::string& directory, 
    double p_initial
) {
    // 파일명: "particles_step_p_<p_initial>_step_<step>.csv.gz" 형태
    std::string filename = directory + "/data/particles_step_p_" 
                          + std::to_string(p_initial) 
                          + "_step_" + std::to_string(step) + ".csv.gz";

    // CSV 형태의 데이터를 문자열로 만든다
    // 첫 행: "x,y,z,step"
    std::string csv_data = "x,y,z,step\n";

    // 격자(grid)를 순회하며, val>0 인(입자가 있는) 칸만 CSV 문자열에 추가
    for (int x = 0; x < L; x++) {
        for (int y = 0; y < L; y++) {
            for (int z = 0; z < total_height; z++) {
                int val = grid[x * L * total_height + y * total_height + z];
                // val>0 이면 입자가 있는 셀이므로 기록
                if (val > 0) {
                    csv_data += std::to_string(x) + "," 
                              + std::to_string(y) + ","
                              + std::to_string(z) + ","
                              + std::to_string(val) + "\n";
                }
            }
        }
    }

    // gzip으로 압축 저장
    if (!write_gzip_file(filename, csv_data)) {
        std::cerr << "Failed to write gzip file: " << filename << std::endl;
    }
}

////////////////////////////////////////////////////////////////////////////
// main 함수
////////////////////////////////////////////////////////////////////////////
int main() {
    int L = 100;              // 격자 x, y 크기 (L x L)
    int total_height = 600;   // 격자의 높이 (z 최대값)
    int H = 400;              // 초기 입자 출발 높이의 최소값
    double dt = 1.0;          // 시간 증가량(한 스텝마다) 현재는 사용하지 않음
    int delta_H = 200;        // 초기 입자 출발 높이 범위 (H ~ H+delta_H)

    // 여러 p 값을 반복하여 시뮬레이션, 필요시 리스트의 p값 추가, 삭제
    // p_initial은 이웃 입자가 있을 때 증착될 확률
    std::vector<double> p_initial_values = {0.1, 0.2, 0.4, 0.6, 0.8, 1.0};

    // 결과를 저장할 상위 디렉토리
    // 각 p_initial별로 서브 디렉토리를 생성함
    // 12/26일에 생성한 첫번째 데이터셋이라는 의미
    std::string base_directory = "/Users/noyes2/Desktop/chanyeong/sim6/1226_1dataset";

    // 모든 p_initial에 대해 시뮬레이션
    for (auto p_initial : p_initial_values) {
        // p값 폴더: "p__<p_initial>" 이름으로 생성
        //과거 데이터와 혼동막기위해 _을 두번사용:p__<p_initial>
        std::string main_directory = base_directory + "/p__" + std::to_string(p_initial);
        create_output_directories(main_directory);

        // 3차원 격자를 1D 배열로 표현 (크기: L * L * total_height)
        // 모든 칸을 0(비어있음)으로 초기화
        std::vector<int> grid(L * L * total_height, 0);

        // 난수 생성기 및 분포 설정
        std::random_device rd; //하드웨어 기반 난수생성
        std::mt19937 gen(rd());                 // Mersenne Twister 엔진 사용
        std::uniform_real_distribution<double> dist(0.0, 1.0); // [0,1) 난수
        // x, y 랜덤 위치 (0~L-1)
        std::uniform_int_distribution<int> x_dist(0, L-1);
        std::uniform_int_distribution<int> y_dist(0, L-1);
        // z(높이)는 [H, H+delta_H] 범위에서 랜덤
        std::uniform_int_distribution<int> h_dist(H, H + delta_H);

        double current_time = 0.0;  // 시뮬레이션 시간 누적
        int deposited_count = 0;    // 증착된 입자 수 (누적)
        int launched_count = 0;     // 발사된(던져진) 입자 수 (누적)
        int step = 0;              // 전체 스텝(반복 회수)

        // 시뮬레이션 루프: 증착된 입자가 5만 개 도달할 때까지
        while (deposited_count < 50000) {
            step++;             // 스텝 증가
            launched_count++;   // 입자 1개 발사 시도

            // 무작위로 (x, y, z) 초기 위치 설정
            int x = x_dist(gen);
            int y = y_dist(gen);
            int z = h_dist(gen);

            // random_walk_3d 함수(랜덤 워크 + 증착 시도)
            // escaped == true면 격자 밖으로 빠져나간 것
            // escaped == false면 증착된 것
            bool escaped = random_walk_3d(
                grid, 
                L, 
                total_height, 
                p_initial,
                x, 
                y, 
                z, 
                step, 
                current_time, 
                dt,
                gen, 
                dist
            );

            // 증착 성공 시 deposited_count 증가
            if (!escaped) {
                deposited_count++;
            }

            // 진행 상황 모니터링: 만 입자마다 메시지 출력
            if (deposited_count % 10000 == 0 && deposited_count > 0) {
                std::cout << "p = " << p_initial 
                          << ", 증착된 입자 수: " << deposited_count 
                          << ", 발사된 입자 수: " << launched_count << std::endl;
            }
        }

        // 증착 5만 개 달성 후, 최종 상태를 gzip CSV로 저장
        save_simulation_data_gz(grid, L, total_height, step, main_directory, p_initial);

        // 최종 요약 출력
        std::cout << "\n시뮬레이션 완료 (p = " << p_initial << "):" << std::endl;
        std::cout << "총 발사된 입자: " << launched_count << std::endl;
        std::cout << "총 증착된 입자: " << deposited_count << std::endl;
        std::cout << "최종 스텝: " << step << std::endl;
    }

    return 0; // 프로그램 종료
}
