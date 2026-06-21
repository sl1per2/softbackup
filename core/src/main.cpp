#include "common.h"
#include "agent/agent.h"
#include "backup_engine/backup_job.h"
#include "dedup/dedup_engine.h"
#include "crypto/crypto_engine.h"
#include "cdp/cdp_engine.h"
#include "ipc/ipc_server.h"
#include "ipc/unix_socket_server.h"
#include "ipc/message_handler.h"
#include "data_mover/data_mover.h"
#include "traffic_optimizer/traffic_optimizer.h"

#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <signal.h>
#include <atomic>

namespace po = boost::program_options;
static std::atomic<bool> g_shutdown{false};

static void signal_handler(int) { g_shutdown = true; }

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("OBS Backup Core");
        desc.add_options()
            ("help,h", "Show help")
            ("config,c", po::value<std::string>()->default_value("/etc/obs/core.conf"), "Config file")
            ("daemon,d", po::bool_switch()->default_value(false), "Run as daemon")
            ("socket-path,s", po::value<std::string>()->default_value("/tmp/obs-core.sock"), "IPC socket path")
            ("log-level,l", po::value<std::string>()->default_value("info"), "Log level");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        // Setup logging
        auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "/var/log/obs/core.log", 10 * 1024 * 1024, 3);
        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        auto logger = std::make_shared<spdlog::logger>("obs", sinks.begin(), sinks.end());
        logger->set_level(spdlog::level::from_str(vm["log-level"].as<std::string>()));
        spdlog::set_default_logger(logger);

        spdlog::info("OBS Backup Core starting...");

        // Signal handling
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        // Boost.Asio io_context
        boost::asio::io_context io_ctx;

        // Initialize subsystems
        auto agent = std::make_unique<obs::Agent>();
        auto cdp_engine = std::make_unique<obs::CdpEngine>();
        auto traffic_optimizer = std::make_unique<obs::TrafficOptimizer>();

        // Start agent
        std::string server_url = "http://localhost:8000";
        std::string socket_path = vm["socket-path"].as<std::string>();
        agent->start(server_url, socket_path);

        // IPC Server (cross-platform)
#ifdef _WIN32
        std::string pipe_name = "obs-core";
        auto ipc_server = std::make_unique<obs::NamedPipeServer>(io_ctx, pipe_name);
#else
        auto ipc_server = std::make_unique<obs::UnixSocketServer>(io_ctx, socket_path);
#endif

        // Register IPC handlers
        auto handler = std::make_shared<obs::MessageHandler>();
        handler->set_cdp_engine(cdp_engine.get());
        handler->set_backup_job_factory([]() {
            obs::BackupConfig cfg;
            cfg.job_id = "manual_" + obs::current_time_string();
            return std::make_shared<obs::BackupJob>(cfg);
        });

        ipc_server->register_handler(1, [handler](const std::vector<uint8_t>& d) {
            return handler->handle(1, d);
        });
        ipc_server->register_handler(2, [handler](const std::vector<uint8_t>& d) {
            return handler->handle(2, d);
        });
        ipc_server->register_handler(3, [handler](const std::vector<uint8_t>& d) {
            return handler->handle(3, d);
        });
        ipc_server->register_handler(4, [handler](const std::vector<uint8_t>& d) {
            return handler->handle(4, d);
        });
        ipc_server->register_handler(5, [handler](const std::vector<uint8_t>& d) {
            return handler->handle(5, d);
        });

        ipc_server->start();

        spdlog::info("Core ready. Press Ctrl+C to shutdown.");

        // Main event loop
        while (!g_shutdown) {
            io_ctx.poll_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        spdlog::info("Shutting down...");
        ipc_server->stop();
        agent->stop();

        spdlog::info("OBS Backup Core stopped.");

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }
    return 0;
}
