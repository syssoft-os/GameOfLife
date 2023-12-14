#include <iostream>
#include <cstdint>
#include <chrono>
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <random>
#include <mutex>
#include <atomic>
#include <cstring>
#include <condition_variable>

#if defined(__clang__) || defined(__GNUC__) || defined(__GNUG__)
#define POPCNT64 __builtin_popcountll
#elif defined(_MSC_VER)
#define POPCNT64 __popcnt64
#else
#error compiler not supported
#endif

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using f32 = float;
using f64 = double;

class ThreadPool
{
public:
    ThreadPool();
    ~ThreadPool();

    void WaitForTasks();
    template<typename Func, typename... Args>
    void PushTask(Func&& task, Args&&... args)
    {
        {
            const std::scoped_lock tasksLock(m_TasksMutex);
            m_Tasks.emplace(std::bind(std::forward<Func>(task), std::forward<Args>(args)...));
        }
        m_TaskAvailable.notify_one();
    }
    size_t ThreadCount() const { return m_ThreadCount; }
private:
    void Worker();
    void CreateThreads();
    void DestroyThreads();

    std::vector<std::thread> m_Threads;
    size_t m_ThreadCount;

    std::queue<std::function<void()>> m_Tasks;
    size_t m_RunningTasksCount = 0;
    std::mutex m_TasksMutex;
    std::condition_variable m_TasksDone;
    std::condition_variable m_TaskAvailable;

    bool m_Waiting = false;
    bool m_KeepWorkersRunning = false;
};

class TheGrid
{
public:
    TheGrid(u32 width, u32 height);
    TheGrid(u32 width, u32 height, i32 seed, f64 density);
    ~TheGrid();

    void SetCell(u32 x, u32 y, bool value);
    bool GetCell(u32 x, u32 y) const;
    void NextGeneration();
    void SimulateGenerations(u32 iterations);
    u32 CountLivingNeighbors(u32 x, u32 y) const;
    u32 NumberOfSurvivors() const;
    bool IsStatic();

    TheGrid& Parallel();
    TheGrid& Sequential();

    u32 Width() const { return m_Width; }
    u32 Height() const { return m_Height; }
private:
    void NextGenerationImpl();
    void NextGenerationParallelImpl();

    u32 m_Width;
    u32 m_Height;
    u32 m_HeightPadded;
    u32 m_WordsPerRow;
    u32 m_RightPadding;
    u32 m_LastWordCol;
    u64* m_CurGen;
    u64* m_NextGen;

    bool m_UseParallel = false;
    std::unique_ptr<ThreadPool> m_ThreadPool;
};

std::ostream& operator<<(std::ostream& stream, const TheGrid& grid)
{
    for (u32 row = 0; row < grid.Height(); ++row)
    {
        for (u32 col = 0; col < grid.Width(); ++col)
            stream << (grid.GetCell(col, row) ? 'X' : '.');
        stream << '\n';
    }
    return stream;
}

template<typename Func>
static f64 TimeFunction(Func&& f)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    f();
    auto endTime = std::chrono::high_resolution_clock::now();
    return (f64)(endTime - startTime).count() / 1e6;
}

static constexpr f64 Hash31Double(u32 x, u32 y, u32 z)
{
    u32 x2 = ((x >> 8) ^ y) * 1103515245;
    u32 y2 = ((y >> 8) ^ z) * 1103515245;
    u32 z2 = ((z >> 8) ^ x) * 1103515245;
    x = ((x2 >> 8) ^ y2) * 1103515245;
    y = ((y2 >> 8) ^ z2) * 1103515245;
    x2 = ((x >> 8) ^ y) * 1103515245;
    return (f64)(x2 & 2147483647) / 2147483647;
}

