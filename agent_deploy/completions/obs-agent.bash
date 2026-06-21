_obs_agent_completions() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="start stop restart status diag version config upgrade clear-cache"
    case "${prev}" in
        config) COMPREPLY=($(compgen -W "--edit --show" -- "${cur}")); return 0 ;;
        *) COMPREPLY=($(compgen -W "${opts} --help --server --token --log-level" -- "${cur}")); return 0 ;;
    esac
}
complete -F _obs_agent_completions obs-agent
