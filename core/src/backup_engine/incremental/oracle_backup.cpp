#include "backup_engine/incremental/oracle_backup.h"
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <regex>
#include <chrono>

namespace obs {

OracleBackup::OracleBackup() {}
OracleBackup::~OracleBackup() {}

std::string OracleBackup::get_connection_string(const OracleBackupConfig& config) {
    if (!config.service_name.empty()) {
        return config.username + "/" + config.password + "@" + config.host + ":" + config.port + "/" + config.service_name;
    }
    return config.username + "/" + config.password + "@" + config.sid;
}

std::string OracleBackup::build_rman_script(const OracleBackupConfig& config, const std::string& action) {
    std::ostringstream script;
    script << "CONNECT TARGET " << get_connection_string(config) << "\n";
    script << "CONFIGURE CONTROLFILE AUTOBACKUP ON;\n";

    if (config.compressed) {
        script << "CONFIGURE DEVICE TYPE DISK PARALLELISM 4 BACKUP TYPE TO COMPRESSED BACKUPSET;\n";
    }

    script << action << "\n";
    script << "EXIT;\n";
    return script.str();
}

std::string OracleBackup::run_rman(const std::string& script, const OracleBackupConfig& config) {
    std::string script_path = "/tmp/vovqa_rman_" + std::to_string(std::time(nullptr)) + ".rman";
    std::ofstream ofs(script_path);
    ofs << script;
    ofs.close();

    std::string cmd = config.rman_path + " TARGET " + get_connection_string(config)
                    + " CMDFILE " + script_path + " LOG /tmp/vovqa_rman.log 2>&1";
    spdlog::info("Running RMAN: {}", cmd);
    system(cmd.c_str());

    std::ifstream ifs("/tmp/vovqa_rman.log");
    std::string output;
    if (ifs) {
        std::string line;
        while (std::getline(ifs, line)) output += line + "\n";
    }
    std::remove(script_path.c_str());
    return output;
}

OracleBackupResult OracleBackup::parse_rman_output(const std::string& output) {
    OracleBackupResult result;
    result.output_log = output;

    // Parse SCN
    std::regex scn_regex("starting partial backup at (\\d+)");
    std::smatch scn_match;
    if (std::regex_search(output, scn_match, scn_regex)) {
        result.scn_from = std::stoll(scn_match[1].str());
    }

    std::regex scn_to_regex("including current controlfile in backupset");
    result.scn_to = result.scn_from + 1000000;

    // Parse bytes backed up
    std::regex bytes_regex("backup set complete, elapsed time:.*?Output File:(.*?)(?:\\n|$)");
    std::regex size_regex("(\\d+)\\s+bytes");
    std::smatch size_match;
    std::string remaining = output;
    while (std::regex_search(remaining, size_match, size_regex)) {
        result.bytes_backed_up += std::stoll(size_match[1].str());
        remaining = size_match.suffix();
    }

    // Parse files backed up
    std::regex files_regex("(\\d+)\\s+files?\\s+backed\\s+up");
    std::smatch files_match;
    if (std::regex_search(output, files_match, files_regex)) {
        result.files_backed_up = std::stoi(files_match[1].str());
    }

    // Check BCT usage
    result.bct_used = output.find("Block Change Tracking") != std::string::npos;

    result.compression_ratio = result.bytes_backed_up > 0 ? 1.0 : 0;
    result.success = output.find("RMAN-00569") == std::string::npos &&
                     output.find("RMAN-00571") == std::string::npos &&
                     output.find("ORA-") == std::string::npos;
    if (!result.success) {
        auto err_pos = output.find("ORA-");
        if (err_pos != std::string::npos) {
            result.error = output.substr(err_pos, std::min(output.find('\n', err_pos) - err_pos, size_t(200)));
        }
    }

    return result;
}

bool OracleBackup::enable_bct(const OracleBackupConfig& config) {
    spdlog::info("Oracle: enabling Block Change Tracking");
    std::string action = "ALTER DATABASE ENABLE BLOCK CHANGE TRACKING USING FILE '"
                       + config.backup_dest + "/bct/vovqa_bct.ctf';";
    std::string script = build_rman_script(config, action);
    std::string output = run_rman(script, config);
    return output.find("ORA-") == std::string::npos;
}

bool OracleBackup::is_bct_enabled(const OracleBackupConfig& config) {
    std::string action = "SELECT STATUS FROM V$BLOCK_CHANGE_TRACKING;";
    std::string script = build_rman_script(config, action);
    std::string output = run_rman(script, config);
    return output.find("ENABLED") != std::string::npos;
}

std::string OracleBackup::get_current_scn(const OracleBackupConfig& config) {
    std::string action = "SELECT CURRENT_SCN FROM V$DATABASE;";
    std::string script = build_rman_script(config, action);
    std::string output = run_rman(script, config);
    std::regex scn_regex("(\\d+)");
    std::smatch match;
    if (std::regex_search(output, match, scn_regex)) {
        return match[1].str();
    }
    return "0";
}

OracleBackupResult OracleBackup::full_backup(const OracleBackupConfig& config) {
    spdlog::info("Oracle: RMAN LEVEL 0 full backup");
    std::ostringstream action;
    action << "BACKUP INCREMENTAL LEVEL 0 DATABASE TAG 'OBS_FULL'";
    if (config.compressed) action << " AS COMPRESSED BACKUPSET";
    action << " FORMAT '" << config.backup_dest << "/full_%d_%T_%s_%p'";
    if (config.include_archivelog) action << " PLUS ARCHIVELOG DELETE INPUT";
    if (config.include_spfile) action << "; BACKUP SPFILE FORMAT '" << config.backup_dest << "/spfile_%d_%T_%s_%p'";
    if (config.include_controlfile) action << "; BACKUP CURRENT CONTROLFILE FORMAT '" << config.backup_dest << "/controlfile_%d_%T_%s_%p'";
    action << ";";

    auto start = std::chrono::steady_clock::now();
    std::string output = run_rman(build_rman_script(config, action.str()), config);
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();

    auto result = parse_rman_output(output);
    result.elapsed_seconds = elapsed;
    return result;
}

OracleBackupResult OracleBackup::incremental_backup(const OracleBackupConfig& config, int level) {
    spdlog::info("Oracle: RMAN LEVEL {} incremental backup", level);
    std::ostringstream action;
    action << "BACKUP INCREMENTAL LEVEL " << level << " DATABASE TAG 'OBS_INCR_L" << level << "'";
    if (config.compressed) action << " AS COMPRESSED BACKUPSET";
    action << " FORMAT '" << config.backup_dest << "/incr_L" << level << "_%d_%T_%s_%p'";
    if (config.include_archivelog) action << " PLUS ARCHIVELOG DELETE INPUT";
    action << ";";

    auto start = std::chrono::steady_clock::now();
    std::string output = run_rman(build_rman_script(config, action.str()), config);
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();

    auto result = parse_rman_output(output);
    result.elapsed_seconds = elapsed;
    result.bct_used = is_bct_enabled(config);
    return result;
}

OracleBackupResult OracleBackup::archivelog_backup(const OracleBackupConfig& config) {
    spdlog::info("Oracle: archivelog backup");
    std::string action = "BACKUP ARCHIVELOG ALL FORMAT '" + config.backup_dest
                       + "/arch_%d_%T_%s_%p' DELETE INPUT;";
    auto start = std::chrono::steady_clock::now();
    std::string output = run_rman(build_rman_script(config, action), config);
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start).count();
    auto result = parse_rman_output(output);
    result.elapsed_seconds = elapsed;
    return result;
}

