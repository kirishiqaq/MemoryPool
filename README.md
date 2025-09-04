基于哈希映射的多种定长内存分配器




另：推测我使用的MinGW GCC 缺少标准的 C++11 线程类，从此库中补充得到thread 和 mutex 的内容
https://github.com/meganz/mingw-std-threads/tree/master

另：第二版小对象分配NUM_ALLOCS越大，反而New/Delete效率会更高 准备进行后续测试
