#include <stdio.h>
#include <stdlib.h>

// 제약조건
// 1. input 으로 주어지는 파일의 한줄은 100자를 넘지 않음.
// 2. input 으로 주어지는 파일의 길이는 100000줄을 넘지 않음. 즉, address trace 길이는 100000줄을 넘지 않음.
#define MAX_ROW     1000000
#define MAX_COL     2
#define MAX_INPUT   100

// LRU 알고리즘 자료구조
typedef struct {
    int valid;      // 유효 비트 (1: 유효, 0: 무효)
    int tag;        // 태그 비트
    int timestamp;  // 타임스탬프
    int dirty;      // (1: 일치하지 않음, 0: 일치함)
} CacheBlock;

// 구현해야하는 함수
void solution(int cache_size, int block_size, int assoc);
void read_op(int addr, int cache_size, int block_size, int assoc);
void write_op(int addr, int cache_size, int block_size, int assoc);
void fetch_inst(int addr, int cache_size, int block_size, int assoc);
//====================dohwan====================================
int find_lru_block(CacheBlock* cache, int assoc);
void update_timestamp(CacheBlock* cache, int assoc, int used_block);
//=============================================================
// 문제를 풀기 위한 힌트로써 제공된 것이며, 마음대로 변환 가능합니다.
enum COLS {
    MODE,
    ADDR
};
//====================================dohwan =======================================


int time_count; /* LRU를 구현하기 위한 시간 */
int total_set;
//===================================================================================
int i_total, i_miss;            /* instructino cache 총 접근 횟수, miss 횟수*/
int d_total, d_miss, d_write;   /* data cache 접근 횟수 및 miss 횟수, memory write 횟수 */
int trace[MAX_ROW][MAX_COL] = {{0,0},};
int trace_length = 0;
//==================================dohwan=====================================
CacheBlock** instruction_cache;  // instruction cache 배열
CacheBlock** data_cache;         // data cache 배열
//=========================================================================
int main(){
    // DO NOT MODIFY -- START --  //
    // cache size
    int cache[6] = {1024, 2048, 4096, 8192, 16384,1024*1024};
    // block size
    int block[2] = {16, 64};
    // associatvity e.g., 1-way, 2-way, ... , 8-way
    int associative[5] = {1, 2, 4, 8,/*fully associaitive 구현*/ 0};
    int i=0,j=0,k=0;


    /* 입력 받아오기 */
    char input[MAX_INPUT];
    while (fgets(input, sizeof(input), stdin)) {
        if(sscanf(input, "%d %x\n", &trace[trace_length][MODE], &trace[trace_length][ADDR]) != 2) {
            fprintf(stderr, "error!\n");
        }
        trace_length++;
    }


    /* 캐시 시뮬레이션 */
    printf("cache size || block size || associative || d-miss rate || i-miss rate || mem write\n");
    for(i=0; i<6; i++){
        for(j=0; j<2; j++){
            for(k=0; k<5; k++){
                int current_associativity = associative[k];
                if (current_associativity == 0) {
                    // Fully associative의 경우 캐시 크기 / 블록 크기 계산
                    current_associativity = cache[i] / block[j];
                }
                solution(cache[i], block[j], current_associativity);
            }
        }
    }
    // DO NOT MODIFY -- END --  //
    return 0;
}

