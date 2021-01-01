#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "alloc.h"

struct mem_chunk{ //chunk 구조체
	int valid;
	char *start_addr;
	char *end_addr;
	int size;
};

int chunk_num; //chunk 갯수
struct mem_chunk chunk[512];

int init_alloc(void) //4KB 페이지 할당
{
	chunk_num = 0;

	char *start_mem_addr = NULL; //시작 주소

	for(int i=0; i<512; i++){
		memset(&chunk[i], 0, sizeof(chunk[i]));
	}

	start_mem_addr = mmap(0, PAGESIZE, PROT_WRITE|PROT_READ, MAP_ANONYMOUS|MAP_PRIVATE,-1, 0); //4KB 할당

	if(start_mem_addr == MAP_FAILED){ //할당 에러 시 종료
		perror("mmap error");
		return 1;
	}

	else{ //성공 시 chunk 정보 할당, 리턴 0
		memset(start_mem_addr,0, PAGESIZE);
		chunk[0].start_addr = start_mem_addr;
		chunk[0].end_addr = start_mem_addr + PAGESIZE;
		chunk[0].size = PAGESIZE;
		chunk[0].valid = 1;
		return 0;
	}
}

int cleanup(void){ //memory page 반환

	for(int i=0; i<512; i++){ //구조체 배열 초기화
		memset(&chunk[i], 0, sizeof(chunk[i]));
	}

	if(munmap(chunk[0].start_addr, PAGESIZE)==-1){
		perror("munmap error");
		return 1;
	}

	return 0; //성공 시
}

char *alloc(int alloc_size){ //chunk 할당
	if(alloc_size % 8 != 0) //8바이트의 배수가 아니면 NULL 리턴
		return NULL;

	for(int i=0; i<=chunk_num; i++){
		if(chunk[i].valid == 1){			
			if(chunk[i].size == alloc_size){ //사용 가능한 chunk가 size가 같을때
				chunk[i].valid = 0; // 해당 chunk 사용
				return chunk[i].start_addr;
			}

			else if(chunk[i].size > alloc_size){ //사용 가능한 chunk가 size가 클 때
				chunk_num++; //chunk 갯수 증가

				for(int j=chunk_num; j>i+1; j--){ //chunk가 추가 되므로 원래 있던 chunk정보 이동
					chunk[j].valid = chunk[j-1].valid;
					chunk[j].size = chunk[j-1].size;
					chunk[j].start_addr = chunk[j-1].start_addr;
					chunk[j].end_addr = chunk[j-1].end_addr;
				}

				chunk[i+1].valid = 1; //남은 공간을 가진 chunk 생성
				chunk[i+1].size = chunk[i].size - alloc_size;
				chunk[i+1].start_addr = chunk[i].start_addr + alloc_size;
				chunk[i+1].end_addr = chunk[i].end_addr;

				chunk[i].valid = 0; //해당 chunk 사용
				chunk[i].size = alloc_size; 
				chunk[i].end_addr = chunk[i].start_addr + alloc_size;
				return chunk[i].start_addr;
			}
		}
	}

	return NULL; //할당 할 chunk 없다면
}


void dealloc(char *chunk_ptr){ //chunk 해제
	int new_chunk_num = chunk_num; //chunk를 해제하고 난 뒤의 chunk 수
	int merge_chunk = 0; //병합된 chunk

	for(int i=0; i<=chunk_num; i++){
		if(chunk[i].start_addr == chunk_ptr){ //해당 포인터를 시작으로 갖는 chunk 찾기
			chunk[i].valid = 1;
			memset(chunk_ptr, 0, chunk[i].size);
			chunk_ptr = NULL;

			if(i != chunk_num && chunk[i+1].valid == 1){ //바로 뒤에 인접한 chunk가 비어있다면 병합
				chunk[i].size = chunk[i].size + chunk[i+1].size;
				chunk[i].end_addr = chunk[i+1].end_addr;
				chunk[i+1].valid = -1; //병합된 chunk라고 표시
				new_chunk_num--; //chunk 갯수 줄어듬
				merge_chunk = i+1; 
			}

			if(i != 0 && chunk[i-1].valid == 1){ //바로 앞에 인접한 chunk가 비어있다면 병합
				chunk[i-1].size = chunk[i].size + chunk[i-1].size;
				chunk[i-1].end_addr = chunk[i].end_addr;
				chunk[i].valid = -1; //병합된 chunk라고 표시
				new_chunk_num--; //chunk 갯수 줄어듬
				merge_chunk = i;
			}

			if(chunk_num - new_chunk_num == 0) //병합한 chunk가 없다면
				break;

			else{ //병합한 chunk가 있다면
				for(int j=merge_chunk; j<=new_chunk_num; j++){ //chunk 정보 배열에 정렬
					chunk[j].valid = chunk[j + chunk_num - new_chunk_num].valid;
					chunk[j].start_addr = chunk[j + chunk_num - new_chunk_num].start_addr;
					chunk[j].end_addr = chunk[j + chunk_num - new_chunk_num].end_addr;
					chunk[j].size = chunk[j + chunk_num - new_chunk_num].size;
				}
			}

			chunk_num = new_chunk_num; //새로운 chunk갯수 저장
			break;
		}
	}
}
