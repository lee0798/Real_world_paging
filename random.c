//
// Created by 이도환 on 6/12/24.
//
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// 제약조건
// 1. input 으로 주어지는 파일의 한줄은 100자를 넘지 않음.
// 2. input 으로 주어지는 파일의 길이는 100000줄을 넘지 않음. 즉, address trace 길이는 100000줄을 넘지 않음.
#define MAX_ROW     100000
#define MAX_COL     2
#define MAX_INPUT   100

// 캐시 블록 구조체 정의
typedef struct {
    int valid;      // 유효 비트 (1: 유효, 0: 무효)
    int tag;        // 태그 비트
    int dirty;      // 더티 비트 (1: 변경됨, 0: 변경되지 않음)
} CacheBlock;

// 함수 선언
void solution(int cache_size, int block_size, int assoc);
void read_op(int addr, int cache_size, int block_size, int assoc);
void write_op(int addr, int cache_size, int block_size, int assoc);
void fetch_inst(int addr, int cache_size, int block_size, int assoc);
int find_random_block(CacheBlock* cache, int assoc);

// 전역 변수
enum COLS {
    MODE,
    ADDR
};

int total_set;
int i_total, i_miss;            // instruction cache 접근 횟수 및 miss 횟수
int d_total, d_miss, d_write;   // data cache 접근 횟수 및 miss 횟수, memory write 횟수
int trace[MAX_ROW][MAX_COL] = {{0,0},};
int trace_length = 0;

CacheBlock** instruction_cache;  // instruction cache 배열
CacheBlock** data_cache;         // data cache 배열

int main(){
    srand(42);
    // DO NOT MODIFY -- START --  //
    // 캐시 크기, 블록 크기, 연관도를 정의
    int cache[5] = {1024, 2048, 4096, 8192, 16384};
    int block[2] = {16, 64};
    int associative[4] = {1, 2, 4, 8};
    int i=0,j=0,k=0;

    // 입력 받아오기
    char input[MAX_INPUT];
    while (fgets(input, sizeof(input), stdin)) {
        if(sscanf(input, "%d %x\n", &trace[trace_length][MODE], &trace[trace_length][ADDR]) != 2) {
            fprintf(stderr, "error!\n");
        }
        trace_length++;
    }

    // 캐시 시뮬레이션
    printf("cache size || block size || associative || d-miss rate || i-miss rate || mem write\n");
    for(i=0; i<5; i++){
        for(j=0; j<2; j++){
            for(k=0; k<4; k++){
                solution(cache[i], block[j], associative[k]);
            }
        }
    }
    // DO NOT MODIFY -- END --  //
    return 0;
}

void solution(int cache_size, int block_size, int assoc) {
    // 전역 변수 값 초기화
    i_total = i_miss = 0;
    d_total = d_miss = d_write = 0;

    total_set = cache_size / (block_size * assoc);

    instruction_cache = calloc(total_set, sizeof(CacheBlock*));
    data_cache = calloc(total_set, sizeof(CacheBlock*));

    for (int i = 0; i < total_set; i++) {
        instruction_cache[i] = calloc(assoc, sizeof(CacheBlock));
        data_cache[i] = calloc(assoc, sizeof(CacheBlock));
    }

    // DO NOT MODIFY -- START --  //
    int mode, addr;
    double i_miss_rate, d_miss_rate;

    int index = 0;
    while(index != trace_length) {
        mode = trace[index][MODE];
        addr = trace[index][ADDR];

        switch(mode) {
            case 0 :
                read_op(addr, cache_size, block_size, assoc);
                d_total++;
                break;
            case 1 :
                write_op(addr, cache_size, block_size, assoc);
                d_total++;
                break;
            case 2 :
                fetch_inst(addr, cache_size, block_size, assoc);
                i_total++;
                break;
        }
        index++;
    }
    // DO NOT MODIFY -- END --  //

    // miss rate 계산
    i_miss_rate = (double)i_miss / (double)i_total;
    d_miss_rate = (double)d_miss / (double)d_total;

    // 결과 출력
    printf("%8d\t%8d\t%8d\t%.4lf\t%.4lf\t%8d\n", cache_size, block_size, assoc, d_miss_rate, i_miss_rate, d_write);

    // 메모리 해제
    for (int i = 0; i < total_set; i++) {
        free(instruction_cache[i]);
        free(data_cache[i]);
    }
    free(instruction_cache);
    free(data_cache);
}

void read_op(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    for (int i = 0; i < assoc; i++) {
        if (data_cache[index][i].valid && data_cache[index][i].tag == tag) {
            return;
        }
    }

    d_miss++;

    int random_block = find_random_block(data_cache[index], assoc);
    if (data_cache[index][random_block].dirty)
        d_write++;

    // 새로운 블록으로 갱신
    data_cache[index][random_block].valid = 1;
    data_cache[index][random_block].tag = tag;
    data_cache[index][random_block].dirty = 0;

    return;
}

void write_op(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    for (int i = 0; i < assoc; i++) {
        if (data_cache[index][i].valid && data_cache[index][i].tag == tag) {
            data_cache[index][i].dirty = 1;
            return;
        }
    }

    d_miss++;

    int random_block = find_random_block(data_cache[index], assoc);
    if (data_cache[index][random_block].dirty)
        d_write++;

    // 새로운 블록으로 갱신
    data_cache[index][random_block].valid = 1;
    data_cache[index][random_block].tag = tag;
    data_cache[index][random_block].dirty = 1;

    return;
}

void fetch_inst(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    for (int i = 0; i < assoc; i++) {
        if (instruction_cache[index][i].valid && instruction_cache[index][i].tag == tag) {
            return;
        }
    }
    i_miss++;

    int random_block = find_random_block(instruction_cache[index], assoc);
    instruction_cache[index][random_block].valid = 1;
    instruction_cache[index][random_block].tag = tag;
    instruction_cache[index][random_block].dirty = 0;

    return;
}

int find_random_block(CacheBlock* cache, int assoc) {
    return rand() % assoc;
}
