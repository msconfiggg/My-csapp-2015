#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

typedef struct cache_line {
    int valid;                              //有效位
    int tag;                                //标记位
    int time_stamp;                          //时间戳
} Cache_line;

typedef struct cache {
    int S;                                  //set数量
    int E;                                  //每个set的行数
    int B;                                  //block大小
    Cache_line **lines;                     //每个set是一维，每个line是二维
} Cache;

int hit_count = 0;                          //命中
int miss_count = 0;                         //不命中
int eviction_count = 0;                     //驱逐
int verbose = 0;                            //是否打印详细信息
char fileName[100];                         //文件名
Cache *cache = NULL;

//初始化cache
void init_cache(int s, int E, int b) {
    cache = (Cache *) malloc(sizeof(Cache));
    cache->S = 1 << s;
    cache->E = E;
    cache->B = 1 << b;
    cache->lines = (Cache_line **) malloc(cache->S * sizeof(Cache_line *));

    for (int i = 0; i < cache->S; i++) {
        cache->lines[i] = (Cache_line *) malloc(cache->E * sizeof(Cache_line));
        for (int j = 0; j < cache->E; j++) {
            cache->lines[i][j].valid = 0;
            cache->lines[i][j].tag = -1;
            cache->lines[i][j].time_stamp = 0;
        }
    }
}

//释放cache
void free_cache() {
    for (int i = 0; i < cache->S; i++) {
        free(cache->lines[i]);
    }

    free(cache->lines);
    free(cache);
}

//如果hit，直接返回index
int get_index(int op_set, int op_tag) {
    for (int i = 0; i < cache->E; i++) {
        if (cache->lines[op_set][i].valid == 1 && cache->lines[op_set][i].tag == op_tag) {
            return i;
        }
    }

    return -1;
}

//如果miss，找到第一个valid为0的index
int is_full(int op_set) {
    for (int i = 0; i < cache->E; i++) {
        if (cache->lines[op_set][i].valid == 0) {
            return i;
        }
    }

    return -1;
}

//如果miss且cache满了，找到LRU的index
int find_LRU(int op_set) {
    int max_index = -1;
    int max_time_stamp = -1;
    for (int i = 0; i < cache->E; i++) {
        if (cache->lines[op_set][i].time_stamp > max_time_stamp) {
            max_index = i;
            max_time_stamp = cache->lines[op_set][i].time_stamp;
        }
    }

    return max_index;
}

//更新cache
void update_cache(int index, int op_set, int op_tag) {
    cache->lines[op_set][index].valid = 1;
    cache->lines[op_set][index].tag = op_tag;
    for (int i = 0; i < cache->E; i++) {
        if (cache->lines[op_set][i].valid == 1) {
            cache->lines[op_set][i].time_stamp++;
        }
    }

    cache->lines[op_set][index].time_stamp = 0;
}

//更新
void update_info(int op_set, int op_tag) {
    int index = get_index(op_set, op_tag);
    if (index == -1) {
        miss_count++;
        if (verbose) {
            printf("miss ");
        }

        index = is_full(op_set);
        if (index == -1) {
            eviction_count++;
            if (verbose) {
                printf("eviction ");
            }

            index = find_LRU(op_set);
        }
    } else {
        hit_count++;
        if (verbose) {
            printf("hit ");
        }
    }

    update_cache(index, op_set, op_tag);
}

void get_trace(int s, int E, int b) {
    FILE *file = fopen(fileName, "r");
    if (file == NULL) {
        exit(-1);
    }

    char operation;
    unsigned address;
    int size;
    while (fscanf(file, " %c %x,%d", &operation, &address, &size) > 0) {
        int op_set = address >> b & ((1 << s) - 1);
        int op_tag = address >> (b + s);
        switch (operation) {
        case 'L':
            update_info(op_set, op_tag);
            break;
        case 'S':
            update_info(op_set, op_tag);
            break;
        case 'M':
            update_info(op_set, op_tag);
            update_info(op_set, op_tag);
            break;
        default:
            break;
        }
    }

    fclose(file);
}

void print_help() {
    printf("Usage: ./csim-ref [-hv]-s <s>-E <E>-b <b>-t <tracefile>\n\n");
    printf("Options:\n");
    printf("-h              Optional help flag that prints usage info\n");
    printf("-v              Optional verbose flag that displays trace info\n");
    printf("-s <s>          Number ofset index bits (S = 2^s is the number of sets)\n");
    printf("-E <E>          Associativity (number of lines per set)\n");
    printf("-b <b>          Number of block bits (B = 2^b is the block size)\n");
    printf("-t <tracefile>  Name of the valgrind trace to replay\n\n");
    printf("Examples:\n");
    printf("linux>  ./csim-ref -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("linux>  ./csim-ref -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}
 
int main(int argc, char *argv[]) {
    char opt;
    int s, E, b;
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (opt){
        case 'h':
            print_help();
            break;
        case 'v':
            verbose = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            strcpy(fileName, optarg);
            break;
        default:
            break;
        }
    }

    init_cache(s, E, b);
    get_trace(s, E, b);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