void solution(int cache_size, int block_size, int assoc) {
    //=======================dohwan=========================
    /* 전역변수 값 초기화 */
    //IDS 각 블락 count 수 max 값은 4까지
    i_total=i_miss=0;
    d_total=d_miss=d_write=0;

    total_set = cache_size / (block_size * assoc);


    instruction_cache = calloc(total_set, sizeof(CacheBlock*));;
    data_cache = calloc(total_set, sizeof(CacheBlock*));;

    for (int i = 0; i < total_set; i++) {
        instruction_cache[i] = calloc(assoc, sizeof(CacheBlock));
        data_cache[i] = calloc(assoc, sizeof(CacheBlock));
    }
    //========================================================


	// DO NOT MODIFY -- START --  //
    int mode, addr;
    double i_miss_rate, d_miss_rate;    /* miss rate을 저장하는 변수 */

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

    // hint. data cache miss rate 와 intruction cache miss rate를 계산하시오.
    // ? 에는 알맞는 변수를 넣으면 됩니다.
    i_miss_rate = (double)i_miss / (double)i_total;
    d_miss_rate = (double)d_miss / (double)d_total;

	// DO NOT MODIFY -- START --  //
    printf("%8d\t%8d\t%8d\t%.4lf\t%.4lf\t%8d\n", cache_size, block_size, assoc, d_miss_rate, i_miss_rate, d_write);
	// DO NOT MODIFY -- END --  //
    //=============dohwan=======================
    for (int i = 0; i < total_set; i++) {
        free(instruction_cache[i]);
        free(data_cache[i]);
    }
    free(instruction_cache);
    free(data_cache);
    //==================================
}

// 아래 함수를 직접 구현하시오, 차례로 읽기, 쓰기, 그리고 인스트럭션 fetch 동작입니다.
void read_op(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    int i;
    for (i = 0; i < assoc; i++) {
        if (data_cache[index][i].valid && data_cache[index][i].tag == tag) {
            update_timestamp(data_cache[index], assoc, i);
            return;
        }
    }

    d_miss++;

    int lru_block = find_lru_block(data_cache[index], assoc);
    if (data_cache[index][lru_block].dirty)
        d_write++;

    // Update cache with new block
    data_cache[index][lru_block].valid = 1;
    data_cache[index][lru_block].tag = tag;
    data_cache[index][lru_block].dirty = 0;

    update_timestamp(data_cache[index], assoc, lru_block);
    return;
}


void write_op(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    int i;
    for (i = 0; i < assoc; i++) {
        if (data_cache[index][i].valid && data_cache[index][i].tag == tag) {
            update_timestamp(data_cache[index], assoc, i);
            data_cache[index][i].dirty  = 1;
            return;
        }
    }

    d_miss++;

    int lru_block = find_lru_block(data_cache[index], assoc);
    if (data_cache[index][lru_block].dirty)
        d_write++;

    data_cache[index][lru_block].valid = 1;
    data_cache[index][lru_block].tag = tag;
    data_cache[index][lru_block].dirty = 1;

    update_timestamp(data_cache[index], assoc, lru_block);
    return;
}

void fetch_inst(int addr, int cache_size, int block_size, int assoc){
    int index = (addr / block_size) % total_set;
    int tag = (addr / block_size) * total_set;

    int i;
    for (i = 0; i < assoc; i++) {
        if (instruction_cache[index][i].valid && instruction_cache[index][i].tag == tag) {
            // Cache hit
            update_timestamp(instruction_cache[index], assoc, i);
            return;
        }
    }
    i_miss++;

    int lru_block = find_lru_block(instruction_cache[index], assoc);
    instruction_cache[index][lru_block].valid = 1;
    instruction_cache[index][lru_block].tag = tag;
    instruction_cache[index][lru_block].dirty = 0;

    update_timestamp(instruction_cache[index], assoc, lru_block);
    return;
}

int find_lru_block(CacheBlock* cache, int assoc) {
    int lru_block = 0;
    int max_timestamp = cache[0].timestamp;
    int i;
    for (i = 1; i < assoc; i++) {
        if (cache[i].timestamp > max_timestamp) {
            lru_block = i;
            max_timestamp = cache[i].timestamp;
        }
    }
    return lru_block;
}

void update_timestamp(CacheBlock* cache, int assoc, int used_block) {
    int i;
    for (i = 0; i < assoc; i++) {
        if (i != used_block) {
            cache[i].timestamp++;
        }
    }
    cache[used_block].timestamp = 0;
}


// hint 1. LRU 알고리즘 교체 정책을 구현하기 위한 함수도 작성하셔서 적용하면 됩니다.
// hint 2. Random 알고리즘 교체 정책을 구현하기 위한 함수도 작성하셔서 적용하면 됩니다.