int main()
{
    static u32 width = 1000;
    static u32 height = 1000;
    static i32 seed = 10;
    static f64 density = 0.5;
    static u32 generations = 1000;

    f64 executionTime2 = TimeFunction([] {
        auto g = TheGrid(width, height, seed, density);
        g.Parallel().SimulateGenerations(generations);
        std::cout << "Survivors: " << g.NumberOfSurvivors() << '\n';
        if (g.IsStatic())
            std::cout << "The grid is static\n";
        else
            std::cout << "The grid is still living and not static\n";
        });
    std::cout << "Execution time: " << executionTime2 << " ms\n";
}

TheGrid::TheGrid(u32 width, u32 height)
    : m_Width(width), m_Height(height), m_HeightPadded(height + 2), m_WordsPerRow((width >> 6) + 2)
{
    m_RightPadding = (((m_WordsPerRow - 1) * 64 - m_Width) & 63);
    m_LastWordCol = m_RightPadding ? m_WordsPerRow - 1 : m_WordsPerRow - 2;
    m_CurGen = new u64[m_WordsPerRow * m_HeightPadded]{ 0 };
    m_NextGen = new u64[m_WordsPerRow * m_HeightPadded]{ 0 };
}

TheGrid::TheGrid(u32 width, u32 height, i32 seed, f64 density)
    : m_Width(width), m_Height(height), m_HeightPadded(height + 2), m_WordsPerRow((width >> 6) + 2)
{
    m_RightPadding = (((m_WordsPerRow - 1) * 64 - m_Width) & 63);
    m_LastWordCol = m_RightPadding ? m_WordsPerRow - 1 : m_WordsPerRow - 2;
    m_CurGen = new u64[m_WordsPerRow * m_HeightPadded]{ 0 };
    m_NextGen = new u64[m_WordsPerRow * m_HeightPadded]{ 0 };

    if (seed < 0)
        seed = std::random_device{}() & 2147483647;
    for (u32 col = 0; col < m_Width; col++)
        for (u32 row = 0; row < m_Height; row++)
            SetCell(col, row, Hash31Double(col, row, seed) < density);
}

TheGrid::~TheGrid()
{
    delete[] m_CurGen;
    delete[] m_NextGen;
}

void TheGrid::SetCell(u32 x, u32 y, bool value)
{
    y += 1;
    u32 arrayX = (x >> 6) + 1;
    u32 wordShift = (x & 63);
    m_CurGen[arrayX + y * m_WordsPerRow] = (m_CurGen[arrayX + y * m_WordsPerRow] & ~(1ll << wordShift))
        | ((u64)value << wordShift);
}

bool TheGrid::GetCell(u32 x, u32 y) const
{
    y += 1;
    u32 arrayX = (x >> 6) + 1;
    u32 wordShift = (x & 63);
    return m_CurGen[arrayX + y * m_WordsPerRow] & (1ll << wordShift);
}

void TheGrid::NextGeneration()
{
    if (m_UseParallel)
        NextGenerationParallelImpl();
    else
        NextGenerationImpl();
}

void TheGrid::SimulateGenerations(u32 iterations)
{
    if (m_UseParallel)
        for (u32 i = 0; i < iterations; ++i)
            NextGenerationParallelImpl();
    else
        for (u32 i = 0; i < iterations; ++i)
            NextGenerationImpl();
}

u32 TheGrid::CountLivingNeighbors(u32 x, u32 y) const
{
    u32 left = x > 0 ? x - 1 : m_Width - 1;
    u32 right = x + 1 < m_Width ? x + 1 : 0;
    u32 down = y > 0 ? y - 1 : m_Height - 1;
    u32 up = y + 1 < m_Height ? y + 1 : 0;
    return GetCell(left, down) + GetCell(x, down) + GetCell(right, down)
        + GetCell(left, y) + GetCell(right, y)
        + GetCell(left, up) + GetCell(x, up) + GetCell(right, up);
}

u32 TheGrid::NumberOfSurvivors() const
{
    u64 count = 0;
    for (u32 i = 1; i + 1 < m_HeightPadded; ++i)
    {
        for (u32 j = 1; j <= m_LastWordCol; ++j)
        {
            u64 cells = m_CurGen[j + i * m_WordsPerRow];
            if (j >= m_LastWordCol)
                cells = (cells << m_RightPadding) >> m_RightPadding;
            count += POPCNT64(cells);
        }
    }
    return (u32)count;
}

