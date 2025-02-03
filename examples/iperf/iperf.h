#include <stdint.h>
#include <stdbool.h>

// 타이머와 데이터 통계를 관리하는 구조체 정의
typedef struct {
    uint32_t pacing_timer_us;  // 타이머 주기 (마이크로초 단위)
    bool running;              // 실행 여부 플래그
    uint32_t t0;               // 테스트 시작 시간
    uint32_t t1;               // 마지막 업데이트 시간
    uint32_t t3;               // 테스트 종료 시간
    uint32_t nb0;              // 총 바이트 수
    uint32_t nb1;              // 주기별 바이트 수
    uint32_t np0;              // 총 패킷 수
    uint32_t np1;              // 주기별 패킷 수
} Stats;

void stats_init(Stats *stats, uint32_t pacing_timer_ms);
void stats_start(Stats *stats);
void stats_update(Stats *stats, bool final);
void stats_stop(Stats *stats);
void stats_add_bytes(Stats *stats, uint32_t n);

void handle_param_exchange(uint8_t socket_ctrl, bool *reverse, bool *udp);
void handle_create_streams(uint8_t socket_ctrl, bool udp);
void start_iperf_test(uint8_t socket_ctrl, uint8_t socket_data, Stats *stats, bool reverse, bool udp);
void exchange_results(uint8_t socket_ctrl, Stats *stats);