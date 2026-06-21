#compdef obs-agent

_obs_agent() {
    _arguments \
        '1:command:(start stop restart status diag version config upgrade clear-cache)' \
        '--server[Server URL]:url:' \
        '--token[Agent token]:token:' \
        '--log-level[Log level]:(debug info warn error)' \
        '--help[Show help]'
}

_obs_agent "$@"
