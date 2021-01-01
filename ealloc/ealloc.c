#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include "ealloc.h"

struct mem_chunk{ //chunk 구조체
	int valid;
	char *start_addr;
	char *end_addr;
	int size;
	int chunk_in_page; //페이지의 첫번째 블록인지 구분
};

int chunk_num; //chunk 갯수
int page_num; //페이지 갯수

struct mem_chunk chunk[64];

void init_alloc(void) //메모리 관리자 초기화
{
	for(int i=0; i<64; i++){
		memset(&chunk[i], 0, sizeof(chunk[i]));
	}

	chunk_num = -1; //할당된 페이지 없음
	page_num = 0; 
}

void cleanup(void){ //메모리 관리자 정리
	chunk_num = -1; //할당된 페이지 없음
	page_num = 0;

	for(int i=0; i<64; i++){
		memset(&chunk[i], 0, sizeof(chunk[i]));
	}
}

char *alloc(int alloc_size){ //chunk 할당
	if(alloc_size % 256 != 0 || alloc_size >PAGESIZE) //256 바이트의 배수가 아니거나 페이지 크기를 넘으면 NULL 리턴
		return NULL;

	char *start_mem_addr = NULL; //시작주소

	if(chunk_num==-1){ //첫 번째 mmap 호출
		start_mem_addr = mmap(0, PAGESIZE, PROT_WRITE|PROT_READ, MAP_ANONYMOUS|MAP_PRIVATE,-1, 0); //4KB 할당
		if(start_mem_addr == MAP_FAILED){ //할당 에러 시
			perror("mmap error");
			return NULL;
		}
		else{ //할당 성공 시
			chunk[0].start_addr = start_mem_addr;
			chunk[0].end_addr = start_mem_addr + PAGESIZE;
			chunk[0].size = PAGESIZE;
			chunk[0].valid = 1;
			chunk[0].chunk_in_page = 1;
			page_num = 1;
			chunk_num = 0;
		}
	}

	while(1){

		for(int i=0; i<=chunk_num; i++){ //할당된 메모리 페이지 내의 free공간 있는지 확인
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
						chunk[j].chunk_in_page = chunk[j-1].chunk_in_page;
					}

					chunk[i+1].valid = 1; //남은 공간을 가진 chunk 생성
					chunk[i+1].size = chunk[i].size - alloc_size;
					chunk[i+1].start_addr = chunk[i].start_addr + alloc_size;
					chunk[i+1].end_addr = chunk[i].end_addr;
					chunk[i+1].chunk_in_page = 0;

					chunk[i].valid = 0; //해당 chunk 사용
					chunk[i].size = alloc_size; 
					chunk[i].end_addr = chunk[i].start_addr + alloc_size;
					return chunk[i].start_addr;
				}
			}
		}

		if(page_num<4){ //새로운 page를 할당할 수 있다면
			start_mem_addr = mmap(0, PAGESIZE, PROT_WRITE|PROT_READ, MAP_ANONYMOUS|MAP_PRIVATE,-1, 0); //4KB 할당

			if(start_mem_addr == MAP_FAILED){ //할당 에러 시 종료
				perror("mmap error");
				return NULL;
			}

			else{ //성공 시 chunk 정보 할당, 리턴 0
				chunk_num++;
				page_num++;
				chunk[chunk_num].start_addr = start_mem_addr;
				chunk[chunk_num].end_addr = start_mem_addr + PAGESIZE;
				chunk[chunk_num].size = PAGESIZE;
				chunk[chunk_num].valid = 1;
				chunk[chunk_num].chunk_in_page = 1;
			}
		}

		else
			break;
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

			if(i != chunk_num && chunk[i+1].valid == 1 && chunk[i+1].chunk_in_page == 0){ //바로 뒤에 인접한 chunk가 비어있다면 병합
				chunk[i].size = chunk[i].size + chunk[i+1].size;
				chunk[i].end_addr = chunk[i+1].end_addr;
				chunk[i+1].valid = -1; //병합된 chunk라고 표시
				new_chunk_num--; //chunk 갯수 줄어듬
				merge_chunk = i+1; 
			}

			if(chunk[i].chunk_in_page == 0 && chunk[i-1].valid == 1){ //바로 앞에 인접한 chunk가 비어있다면 병합
				chunk[i-1].size = chunk[i].size + chunk[i-1].size;
				chunk[i-1].end_addr = chunk[i].end_addr;
				chunk[i].valid = -1; //병합된 chunk라고 표시
				new_chunk_num--; //chunk 갯수 줄어듬
				merge_chunk = i;
			}

			if(chunk_num - new_chunk_num == 0) //병합한 chunk가 없다면
				break;

			else{ //병합된 chunk가 있다면				
				for(int j = merge_chunk; j<=new_chunk_num; j++){ //chunk 정보 배열에 정렬
					chunk[j].valid = chunk[j + chunk_num - new_chunk_num].valid;
					chunk[j].start_addr = chunk[j + chunk_num - new_chunk_num].start_addr;
					chunk[j].end_addr = chunk[j + chunk_num - new_chunk_num].end_addr;
					chunk[j].size = chunk[j + chunk_num - new_chunk_num].size;
					chunk[j].chunk_in_page = chunk[j + chunk_num - new_chunk_num].chunk_in_page;
				}
			}

			chunk_num = new_chunk_num; //새로운 chunk갯수 저장
			break;
		}
	}
}