bool TheGrid::IsStatic()
{
    NextGeneration();
    bool result = true;
    for (u32 i = 1; i + 1 < m_HeightPadded; ++i)
    {
        for (u32 j = 1; j <= m_LastWordCol; ++j)
        {
            u64 cells1 = m_CurGen[j + i * m_WordsPerRow];
            u64 cells2 = m_NextGen[j + i * m_WordsPerRow];
            if (j >= m_LastWordCol)
            {
                cells1 = (cells1 << m_RightPadding) >> m_RightPadding;
                cells2 = (cells2 << m_RightPadding) >> m_RightPadding;
            }
            if (cells1 != cells2)
            {
                result = false;
                break;
            }
        }
    }

    std::swap(m_CurGen, m_NextGen);
    return result;
}

TheGrid& TheGrid::Parallel()
{
    if (m_UseParallel)
        return *this;
    m_UseParallel = true;
    m_ThreadPool = std::make_unique<ThreadPool>();
    return *this;
}

TheGrid& TheGrid::Sequential()
{
    if (!m_UseParallel)
        return *this;
    m_UseParallel = false;
    m_ThreadPool.reset();
    return *this;
}

static constexpr void Add2(u64 a, u64 b, u64& outSum, u64& outCarry)
{
    outSum = a ^ b;
    outCarry = a & b;
}

static constexpr void Add3(u64 a, u64 b, u64 c, u64& outSum0, u64& outSum1)
{
    u64 sum0 = 0;
    u64 carry0 = 0;
    u64 carry1 = 0;
    Add2(a, b, sum0, carry0);
    Add2(sum0, c, outSum0, carry1);
    outSum1 = carry0 | carry1;
}

static constexpr void Add3(u64 a, u64 b, u64 c, u64& outSum0, u64& outSum1, u64& outSum2)
{
    u64 sum0 = 0;
    u64 carry0 = 0;
    u64 carry1 = 0;
    Add2(a, b, sum0, carry0);
    Add2(sum0, c, outSum0, carry1);
    Add2(carry0, carry1, outSum1, outSum2);
}

