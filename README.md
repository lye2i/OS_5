# OS_5
*과제개요

설계과제 5는 프로그램에서 메모리를 동적으로 할당하는 사용자 지정 메모리 관리자를 직접 구현하는 것으로, 우리가 자주 쓰는 동적 메모리 할당인 malloc(3)과 메모리 해제 free(3) 함수를 직접 구현한다. 메모리 할당인 malloc(3)은 mmap(2)함수를 사용하여 메모리 페이지를 가져오고, 메모리 관리자는 직접 구현한 alloc()으로 이 메모리 페이지들의 chunk를 동적으로 할당한다. dealloc()를 구현함으로써 할당한 메모리 chunk를 해제하고 또한 cleanup()에서 munmap(2) 함수를 사용함으로써 가져온 메모리 페이지를 다시 반환한다. 메모리 할당에서 기본적인 원칙이 메모리 병합이 이루어져야 하며, 최적 적합, 최초 적합, 최악 작합 등의 방법을 사용하여 비어있는 chunk를 제대로 사용할 수 있도록 구현해야 한다. 또한 확장 가능한 Heap을 기반으로 한 메모리 관리자를 구현함으로써 malloc(3), free(3) 함수의 동작 원리와 메모리 관리 기법을 이해한다.