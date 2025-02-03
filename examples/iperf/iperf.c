#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "iperf.h"


// 현재 시간을 마이크로초 단위로 반환
uint32_t get_time_us() {
    return (uint32_t)time_us_64();
}

// Stats 구조체 초기화 함수
void stats_init(Stats *stats, uint32_t pacing_timer_ms) {
    stats->pacing_timer_us = pacing_timer_ms * 1000;
    stats->running = false;
    stats->t0 = 0;
    stats->t1 = 0;
    stats->t3 = 0;
    stats->nb0 = 0;
    stats->nb1 = 0;
    stats->np0 = 0;
    stats->np1 = 0;
}

// Stats 시작 함수
void stats_start(Stats *stats) {
    stats->running = true;
    stats->t0 = stats->t1 = get_time_us();
    stats->nb0 = stats->nb1 = 0;
    stats->np0 = stats->np1 = 0;
    printf("Interval           Transfer     Bitrate\n");
}

// Stats 업데이트 함수
void stats_update(Stats *stats, bool final) {
    if (!stats->running) return;

    uint32_t t2 = get_time_us();
    uint32_t dt = t2 - stats->t1;  // 마지막 업데이트 이후 경과 시간

    if (final || dt > stats->pacing_timer_us) {
        double ta = (stats->t1 - stats->t0) / 1e6;  // 이전 시간 간격 시작
        double tb = (t2 - stats->t0) / 1e6;         // 현재 시간 간격 종료
        double transfer_mbits = (stats->nb1 * 8) / 1e6 / (dt / 1e6);  // Mbps 계산

        printf("%5.2f-%-5.2f sec %8u Bytes  %5.2f Mbits/sec\n",
               ta, tb, stats->nb1, transfer_mbits);

        stats->t1 = t2;  // 타이머 갱신
        stats->nb1 = 0;  // 주기별 데이터 초기화
        stats->np1 = 0;

    }
}

// Stats 중지 함수
void stats_stop(Stats *stats) {
    if (!stats->running) return;

    stats_update(stats, true);  // 마지막 업데이트
    stats->running = false;

    stats->t3 = get_time_us();  // 종료 시간 기록
    uint32_t total_time_us = stats->t3 - stats->t0;
    double total_time_s = total_time_us / 1e6;
    double transfer_mbits = (stats->nb0 * 8) / 1e6 / total_time_s;

    printf("------------------------------------------------------------\n");
    printf("Total: %5.2f sec %8u Bytes  %5.2f Mbits/sec\n",
           total_time_s, stats->nb0, transfer_mbits);
}

// Stats 데이터 추가 함수
void stats_add_bytes(Stats *stats, uint32_t n) {
    if (!stats->running) return;

    stats->nb0 += n;  // 총 바이트 수 증가
    stats->nb1 += n;  // 주기별 바이트 수 증가
    stats->np0 += 1;  // 총 패킷 수 증가
    stats->np1 += 1;  // 주기별 패킷 수 증가

}