void TheGrid::NextGenerationImpl()
{
    {
        u32 leftPadding = (64 - m_RightPadding) & 63;
        u32 yIndex = m_WordsPerRow;
        for (u32 y = 1; y + 1 < m_HeightPadded; ++y, yIndex += m_WordsPerRow)
        {
            m_CurGen[yIndex] = m_CurGen[m_LastWordCol + yIndex] << m_RightPadding;
            m_CurGen[m_LastWordCol + !m_RightPadding + yIndex] =
                ((m_CurGen[m_LastWordCol + !m_RightPadding + yIndex] << m_RightPadding) >> m_RightPadding) * (bool)m_RightPadding
                | m_CurGen[1 + yIndex] << leftPadding;
        }

        std::memcpy(m_CurGen, m_CurGen + (m_HeightPadded - 2) * m_WordsPerRow, m_WordsPerRow * sizeof(u64));
        std::memcpy(m_CurGen + (m_HeightPadded - 1) * m_WordsPerRow, m_CurGen + m_WordsPerRow, m_WordsPerRow * sizeof(u64));
    }

    for (u32 i = 1; i + 1 < m_HeightPadded; ++i)
    {
        for (u32 j = 1; j <= m_LastWordCol; ++j)
        {
            u64 cellsDown = m_CurGen[j + (i - 1) * m_WordsPerRow];
            u64 cells = m_CurGen[j + i * m_WordsPerRow];
            u64 cellsUp = m_CurGen[j + (i + 1) * m_WordsPerRow];

            u64 cellsLeft = cells << 1 | (m_CurGen[j - 1 + i * m_WordsPerRow]) >> 63;
            u64 cellsRight = cells >> 1;
            u64 cellsDownLeft = cellsDown << 1 | (m_CurGen[j - 1 + (i - 1) * m_WordsPerRow]) >> 63;
            u64 cellsDownRight = cellsDown >> 1;
            u64 cellsUpLeft = cellsUp << 1 | (m_CurGen[j - 1 + (i + 1) * m_WordsPerRow]) >> 63;
            u64 cellsUpRight = cellsUp >> 1;

            if (j + 1 < m_WordsPerRow)
            {
                cellsDownRight |= m_CurGen[j + 1 + (i - 1) * m_WordsPerRow] << 63;
                cellsRight |= m_CurGen[j + 1 + i * m_WordsPerRow] << 63;
                cellsUpRight |= m_CurGen[j + 1 + (i + 1) * m_WordsPerRow] << 63;
            }

            u64 sumLR0, sumLR1;
            u64 sumDown0, sumDown1, sumDown2;
            u64 sumUp0, sumUp1, sumUp2;
            u64 sum0, sum1, sum2, sum22;
            Add2(cellsLeft, cellsRight, sumLR0, sumLR1);
            Add3(cellsDown, cellsDownLeft, cellsDownRight, sumDown0, sumDown1, sumDown2);
            Add3(cellsUp, cellsUpLeft, cellsUpRight, sumUp0, sumUp1, sumUp2);
            Add3(sumLR0, sumDown0, sumUp0, sum0, sum1, sum2);
            Add3(sum1, sumLR1, sumDown1, sum1, sum22);
            sum2 |= sum22;
            Add2(sum1, sumUp1, sum1, sum22);
            sum2 |= sum22 | sumDown2 | sumUp2;

            m_NextGen[j + i * m_WordsPerRow] = ~sum2 & ((cells & sum1) | (~cells & sum0 & sum1));
        }
    }
    std::swap(m_CurGen, m_NextGen);
}

void TheGrid::NextGenerationParallelImpl()
{
    {
        u32 leftPadding = (64 - m_RightPadding) & 63;
        u32 yIndex = m_WordsPerRow;
        for (u32 y = 1; y + 1 < m_HeightPadded; ++y, yIndex += m_WordsPerRow)
        {
            m_CurGen[yIndex] = m_CurGen[m_LastWordCol + yIndex] << m_RightPadding;
            m_CurGen[m_LastWordCol + !m_RightPadding + yIndex] =
                ((m_CurGen[m_LastWordCol + !m_RightPadding + yIndex] << m_RightPadding) >> m_RightPadding) * (bool)m_RightPadding
                | m_CurGen[1 + yIndex] << leftPadding;
        }

        std::memcpy(m_CurGen, m_CurGen + (m_HeightPadded - 2) * m_WordsPerRow, m_WordsPerRow * sizeof(u64));
        std::memcpy(m_CurGen + (m_HeightPadded - 1) * m_WordsPerRow, m_CurGen + m_WordsPerRow, m_WordsPerRow * sizeof(u64));
    }

    static constexpr u32 c_RowsPerThread = 128;
    const u32 numRegions = (m_Height + c_RowsPerThread - 1) / c_RowsPerThread;

    for (u32 regionY = 0; regionY < numRegions; ++regionY)
    {
        m_ThreadPool->PushTask([this, regionY] {
            const u32 startY = regionY * c_RowsPerThread + 1;
            const u32 endY = startY + c_RowsPerThread < m_HeightPadded - 1 ? startY + c_RowsPerThread : (m_HeightPadded - 1);
            for (u32 i = startY; i < endY; ++i)
            {
                for (u32 j = 1; j <= m_LastWordCol; ++j)
                {
                    u64 cellsDown = m_CurGen[j + (i - 1) * m_WordsPerRow];
                    u64 cells = m_CurGen[j + i * m_WordsPerRow];
                    u64 cellsUp = m_CurGen[j + (i + 1) * m_WordsPerRow];

                    u64 cellsLeft = cells << 1 | (m_CurGen[j - 1 + i * m_WordsPerRow]) >> 63;
                    u64 cellsRight = cells >> 1;
                    u64 cellsDownLeft = cellsDown << 1 | (m_CurGen[j - 1 + (i - 1) * m_WordsPerRow]) >> 63;
                    u64 cellsDownRight = cellsDown >> 1;
                    u64 cellsUpLeft = cellsUp << 1 | (m_CurGen[j - 1 + (i + 1) * m_WordsPerRow]) >> 63;
                    u64 cellsUpRight = cellsUp >> 1;

                    if (j + 1 < m_WordsPerRow)
                    {
                        cellsDownRight |= m_CurGen[j + 1 + (i - 1) * m_WordsPerRow] << 63;
                        cellsRight |= m_CurGen[j + 1 + i * m_WordsPerRow] << 63;
                        cellsUpRight |= m_CurGen[j + 1 + (i + 1) * m_WordsPerRow] << 63;
                    }

                    u64 sumLR0, sumLR1;
                    u64 sumDown0, sumDown1, sumDown2;
                    u64 sumUp0, sumUp1, sumUp2;
                    u64 sum0, sum1, sum2, sum22;
                    Add2(cellsLeft, cellsRight, sumLR0, sumLR1);
                    Add3(cellsDown, cellsDownLeft, cellsDownRight, sumDown0, sumDown1, sumDown2);
                    Add3(cellsUp, cellsUpLeft, cellsUpRight, sumUp0, sumUp1, sumUp2);
                    Add3(sumLR0, sumDown0, sumUp0, sum0, sum1, sum2);
                    Add3(sum1, sumLR1, sumDown1, sum1, sum22);
                    sum2 |= sum22;
                    Add2(sum1, sumUp1, sum1, sum22);
                    sum2 |= sum22 | sumDown2 | sumUp2;

                    m_NextGen[j + i * m_WordsPerRow] = ~sum2 & ((cells & sum1) | (~cells & sum0 & sum1));
                }
            }
            });
    }
    m_ThreadPool->WaitForTasks();

    std::swap(m_CurGen, m_NextGen);
}

