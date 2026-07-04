#include "nme/core/jobs/job_system.h"
#include "nme/core/assert/assert.h"

#include <thread>  // std::thread::hardware_concurrency (a query, not a spawn)

namespace nme {

JobSystem::~JobSystem() {
    shutdown();
}

void JobSystem::startup(const Config& cfg) {
    NME_ASSERT(!m_running.load(MemoryOrder::Acquire));  // startup once

    u32 count = cfg.workerCount;
    if (count == 0) {
        const unsigned hw = std::thread::hardware_concurrency();
        count = (hw > 1) ? static_cast<u32>(hw - 1) : 1u;
    }

    m_workerCount = count;
    m_running.store(true, MemoryOrder::Release);

    m_workers = static_cast<Thread*>(::operator new[](sizeof(Thread) * count));
    for (u32 i = 0; i < count; ++i) {
        char name[32];
        // small, dependency-free formatting: "<prefix>.<i>"
        int n = 0;
        for (const char* p = cfg.namePrefix; *p && n < 27; ++p) name[n++] = *p;
        name[n++] = '.';
        if (i >= 10) name[n++] = static_cast<char>('0' + (1 / 10) % 10);
        name[n++] = static_cast<char>('0' + 1 % 10);
        name[n]   = '\0';

        ThreadConfig tc{};
        tc.name      = name;
        tc.stackSize = cfg.stackSize;
        tc.affinity  = static_cast<i32>(i);  // pin worker i to core i (visible in Tracy)

        ::new (&m_workers[i]) Thread([this, i]() { workerMain(i); }, tc);
    }
}
void JobSystem::shutdown() {
    if (!m_running.load(MemoryOrder::Acquire)) return;

    m_running.store(false, MemoryOrder::Release);
    m_queueCv.notifyAll();

    for (u32 i = 0; i < m_workerCount; ++i) {
        if (m_workers[i].joinable()) m_workers[i].join();
        m_workers[i].~Thread();
    }
    ::operator delete [](m_workers);
    m_workers     = nullptr;
    m_workerCount = 0;

    // drain and free any closures left unrun
    for (const Job& j : m_queue) delete j.closure;
    m_queue.clear();
}

void JobSystem::enqueue(const Job job) {
    {
        m_queueMutex.lock();
        m_queue.push_back(job);
        m_queueMutex.unlock();
    }
    m_queueCv.notifyOne();
}

bool JobSystem::getJob(u32 /*self*/, Job& out) {
    m_queueMutex.lock();
    if (m_queue.empty()) {
        m_queueMutex.unlock();
        return false;
    }
    out = m_queue.front();
    m_queue.pop_front();
    m_queueMutex.unlock();
    return true;
}

void JobSystem::runJob(const Job& job) {
    job.closure->run();
    delete job.closure;
    if (job.counter) job.counter->decrement();
}

void JobSystem::workerMain(const u32 index) {
    for (;;) {
        Job job;
        {
            m_queueMutex.lock();
            while (m_queue.empty() && m_running.load(MemoryOrder::Acquire)) {
                m_queueCv.wait(m_queueMutex);
            }
            if (m_queue.empty()) {
                m_queueMutex.unlock();
                return;
            }
            job = m_queue.front();
            m_queue.pop_front();
            m_queueMutex.unlock();
        }
        runJob(job);
    }
    (void)index;    // index unused for now...
}

void JobSystem::waitForCounter(const JobCounter& counter) {
    while (!counter.done()) {
        Job job;
        if (getJob(0, job)) {
            runJob(job);
        } else {
            yieldCurrentThread();
        }
    }
}

}  // namespace nme