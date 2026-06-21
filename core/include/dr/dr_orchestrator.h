#pragma once
#include "common.h"
#include <functional>
#include <vector>
#include <string>
#include <thread>

namespace obs {

enum class DrPlanStatus { DRAFT, READY, RUNNING, COMPLETED, FAILED, PARTIAL, PENDING };
enum class DrStepStatus { PENDING, RESTORING, RUNNING, COMPLETED, FAILED, SKIPPED };

struct DrPlanStep {
    int order = 0;
    std::string vm_source_backup_id;
    std::string vm_name;
    std::string target_host;
    std::string target_network;
    std::string target_ip;
    int boot_delay_seconds = 30;
    std::string pre_script;
    std::string post_script;
    std::vector<std::string> depends_on;
    int64_t memory_mb = 1024;
    int32_t cpu_count = 2;
};

struct DrPlan {
    std::string plan_id;
    std::string name;
    std::string description;
    std::vector<DrPlanStep> steps;
    std::string created_at;
    std::string updated_at;
};

struct DrStepResult {
    int order = 0;
    std::string vm_name;
    DrStepStatus status = DrStepStatus::PENDING;
    std::string started_at;
    std::string completed_at;
    int64_t rto_seconds = 0;
    std::string error;
    std::string dr_ip;
};

struct DrRun {
    std::string run_id;
    std::string plan_id;
    DrPlanStatus status = DrPlanStatus::PENDING;
    std::vector<DrStepResult> steps_status;
    std::string started_at;
    std::string completed_at;
    int64_t total_rto_seconds = 0;
    std::string error;
};

using DrProgressCallback = std::function<void(const DrRun&)>;

class DrOrchestrator {
public:
    DrOrchestrator();
    ~DrOrchestrator();

    std::string create_plan(const DrPlan& plan);
    bool update_plan(const std::string& plan_id, const DrPlan& plan);
    bool delete_plan(const std::string& plan_id);
    DrPlan get_plan(const std::string& plan_id);
    std::vector<DrPlan> list_plans();

    DrRun run_plan(const std::string& plan_id, DrProgressCallback callback = nullptr);
    DrRun get_run_status(const std::string& run_id);
    bool cancel_run(const std::string& run_id);

    bool failback(const std::string& run_id);

private:
    void execute_step(const DrPlanStep& step, DrStepResult& result);
    void sort_steps_by_dependency(DrPlan& plan);
    bool restore_vm_step(const DrPlanStep& step, DrStepResult& result);
    bool run_pre_script(const std::string& script);
    bool run_post_script(const std::string& script);
    bool wait_for_vm_ready(const std::string& ip, int timeout_seconds);

    std::map<std::string, DrPlan> plans_;
    std::map<std::string, DrRun> runs_;
    mutable std::mutex plans_mutex_;
    mutable std::mutex runs_mutex_;
};

} // namespace obs
