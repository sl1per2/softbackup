#include "framework/application.h"
#include "agent/agent_factory.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <boost/program_options.hpp>
#include <signal.h>

namespace po = boost::program_options;

namespace obs {

static std::atomic<bool> g_shutdown{false};
static void signal_handler(int) { g_shutdown = true; }

Application::Application() {
    service_registry_ = std::make_shared<ServiceRegistry>();
    event_bus_ = std::make_shared<EventBus>();
    command_queue_ = std::make_shared<CommandQueue>();
}

Application::~Application() {
    shutdown();
}

void Application::initialize(const ApplicationConfig& config) {
    config_ = config;

    setup_logging(config.log_level, config.log_dir);
    setup_signal_handlers();

    spdlog::info("=== OBS Backup Core v{} ===", "2.0.0");
    spdlog::info("Initializing application...");

    initialize_subsystems();
    initialize_agents();
    initialize_ipc();

    spdlog::info("Application initialized successfully");
}

void Application::run() {
    running_ = true;
    spdlog::info("Core ready. Press Ctrl+C to shutdown.");

    event_loop();

    spdlog::info("Shutting down...");
    running_ = false;
}

void Application::shutdown() {
    if (!running_.exchange(false)) return;

    spdlog::info("Application shutting down...");

    if (ipc_server_) {
        ipc_server_->stop();
    }

    if (agent_) {
        agent_->stop();
    }

    event_bus_->clear();
    service_registry_->shutdown_all();

    spdlog::info("Application stopped.");
}

void Application::setup_logging(const std::string& log_level, const std::string& log_dir) {
    auto console_sink = std::make_shared<spdlog::sinks::stdout_sink_mt>();

    namespace fs = std::filesystem;
    if (!fs::exists(log_dir)) {
        fs::create_directories(log_dir);
    }

    auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        log_dir + "/core.log", 10 * 1024 * 1024, 3);

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    auto logger = std::make_shared<spdlog::logger>("obs", sinks.begin(), sinks.end());
    logger->set_level(spdlog::level::from_str(log_level));
    spdlog::set_default_logger(logger);
}

void Application::setup_signal_handlers() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

void Application::initialize_subsystems() {
    auto& factory = EngineFactory::instance();
    factory.set_registry(service_registry_);
    pipeline_ = factory.create_pipeline(config_.data_dir, config_.cache_dir);

    service_registry_->register_instance(pipeline_);
    service_registry_->register_instance(event_bus_);
    service_registry_->register_instance(command_queue_);

    cdp_engine_ = std::make_shared<CdpEngine>();
    cdp_engine_->initialize();
    std::shared_ptr<ICdpEngine> cdp_iface = cdp_engine_;
    service_registry_->register_instance<ICdpEngine>(cdp_iface);

    event_bus_->subscribe<JobStartedEvent>([](const JobStartedEvent& e) {
        spdlog::info("[Event] Job started: {}", e.job_id);
    });
    event_bus_->subscribe<JobCompletedEvent>([](const JobCompletedEvent& e) {
        spdlog::info("[Event] Job {} completed: success={}", e.job_id, e.success);
    });
    event_bus_->subscribe<AgentConnectedEvent>([](const AgentConnectedEvent& e) {
        spdlog::info("[Event] Agent connected: {} ({})", e.agent_id, e.hostname);
    });

    spdlog::info("Subsystems initialized");
}

void Application::initialize_agents() {
    auto& factory = AgentFactory::instance();

    agent_ = factory.create_by_name(config_.agent_type);
    if (!agent_) {
        spdlog::warn("Unknown agent type '{}', falling back to generic", config_.agent_type);
        agent_ = factory.create(AgentType::GENERIC);
    }

    agent_->set_pipeline(pipeline_);
    agent_->set_event_bus(event_bus_);

    AgentConfig agent_config;
    agent_config.agent_id = config_.agent_type + "_" + current_time_string();
    agent_config.server_url = config_.server_url;
    agent_config.socket_path = config_.socket_path;
    agent_config.data_dir = config_.data_dir;
    agent_config.cache_dir = config_.cache_dir;

    agent_->start(agent_config);
    service_registry_->register_instance<IAgent>(agent_);

    spdlog::info("Agent initialized: {} (type={})", agent_->get_status().agent_id,
                 agent_->type_name());
}

void Application::initialize_ipc() {
#ifdef _WIN32
    ipc_server_ = std::make_shared<NamedPipeServer>(config_.socket_path);
#else
    ipc_server_ = std::make_shared<UnixSocketServer>(io_ctx_, config_.socket_path);
#endif

    ipc_server_->initialize();

    message_router_ = std::make_shared<MessageRouter>(ipc_server_, event_bus_);
    message_router_->register_agent(agent_);
    message_router_->register_cdp_engine(cdp_engine_);
    message_router_->register_backup_engine(pipeline_->backup_engine());
    message_router_->initialize();

    ipc_server_->start();
    service_registry_->register_instance<IIpcServer>(ipc_server_);

    spdlog::info("IPC initialized");
}

void Application::event_loop() {
    while (running_ && !g_shutdown) {
        io_ctx_.poll_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

} // namespace obs

int main(int argc, char* argv[]) {
    try {
        po::options_description desc("OBS Backup Core v2.0");
        desc.add_options()
            ("help,h", "Show help")
            ("config,c", po::value<std::string>()->default_value("/etc/obs/core.conf"), "Config file")
            ("daemon,d", po::bool_switch()->default_value(false), "Run as daemon")
            ("socket-path,s", po::value<std::string>()->default_value("/tmp/obs-core.sock"), "IPC socket path")
            ("log-level,l", po::value<std::string>()->default_value("info"), "Log level")
            ("agent-type,t", po::value<std::string>()->default_value("generic"),
                "Agent type (generic/esxi/hyperv/mssql/postgresql/oracle/proxmox/saphana/kubernetes/linux_fs/windows_fs/ndmp/exchange)")
            ("server-url,u", po::value<std::string>()->default_value("http://localhost:8000"), "Server URL");

        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        obs::ApplicationConfig app_config;
        app_config.config_file = vm["config"].as<std::string>();
        app_config.socket_path = vm["socket-path"].as<std::string>();
        app_config.log_level = vm["log-level"].as<std::string>();
        app_config.agent_type = vm["agent-type"].as<std::string>();
        app_config.server_url = vm["server-url"].as<std::string>();
        app_config.daemon = vm["daemon"].as<bool>();

        obs::Application app;
        app.initialize(app_config);
        app.run();

    } catch (const std::exception& e) {
        spdlog::error("Fatal error: {}", e.what());
        return 1;
    }
    return 0;
}