ThreadPool::ThreadPool()
    : m_ThreadCount(std::max((size_t)std::thread::hardware_concurrency(), (size_t)1))
{
    m_Threads.resize(m_ThreadCount);
    CreateThreads();
}

ThreadPool::~ThreadPool()
{
    WaitForTasks();
    DestroyThreads();
}

void ThreadPool::WaitForTasks()
{
    std::unique_lock tasksLock(m_TasksMutex);
    m_Waiting = true;
    m_TasksDone.wait(tasksLock, [this] { return !m_RunningTasksCount && m_Tasks.empty(); });
    m_Waiting = false;
}

void ThreadPool::Worker()
{
    std::function<void()> task;
    while (true)
    {
        std::unique_lock tasksLock(m_TasksMutex);
        m_TaskAvailable.wait(tasksLock, [this] { return !m_Tasks.empty() || !m_KeepWorkersRunning; });
        if (!m_KeepWorkersRunning)
            break;
        task = std::move(m_Tasks.front());
        m_Tasks.pop();
        ++m_RunningTasksCount;
        tasksLock.unlock();
        task();
        tasksLock.lock();
        --m_RunningTasksCount;
        if (m_Waiting && !m_RunningTasksCount && m_Tasks.empty())
            m_TasksDone.notify_all();
    }
}

void ThreadPool::CreateThreads()
{
    {
        const std::scoped_lock tasksLock(m_TasksMutex);
        m_KeepWorkersRunning = true;
    }
    for (size_t i = 0; i < m_ThreadCount; ++i)
        m_Threads[i] = std::thread(&ThreadPool::Worker, this);
}

void ThreadPool::DestroyThreads()
{
    {
        const std::scoped_lock tasksLock(m_TasksMutex);
        m_KeepWorkersRunning = false;
    }
    m_TaskAvailable.notify_all();
    for (auto& thread : m_Threads)
        thread.join();
}
