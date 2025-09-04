#include "../include/MemoryPool.h"
#include <mutex>
namespace KirishimemoryPool 
{
MemoryPool::MemoryPool(size_t BlockSize)
    : BlockSize_ (BlockSize)
    , SlotSize_ (0)
    , firstBlock_ (nullptr)
    , curSlot_ (nullptr)
    , freeList_ (nullptr)
    , lastSlot_ (nullptr)
{}

MemoryPool::~MemoryPool()
{
    Slot* cur = firstBlock_;
    while (cur)
    {
        Slot* next = cur->next;
        // free(reinterpret_cast<void*>(firstBlock_));
        operator delete(reinterpret_cast<void*>(cur));
        cur = next;
    }
}

void MemoryPool::init(size_t size)
{
    assert(size > 0);
    SlotSize_ = size;
    firstBlock_ = nullptr;
    curSlot_ = nullptr;
    freeList_ = nullptr;
    lastSlot_ = nullptr;
}

void* MemoryPool::allocate()
{
    // 优先使用空闲链表中的内存槽
    Slot* slot = popFreeList();
    if (slot != nullptr)
        return slot;

    Slot* temp;
    {   
        std::lock_guard<std::mutex> lock(mutexForBlock_);
        if (curSlot_ >= lastSlot_)
        {
            allocateNewBlock();
        }
    
        temp = curSlot_;
        curSlot_ += SlotSize_ / sizeof(Slot);
    }
    
    return temp; 
}

void MemoryPool::deallocate(void* ptr)
{
    if (!ptr) return;

    Slot* slot = reinterpret_cast<Slot*>(ptr);
    pushFreeList(slot);
}

void MemoryPool::allocateNewBlock()
{   
    // 头插
    void* newBlock = operator new(BlockSize_);
    reinterpret_cast<Slot*>(newBlock)->next = firstBlock_;
    firstBlock_ = reinterpret_cast<Slot*>(newBlock);

    char* body = reinterpret_cast<char*>(newBlock) + sizeof(Slot*);
    size_t paddingSize = padPointer(body, SlotSize_); 
    curSlot_ = reinterpret_cast<Slot*>(body + paddingSize);

    //申请新内存块
    lastSlot_ = reinterpret_cast<Slot*>(reinterpret_cast<size_t>(newBlock) + BlockSize_ - SlotSize_ + 1);
}

// 指针对齐
size_t MemoryPool::padPointer(char* p, size_t align)
{

    return align - (reinterpret_cast<size_t>(p) % align);
}

// 无锁入队
bool MemoryPool::pushFreeList(Slot* slot)
{
    while (true)
    {
        Slot* oldHead = freeList_.load(std::memory_order_relaxed);
        slot->next.store(oldHead, std::memory_order_relaxed);

        if (freeList_.compare_exchange_weak(oldHead, slot,
         std::memory_order_release, std::memory_order_relaxed))
        {
            return true;
        }
    }
}

// 无锁出队
Slot* MemoryPool::popFreeList()
{
    while (true)
    {
        Slot* oldHead = freeList_.load(std::memory_order_acquire);
        if (oldHead == nullptr)
            return nullptr; 

        Slot* newHead = nullptr;
        try
        {
            newHead = oldHead->next.load(std::memory_order_relaxed);
        }
        catch(...)
        {
            continue;
        }
        
        if (freeList_.compare_exchange_weak(oldHead, newHead,
         std::memory_order_acquire, std::memory_order_relaxed))
        {
            return oldHead;
        }
    }
}


void HashBucket::initMemoryPool()
{
    for (int i = 0; i < MEMORY_POOL_NUM; i++)
    {
        getMemoryPool(i).init((i + 1) * SLOT_BASE_SIZE);
    }
}   

// 单例模式
MemoryPool& HashBucket::getMemoryPool(int index)
{
    static MemoryPool memoryPool[MEMORY_POOL_NUM];
    return memoryPool[index];
}

} 