OracleBackupResult OracleBackup::spfile_backup(const OracleBackupConfig& config) {
    spdlog::info("Oracle: spfile backup");
    std::string action = "BACKUP SPFILE FORMAT '" + config.backup_dest + "/spfile_%d_%T_%s_%p';";
    std::string output = run_rman(build_rman_script(config, action), config);
    return parse_rman_output(output);
}

OracleBackupResult OracleBackup::controlfile_backup(const OracleBackupConfig& config) {
    spdlog::info("Oracle: controlfile backup");
    std::string action = "BACKUP CURRENT CONTROLFILE FORMAT '" + config.backup_dest + "/controlfile_%d_%T_%s_%p';";
    std::string output = run_rman(build_rman_script(config, action), config);
    return parse_rman_output(output);
}

OracleBackupResult OracleBackup::backup_all(const OracleBackupConfig& config,
                                            std::function<void(const OracleBackupResult&)> callback) {
    OracleBackupResult result;

    // Enable BCT if requested
    if (config.enable_bct && !is_bct_enabled(config)) {
        spdlog::info("Oracle: enabling Block Change Tracking");
        enable_bct(config);
    }

    // Full backup
    result = full_backup(config);
    if (callback) callback(result);
    if (!result.success) return result;

    // Archivelog
    if (config.include_archivelog) {
        auto arch_result = archivelog_backup(config);
        result.bytes_backed_up += arch_result.bytes_backed_up;
        result.files_backed_up += arch_result.files_backed_up;
        if (callback) callback(result);
    }

    // SPFILE
    if (config.include_spfile) {
        auto spfile_result = spfile_backup(config);
        result.bytes_backed_up += spfile_result.bytes_backed_up;
    }

    // Controlfile
    if (config.include_controlfile) {
        auto cf_result = controlfile_backup(config);
        result.bytes_backed_up += cf_result.bytes_backed_up;
    }

    result.success = true;
    return result;
}

bool OracleBackup::restore(const OracleBackupConfig& config, const std::string& backup_path,
                           int64_t target_scn, const std::string& target_time) {
    spdlog::info("Oracle: restoring from {}", backup_path);
    std::ostringstream action;
    action << "STARTUP MOUNT;\n";
    action << "RESTORE DATABASE;\n";
    if (target_scn > 0) {
        action << "SET UNTIL SCN " << target_scn << ";\n";
    } else if (!target_time.empty()) {
        action << "SET UNTIL TIME '" << target_time << "';\n";
    }
    action << "RECOVER DATABASE;\n";
    action << "ALTER DATABASE OPEN RESETLOGS;\n";

    std::string output = run_rman(build_rman_script(config, action.str()), config);
    return output.find("ORA-") == std::string::npos;
}

} // namespace obs
