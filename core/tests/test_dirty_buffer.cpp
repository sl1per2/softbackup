#include "backup_engine/dirty_buffer_logger.h"
#include <cassert>
#include <iostream>
#include <thread>

class MockFlusher : public obs::IBufferFlusher {
public:
    std::string plugin_name() const override { return "mock_plugin"; }

    obs::BufferState query_dirty_state() override {
        obs::BufferState state;
        state.page_count = dirty_pages_;
        state.size_bytes = dirty_pages_ * 4096;
        state.percent_of_ram = (state.size_bytes * 100.0) / (16ULL * 1024 * 1024 * 1024);
        return state;
    }

    bool flush_dirty_buffers() override {
        if (should_fail_) return false;
        dirty_pages_ = after_flush_pages_;
        return true;
    }

    std::vector<obs::ComponentDetail> get_component_details() override {
        return {
            {"buffer_pool", dirty_pages_ / 2, (dirty_pages_ / 2) * 4096},
            {"redo_log", dirty_pages_ / 4, (dirty_pages_ / 4) * 4096},
            {"data_cache", dirty_pages_ / 4, (dirty_pages_ / 4) * 4096}
        };
    }

    uint64_t get_total_ram() override { return 16ULL * 1024 * 1024 * 1024; }
    uint64_t get_buffer_pool_size() override { return 4ULL * 1024 * 1024 * 1024; }

    void set_dirty_pages(int64_t pages) { dirty_pages_ = pages; }
    void set_after_flush_pages(int64_t pages) { after_flush_pages_ = pages; }
    void set_should_fail(bool fail) { should_fail_ = fail; }

private:
    int64_t dirty_pages_ = 1000;
    int64_t after_flush_pages_ = 0;
    bool should_fail_ = false;
};

void test_dirty_buffer_logger_basic() {
    std::cout << "--- test_dirty_buffer_logger_basic ---" << std::endl;
    obs::DirtyBufferLogger logger("/tmp/test_dirty_buf_log.db", "");

    MockFlusher flusher;
    flusher.set_dirty_pages(2341);
    flusher.set_after_flush_pages(0);

    auto result = logger.flush_and_log("job_001", "agent_001", &flusher);

    assert(result.job_id == "job_001");
    assert(result.agent_id == "agent_001");
    assert(result.plugin_name == "mock_plugin");
    assert(result.before.page_count == 2341);
    assert(result.before.size_bytes == 2341 * 4096);
    assert(result.after.page_count == 0);
    assert(result.status == obs::FlushStatus::FLUSHED);
    assert(result.is_consistent == true);
    assert(result.consistency == obs::ConsistencyLevel::APPLICATION_CONSISTENT);
    assert(result.flush_duration_ms >= 0);
    assert(result.component_details.size() == 3);
    assert(result.total_ram_bytes == 16ULL * 1024 * 1024 * 1024);

    std::cout << "[PASS] test_dirty_buffer_logger_basic" << std::endl;
}

void test_dirty_buffer_logger_partial_flush() {
    std::cout << "--- test_dirty_buffer_logger_partial_flush ---" << std::endl;
    obs::DirtyBufferLogger logger("/tmp/test_dirty_buf_log_partial.db", "");

    MockFlusher flusher;
    flusher.set_dirty_pages(5000);
    flusher.set_after_flush_pages(1200);

    auto result = logger.flush_and_log("job_002", "agent_001", &flusher);

    assert(result.before.page_count == 5000);
    assert(result.after.page_count == 1200);
    assert(result.status == obs::FlushStatus::FLUSHED);
    assert(result.is_consistent == false);
    assert(result.consistency == obs::ConsistencyLevel::FILE_CONSISTENT);

    std::cout << "[PASS] test_dirty_buffer_logger_partial_flush" << std::endl;
}

void test_dirty_buffer_logger_flush_failed() {
    std::cout << "--- test_dirty_buffer_logger_flush_failed ---" << std::endl;
    obs::DirtyBufferLogger logger("/tmp/test_dirty_buf_log_fail.db", "");

    MockFlusher flusher;
    flusher.set_dirty_pages(1000);
    flusher.set_should_fail(true);

    auto result = logger.flush_and_log("job_003", "agent_002", &flusher);

    assert(result.before.page_count == 1000);
    assert(result.after.page_count == 1000);
    assert(result.status == obs::FlushStatus::FAILED);
    assert(result.is_consistent == false);
    assert(result.consistency == obs::ConsistencyLevel::CRASH_CONSISTENT);
    assert(!result.error_message.empty());

    std::cout << "[PASS] test_dirty_buffer_logger_flush_failed" << std::endl;
}

void test_dirty_buffer_logger_query_logs() {
    std::cout << "--- test_dirty_buffer_logger_query_logs ---" << std::endl;
    obs::DirtyBufferLogger logger("/tmp/test_dirty_buf_log_query.db", "");

    MockFlusher flusher;
    flusher.set_dirty_pages(100);

    logger.flush_and_log("job_q1", "agent_001", &flusher);
    logger.flush_and_log("job_q2", "agent_001", &flusher);

    auto logs = logger.get_logs("job_q1");
    assert(logs.size() == 1);
    assert(logs[0].backup_job_id == "job_q1");
    assert(logs[0].before_page_count == 100);

    auto all = logger.get_all_logs(10);
    assert(all.size() >= 2);

    std::cout << "[PASS] test_dirty_buffer_logger_query_logs" << std::endl;
}

void test_dirty_buffer_logger_failed_queries() {
    std::cout << "--- test_dirty_buffer_logger_failed_queries ---" << std::endl;
    obs::DirtyBufferLogger logger("/tmp/test_dirty_buf_log_failq.db", "");

    MockFlusher good_flusher;
    good_flusher.set_dirty_pages(50);

    MockFlusher bad_flusher;
    bad_flusher.set_dirty_pages(200);
    bad_flusher.set_should_fail(true);

    logger.flush_and_log("job_f1", "agent_001", &good_flusher);
    logger.flush_and_log("job_f2", "agent_001", &bad_flusher);

    auto failed = logger.get_failed_flushes();
    assert(failed.size() == 1);
    assert(failed[0].backup_job_id == "job_f2");
    assert(failed[0].flush_status == "failed");

    auto inconsistent = logger.get_inconsistent_backups();
    assert(inconsistent.size() == 1);

    std::cout << "[PASS] test_dirty_buffer_logger_failed_queries" << std::endl;
}

void test_dirty_buffer_logger_json() {
    std::cout << "--- test_dirty_buffer_logger_json ---" << std::endl;
    obs::DirtyBufferLogger logger("/tmp/test_dirty_buf_log_json.db", "");

    MockFlusher flusher;
    flusher.set_dirty_pages(500);

    auto result = logger.flush_and_log("job_j1", "agent_001", &flusher);

    assert(!result.component_details.empty());
    assert(result.component_details[0].name == "buffer_pool");

    std::cout << "[PASS] test_dirty_buffer_logger_json" << std::endl;
}

int main() {
    test_dirty_buffer_logger_basic();
    test_dirty_buffer_logger_partial_flush();
    test_dirty_buffer_logger_flush_failed();
    test_dirty_buffer_logger_query_logs();
    test_dirty_buffer_logger_failed_queries();
    test_dirty_buffer_logger_json();
    std::cout << "\nAll DirtyBufferLogger tests passed!" << std::endl;
    return 0;
}
