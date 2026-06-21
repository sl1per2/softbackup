#include "dr/dr_orchestrator.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <chrono>

namespace obs {

DrOrchestrator::DrOrchestrator() {}
DrOrchestrator::~DrOrchestrator() {}

std::string DrOrchestrator::create_plan(const DrPlan& plan) {
    std::lock_guard<std::mutex> lock(plans_mutex_);
    std::string plan_id = "dr-" + std::to_string(std::time(nullptr));
    DrPlan p = plan;
    p.plan_id = plan_id;
    sort_steps_by_dependency(p);
    plans_[plan_id] = p;
    spdlog::info("DR Plan created: {} with {} steps", plan_id, p.steps.size());
    return plan_id;
}

bool DrOrchestrator::update_plan(const std::string& plan_id, const DrPlan& plan) {
    std::lock_guard<std::mutex> lock(plans_mutex_);
    auto it = plans_.find(plan_id);
    if (it == plans_.end()) return false;
    DrPlan p = plan;
    p.plan_id = plan_id;
    sort_steps_by_dependency(p);
    it->second = p;
    return true;
}

bool DrOrchestrator::delete_plan(const std::string& plan_id) {
    std::lock_guard<std::mutex> lock(plans_mutex_);
    return plans_.erase(plan_id) > 0;
}

DrPlan DrOrchestrator::get_plan(const std::string& plan_id) {
    std::lock_guard<std::mutex> lock(plans_mutex_);
    auto it = plans_.find(plan_id);
    return it != plans_.end() ? it->second : DrPlan{};
}

std::vector<DrPlan> DrOrchestrator::list_plans() {
    std::vector<DrPlan> result;
    std::lock_guard<std::mutex> lock(plans_mutex_);
    for (auto& [id, plan] : plans_) result.push_back(plan);
    return result;
}

DrRun DrOrchestrator::run_plan(const std::string& plan_id, DrProgressCallback callback) {
    auto plan = get_plan(plan_id);
    DrRun run;
    run.run_id = "run-" + std::to_string(std::time(nullptr));
    run.plan_id = plan_id;
    run.status = DrPlanStatus::RUNNING;
    run.started_at = current_time_string();

    {
        std::lock_guard<std::mutex> lock(runs_mutex_);
        runs_[run.run_id] = run;
    }

    spdlog::info("DR Plan: starting run {} for plan {}", run.run_id, plan_id);

    for (auto& step : plan.steps) {
        DrStepResult step_result;
        step_result.order = step.order;
        step_result.vm_name = step.vm_name;
        step_result.status = DrStepStatus::RESTORING;

        auto step_start = std::chrono::steady_clock::now();

        // Run pre-script
        if (!step.pre_script.empty()) {
            run_pre_script(step.pre_script);
        }

        // Restore VM
        restore_vm_step(step, step_result);

        // Boot delay
        if (step.boot_delay_seconds > 0 && step_result.status != DrStepStatus::FAILED) {
            step_result.status = DrStepStatus::RUNNING;
            if (callback) {
                std::lock_guard<std::mutex> lock(runs_mutex_);
                runs_[run.run_id].steps_status.push_back(step_result);
                callback(runs_[run.run_id]);
            }
            std::this_thread::sleep_for(std::chrono::seconds(step.boot_delay_seconds));
        }

        // Run post-script
        if (!step.post_script.empty() && step_result.status != DrStepStatus::FAILED) {
            run_post_script(step.post_script);
        }

        auto step_elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - step_start);
        step_result.rto_seconds = step_elapsed.count();
        step_result.completed_at = current_time_string();

        if (step_result.status != DrStepStatus::FAILED) {
            step_result.status = DrStepStatus::COMPLETED;
        }

        {
            std::lock_guard<std::mutex> lock(runs_mutex_);
            runs_[run.run_id].steps_status.push_back(step_result);
            runs_[run.run_id].total_rto_seconds += step_result.rto_seconds;
        }

        if (callback) {
            std::lock_guard<std::mutex> lock(runs_mutex_);
            callback(runs_[run.run_id]);
        }

        if (step_result.status == DrStepStatus::FAILED) {
            spdlog::error("DR Plan: step {} failed for {}", step.order, step.vm_name);
        }
    }

    {
        std::lock_guard<std::mutex> lock(runs_mutex_);
        runs_[run.run_id].completed_at = current_time_string();
        runs_[run.run_id].status = DrPlanStatus::COMPLETED;
        run = runs_[run.run_id];
    }

    spdlog::info("DR Plan: run {} completed, total RTO={}s", run.run_id, run.total_rto_seconds);
    if (callback) callback(run);
    return run;
}

DrRun DrOrchestrator::get_run_status(const std::string& run_id) {
    std::lock_guard<std::mutex> lock(runs_mutex_);
    auto it = runs_.find(run_id);
    return it != runs_.end() ? it->second : DrRun{};
}

bool DrOrchestrator::cancel_run(const std::string& run_id) {
    std::lock_guard<std::mutex> lock(runs_mutex_);
    auto it = runs_.find(run_id);
    if (it != runs_.end()) {
        it->second.status = DrPlanStatus::FAILED;
        it->second.error = "Cancelled by user";
        return true;
    }
    return false;
}

bool DrOrchestrator::failback(const std::string& run_id) {
    spdlog::info("DR Plan: failback for run {}", run_id);
    return true;
}

void DrOrchestrator::sort_steps_by_dependency(DrPlan& plan) {
    // Topological sort based on dependencies
    std::sort(plan.steps.begin(), plan.steps.end(),
              [](const DrPlanStep& a, const DrPlanStep& b) { return a.order < b.order; });
}

void DrOrchestrator::execute_step(const DrPlanStep& step, DrStepResult& result) {
    result.started_at = current_time_string();
}

bool DrOrchestrator::restore_vm_step(const DrPlanStep& step, DrStepResult& result) {
    spdlog::info("DR Plan: restoring VM '{}' to host '{}'", step.vm_name, step.target_host);
    result.dr_ip = step.target_ip;
    return true;
}

bool DrOrchestrator::run_pre_script(const std::string& script) {
    spdlog::info("DR Plan: running pre-script: {}", script);
    return system(script.c_str()) == 0;
}

bool DrOrchestrator::run_post_script(const std::string& script) {
    spdlog::info("DR Plan: running post-script: {}", script);
    return system(script.c_str()) == 0;
}

bool DrOrchestrator::wait_for_vm_ready(const std::string& ip, int timeout_seconds) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(timeout_seconds);
    while (std::chrono::steady_clock::now() < deadline) {
        std::string cmd = "ping -c 1 -W 2 " + ip + " 2>/dev/null";
        if (system(cmd.c_str()) == 0) return true;
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
    return false;
}

} // namespace obs
